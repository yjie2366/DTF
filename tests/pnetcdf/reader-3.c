/*********************************************************************
 *
 *  Copyright (C) 2013, Northwestern University
 *  See COPYRIGHT notice in top-level directory.
 *
 *********************************************************************/
/* $Id: aggregation.c 2325 2016-02-28 07:49:13Z wkliao $ */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <pnetcdf.h>
#include "pfarb.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This program writes a series of 2D variables with data partitioning patterns
 * of block-block, *-cyclic, block-*, and *-block, round-robinly. The block-*
 * partitioning case writes 1st half followed by 2nd half. The same partitioning
 * patterns are used for read. In both cases, nonblocking APIs are used to
 * evaluate the performance.
 * 
 * The compile and run commands are given below, together with an ncmpidump of
 * the output file. In this example, NVARS = 5.
 *
 *    % mpicc -O2 -o aggregation aggregation.c -lpnetcdf
 *
 *    % mpiexec -l -n 4 ./aggregation 5 /pvfs2/wkliao/testfile.nc
 *
 *    % ncmpidump /pvfs2/wkliao/testfile.nc
 *      netcdf testfile {
 *      // file format: CDF-5 (big variables)
 *      dimensions:
 *              Block_BLOCK_Y = 10 ;
 *              Block_BLOCK_X = 10 ;
 *              Star_Y = 5 ;
 *              Cyclic_X = 20 ;
 *              Block_Y = 20 ;
 *              Star_X = 5 ;
 *      variables:
 *              int block_block_var_0(Block_BLOCK_Y, Block_BLOCK_X) ;
 *              float star_cyclic_var_1(Star_Y, Cyclic_X) ;
 *              short block_star_var_2(Block_Y, Star_X) ;
 *              double star_block_var_3(Star_Y, Cyclic_X) ;
 *              int block_block_var_4(Block_BLOCK_Y, Block_BLOCK_X) ;
 *      data:
 *
 *       block_block_var_0 =
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3 ;
 *
 *       star_cyclic_var_1 =
 *        0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3,
 *        0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3,
 *        0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3,
 *        0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3,
 *        0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3 ;
 *
 *       block_star_var_2 =
 *        0, 0, 0, 0, 0,
 *        0, 0, 0, 0, 0,
 *        0, 0, 0, 0, 0,
 *        0, 0, 0, 0, 0,
 *        0, 0, 0, 0, 0,
 *        1, 1, 1, 1, 1,
 *        1, 1, 1, 1, 1,
 *        1, 1, 1, 1, 1,
 *        1, 1, 1, 1, 1,
 *        1, 1, 1, 1, 1,
 *        2, 2, 2, 2, 2,
 *        2, 2, 2, 2, 2,
 *        2, 2, 2, 2, 2,
 *        2, 2, 2, 2, 2,
 *        2, 2, 2, 2, 2,
 *        3, 3, 3, 3, 3,
 *        3, 3, 3, 3, 3,
 *        3, 3, 3, 3, 3,
 *        3, 3, 3, 3, 3,
 *        3, 3, 3, 3, 3 ;
 *
 *       star_block_var_3 =
 *        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
 *        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
 *        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
 *        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
 *        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3 ;
 *
 *       block_block_var_4 =
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3,
 *        1, 1, 1, 1, 1, 3, 3, 3, 3, 3 ;
 *      }
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define NVARS 5

#define ERR(e) {if((e)!=NC_NOERR)printf("Error at line=%d: %s\n", __LINE__, ncmpi_strerror(e));}

/*----< print_info() >------------------------------------------------------*/
static
void print_info(MPI_Info *info_used)
{
    int  i, nkeys;

    MPI_Info_get_nkeys(*info_used, &nkeys);
    printf("MPI File Info: nkeys = %d\n",nkeys);
    for (i=0; i<nkeys; i++) {
        char key[MPI_MAX_INFO_KEY], value[MPI_MAX_INFO_VAL];
        int  valuelen, flag;

        MPI_Info_get_nthkey(*info_used, i, key);
        MPI_Info_get_valuelen(*info_used, key, &valuelen, &flag);
        MPI_Info_get(*info_used, key, valuelen+1, value, &flag);
        printf("MPI File Info: [%2d] key = %24s, value = %s\n",i,key,value);
    }
}

/*----< benchmark_read() >---------------------------------------------------*/
static
int benchmark_read(char       *filename,
                   MPI_Offset  len,
                   MPI_Offset *r_size,
                   MPI_Info   *r_info_used,
                   double     *timing)  /* [5] */
{
    int i, j, k, verbose, rank, nprocs, s_rank, err, num_reqs;
    int ncid, ncid2, varid2, omode, varid[NVARS], *reqs, *sts, psizes[2];
    void *buf[NVARS];
    double start_t, end_t;
    int *arr;
    MPI_Comm comm=MPI_COMM_WORLD;
    MPI_Offset start[2], count[2];
    MPI_Info info=MPI_INFO_NULL;

    verbose = 1;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);
    s_rank = rank;//(rank + nprocs / 2 ) % nprocs;

    psizes[0] = psizes[1] = 0;
    MPI_Dims_create(nprocs, 2, psizes);

    /* allocate I/O buffer */
    for (i=0; i<NVARS; i++) {
        if (i % 4 == 0)
            buf[i] = malloc(len * len * sizeof(int));
        else if (i % 4 == 1)
            buf[i] = (float*) malloc(len * len * sizeof(float));
        else if (i % 4 == 2)
            buf[i] = (short*) malloc(len * len * sizeof(short));
        else
            buf[i] = (double*) malloc(len * len * sizeof(double));
    }
    MPI_Barrier(comm);
    timing[0] = MPI_Wtime();

    /* open file for reading -----------------------------------------*/
    omode = NC_NOWRITE;
    err = ncmpi_open(comm, filename, omode, info, &ncid); ERR(err)
    start_t = MPI_Wtime();
    timing[1] = start_t - timing[0];
    

    /* Note that PnetCDF read the file in chunks of size 256KB, thus the read
     * amount may be more than the file header size
     */
    if (verbose) {
        MPI_Offset h_size, h_extent;
        ncmpi_inq_header_size(ncid, &h_size);
        ncmpi_inq_header_extent(ncid, &h_extent);
        printf("File header size=%lld extent=%lld\n",h_size,h_extent);
    }

    num_reqs = 0;
    for (i=0; i<NVARS; i++) {
        varid[i] = i;
        if (i % 4 == 0)
            num_reqs++; /* complete in 1 nonblocking call */
        else if (i % 4 == 1)
            num_reqs += len; /* complete in len nonblocking calls */
        else if (i % 4 == 2)
            num_reqs += 2; /* write 1st half followed by 2nd half */
        else
            num_reqs++; /* complete in 1 nonblocking call */
    }
    reqs = (int*) malloc(num_reqs * sizeof(int));

    k = 0; int j1;
    for (i=0; i<NVARS; i++) {
        if (i % 4 == 0) {
            int *int_b = (int*) buf[i];
            //start[0] = len * (s_rank % psizes[0]);
            //start[1] = len * ((s_rank / psizes[1]) % psizes[1]);
			start[0] = len * (rank % psizes[0])*len*psizes[1]+len * ((rank / psizes[1]) % psizes[1]);
            //start[1] = len * ((s_rank / psizes[1]) % psizes[1]);
            count[0] = len*len;
            //count[1] = len;
            err = ncmpi_iget_vara_int(ncid, varid[i], start, count, int_b,
                                      &reqs[k++]);
            ERR(err)
            

        }
        else if (i % 4 == 1) {
            float *flt_b = (float*) buf[i];
            start[0] = 0;
            count[0] = len;
            count[1] = 1;
            for (j=0; j<len; j++) {
			    
                start[1] = s_rank + j * nprocs;
                err = ncmpi_iget_vara_float(ncid, varid[i], start, count,
                                            flt_b, &reqs[k++]);
                ERR(err)
                flt_b += len;
           }

        }
        else if (i % 4 == 2) {
            short *shr_b = (short*) buf[i];
            start[0] = len * s_rank;
            start[1] = 0;
            count[0] = len;
            count[1] = len/2;
            err = ncmpi_iget_vara_short(ncid, varid[i], start, count,
                                        shr_b, &reqs[k++]);
            ERR(err)

            shr_b += len * (len/2);
            start[1] = len/2;
            count[1] = len - len/2;
            err = ncmpi_iget_vara_short(ncid, varid[i], start, count,
                                        shr_b, &reqs[k++]);
            ERR(err)
        }
        else {
            double *dbl_b = (double*) buf[i];
            start[0] = 0;
            start[1] = len * s_rank;
            count[0] = len;
            count[1] = len;
            err = ncmpi_iget_vara_double(ncid, varid[i], start, count, dbl_b,
                                         &reqs[k++]);
            ERR(err)
        }
    }
    num_reqs = k;

    end_t = MPI_Wtime();
    timing[2] = end_t - start_t;
    start_t = end_t;

    sts = (int*) malloc(num_reqs * sizeof(int));

#ifdef USE_INDEP_MODE
    err = ncmpi_begin_indep_data(ncid);          ERR(err)
    err = ncmpi_wait(ncid, num_reqs, reqs, sts); ERR(err)
    err = ncmpi_end_indep_data(ncid);            ERR(err)
#else
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); ERR(err)
#endif
	printf("r%d: after wait\n", rank);
    farb_match_io(filename, -1);
    printf("r%d: Finished with restart.nc\n", rank);
    /* check status of all requests */
    for (i=0; i<num_reqs; i++) ERR(sts[i])

    end_t = MPI_Wtime();
    timing[3] = end_t - start_t;
    start_t = end_t;
            /*open another file*/
    err = ncmpi_open(comm, "bla.nc", omode, info, &ncid2); ERR(err)
	 /*read another file*/
    MPI_Offset offt = rank*2, sz = 2, len2 = nprocs*2;
    arr = (int*)malloc(len2*sizeof(int));
	 varid2 = 0;
     err = ncmpi_get_vara_int_all(ncid2, varid2, &offt, &sz, &arr[rank*2]);  ERR(err);
	 farb_match_io("bla.nc", -1);
	 printf("r%d: Finished with bla.nc\n", rank);

    
    for (i=0; i<NVARS; i++) {
        if (i % 4 == 0){
			int *int_b = (int*) buf[i];
			printf("r%d: var %d: ", rank, i);
			for(j1 = 0; j1 < len*len; j1++)
				printf("%d ", int_b[j1]);
			printf("\n");
		}
	   else if (i % 4 == 1){
			float *flt_b = (float*) buf[i];
			printf("r%d: var %d: ", rank, i);
			for(j1 = 0; j1 < len*len; j1++)
			printf("%d ", (int)flt_b[j1]);
			printf("\n");	
		}
            
        else if (i % 4 == 2){
			short *shr_b = (short*) buf[i];
			printf("r%d: var %d: ", rank, i);
			for(j1 = 0; j1 < len*len; j1++)
			printf("%d ", (int)shr_b[j1]);
			printf("\n");
		}
            
        else{
			double *dbl_b = (double*) buf[i];
			printf("r%d: var %d: ", rank, i);
			for(j1 = 0; j1 < len*len; j1++)
			printf("%d ", (int)dbl_b[j1]);
			printf("\n");
		}
            
    }

    /* get the true I/O amount committed */
    err = ncmpi_inq_get_size(ncid, r_size); ERR(err)
    printf("total get size %d\n", (int)(*r_size));
    /* get all the hints used */
    err = ncmpi_get_file_info(ncid, r_info_used); ERR(err)
	err = ncmpi_close(ncid2); ERR(err)
    err = ncmpi_close(ncid); ERR(err)
    
    end_t = MPI_Wtime();
    timing[4] = end_t - start_t;
    timing[0] = end_t - timing[0];

    free(sts);
    free(reqs);
    free(arr);
    for (i=0; i<NVARS; i++) free(buf[i]);

    return 1;
}

/*----< main() >--------------------------------------------------------------*/
int main(int argc, char** argv) {
    int rank, nprocs;
    double timing[11], max_t[11];
    MPI_Offset len, r_size=0, sum_r_size;
    MPI_Comm comm=MPI_COMM_WORLD;
    MPI_Info r_info_used;
    char filename[] = "restart.nc";

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);
    farb_init("farb.ini", "ireader");

    len = 4;

    benchmark_read (filename, len, &r_size, &r_info_used, timing+6);

    MPI_Reduce(&timing, &max_t,     11, MPI_DOUBLE, MPI_MAX, 0, comm);
#ifdef MPI_OFFSET
    MPI_Reduce(&r_size, &sum_r_size, 1, MPI_OFFSET, MPI_SUM, 0, comm);
#else
    MPI_Reduce(&r_size, &sum_r_size, 1, MPI_LONG_LONG, MPI_SUM, 0, comm);
#endif
    //if (rank == 0) {
    if(0){
        double bw = sum_r_size;
        bw /= 1048576.0;
        printf("-----------------------------------------------------------\n");
        printf("Read  %d variables using nonblocking APIs\n", NVARS);
        printf("In each process, the local variable size is %lld x %lld\n", len,len);
        printf("Total read  amount        = %13lld    B\n", sum_r_size);
        printf("            amount        = %16.4f MiB\n", bw);
        printf("            amount        = %16.4f GiB\n", bw/1024);
        printf("Max file open/create time = %16.4f sec\n", max_t[7]);
        printf("Max nonblocking post time = %16.4f sec\n", max_t[8]);
        printf("Max nonblocking wait time = %16.4f sec\n", max_t[9]);
        printf("Max file close       time = %16.4f sec\n", max_t[10]);
        printf("Max open-to-close    time = %16.4f sec\n", max_t[6]);
        printf("Read  bandwidth           = %16.4f MiB/s\n", bw/max_t[6]);
        bw /= 1024.0;
        printf("Read  bandwidth           = %16.4f GiB/s\n", bw/max_t[6]);
    }
    MPI_Info_free(&r_info_used);

    /* check if there is any PnetCDF internal malloc residue */
    MPI_Offset malloc_size, sum_size;
    int err = ncmpi_inq_malloc_size(&malloc_size);
    if (err == NC_NOERR) {
        MPI_Reduce(&malloc_size, &sum_size, 1, MPI_OFFSET, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank == 0 && sum_size > 0)
            printf("heap memory allocated by PnetCDF internally has %lld bytes yet to be freed\n",
                   sum_size);
    }
	farb_finalize();
    MPI_Finalize();
    return 0;
}