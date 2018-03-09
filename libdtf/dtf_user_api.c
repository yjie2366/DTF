/*
* API intended to be used in user applications
*/
#include <mpi.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>

#include "dtf.h"
#include "dtf_init_finalize.h"
#include "dtf_util.h"
#include "dtf_nbuf_io.h"
#include "dtf_req_match.h"

int lib_initialized=0;

file_info_req_q_t *gl_finfo_req_q = NULL;
file_info_t *gl_finfo_list = NULL;
dtf_msg_t *gl_out_msg_q = NULL;

struct file_buffer* gl_filebuf_list = NULL;        /*List of all file buffers*/
struct fname_pattern *gl_fname_ptrns = NULL;    /*Patterns for file name*/
struct component *gl_comps = NULL;                 /*List of components*/
int gl_my_comp_id;                          /*Id of this compoinent*/
int gl_ncomp;                               /*Number of components*/
int gl_verbose;
int gl_my_rank;                         /*For debug messages*/
int gl_scale;
struct dtf_config gl_conf;                 /*Framework settings*/
struct stats gl_stats;
char *gl_my_comp_name = NULL;
void* gl_msg_buf = NULL;

/**
  @brief	Function to initialize the library. Should be called from inside
            the application before any other call to the library. Should be called
            after the MPI is initialized.
  @param	filename        Name of the library configuration file.
  @param    module_name     Name of the module calling the init function.
  @return	int             0 if OK, anything else otherwise

 */
_EXTERN_C_ int dtf_init(const char *filename, char *module_name)
{
  //  char* conf_filepath;
    int err, mpi_initialized;
    char* s;
    int verbose;

    if(lib_initialized)
        return 0;

    MPI_Initialized(&mpi_initialized);

    if(!mpi_initialized){
        fprintf(stderr, "DTF Error: dtf cannot be initialized before MPI is initialized. Aborting...\n");
        fflush(stdout);
        fflush(stderr);
        exit(1);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &gl_my_rank);

    if(strlen(module_name)>MAX_COMP_NAME){
        fprintf(stderr, "DTF Error: module name %s too long\n", module_name);
        fflush(stderr);
        exit(1);
    }

    gl_stats.malloc_size = 0;
    gl_stats.data_msg_sz = 0;
    gl_stats.ndata_msg_sent = 0;
    gl_stats.transfer_time = 0;
    gl_stats.ndb_match = 0;
    gl_stats.walltime = MPI_Wtime();
    gl_stats.t_comm = 0;
    gl_stats.t_hdr = 0;
    gl_stats.nprogress_call = 0;
    gl_stats.nioreqs = 0;
    gl_stats.nbl = 0;
    gl_stats.ngetputcall = 0;
    gl_stats.timer_accum = 0;
    gl_stats.timer_start = 0;
    gl_stats.accum_dbuff_sz = 0;
    gl_stats.accum_dbuff_time = 0;
    gl_stats.t_rw_var = 0;
    gl_stats.t_progr_comm = 0;
    gl_stats.t_do_match = 0;
    gl_stats.nfiles = 0;
    gl_stats.idle_time = 0;
    gl_stats.idle_do_match_time = 0;
    gl_stats.master_time = 0;
    gl_stats.iodb_nioreqs = 0;
    gl_stats.parse_ioreq_time = 0;
    gl_stats.st_mtch_hist = 0;
    gl_stats.st_mtch_rest = 0;
    gl_stats.t_open_hist = 0;
    gl_stats.t_open_rest = 0;
    gl_stats.t_mtch_hist = 0;
    gl_stats.t_mtch_rest = 0;
    gl_stats.t_progr_transf = 0;
    gl_stats.cnt_progr_transf = 0;

    gl_my_comp_name = (char*)dtf_malloc(MAX_COMP_NAME);
    assert(gl_my_comp_name != NULL);
    strcpy(gl_my_comp_name, module_name);

    s = getenv("DTF_VERBOSE_LEVEL");
    if(s == NULL)
        gl_verbose = VERBOSE_ERROR_LEVEL;
    else
        gl_verbose = atoi(s);

    //during init only root will print out stuff
    if(gl_my_rank != 0){
        verbose = gl_verbose;
        gl_verbose = VERBOSE_ERROR_LEVEL;
    }

    s = getenv("DTF_SCALE");
	if(s != NULL)
		gl_scale = atoi(s);
	else
		gl_scale = 0;
		
	DTF_DBG(VERBOSE_DBG_LEVEL, "Init DTF");

    s = getenv("DTF_DETECT_OVERLAP");
    if(s == NULL)
        gl_conf.detect_overlap_flag = 0;
    else
        gl_conf.detect_overlap_flag = atoi(s);

    s = getenv("DTF_DATA_MSG_SIZE_LIMIT");
    if(s == NULL)
        gl_conf.data_msg_size_limit = DTF_DATA_MSG_SIZE_LIMIT;
    else
        gl_conf.data_msg_size_limit = atoi(s) * 1024;
        
    
	s = getenv("DTF_IODB_RANGE");
    if(s == NULL)
        gl_conf.iodb_range = -1;
    else
        gl_conf.iodb_range = (MPI_Offset)atoi(s);
        
	DTF_DBG(VERBOSE_DBG_LEVEL, "Data message size limit set to %d", gl_conf.data_msg_size_limit);

    assert(gl_conf.data_msg_size_limit > 0);

    gl_msg_buf = NULL;
    gl_fname_ptrns = NULL;
    gl_filebuf_list = NULL;
    gl_finfo_req_q = NULL;
    gl_out_msg_q = NULL;
	
    /*Parse ini file and initialize components*/
    err = load_config(filename, module_name);
    if(err) goto panic_exit;

    /*Establish intercommunicators between components*/
    err = init_comp_comm();
    if(err) goto panic_exit;

    lib_initialized = 1;

    //enable print setting for other ranks again
    if(gl_my_rank != 0)
        gl_verbose = verbose;

    DTF_DBG(VERBOSE_DBG_LEVEL, "DTF: Finished initializing");

    return 0;

panic_exit:

    dtf_finalize();
    MPI_Abort(MPI_COMM_WORLD, MPI_ERR_OTHER);
    return 1;
}

/**
  @brief	Function to finalize the library. Should be called from inside
            the application before the MPI is finalized.
  @return	int             0 if OK, anything else otherwise

 */
_EXTERN_C_ int dtf_finalize()
{
    int mpi_initialized, err;
    file_info_t *finfo;

    if(!lib_initialized) return 0;

    MPI_Initialized(&mpi_initialized);

    if(!mpi_initialized){
        DTF_DBG(VERBOSE_ERROR_LEVEL,   "DTF Error: dtf cannot be finalized after MPI is finalized. Aborting...");
        fflush(stdout);
        fflush(stderr);
        exit(1);
    }

//int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
//               MPI_Op op, int root, MPI_Comm comm)

	DTF_DBG(VERBOSE_ERROR_LEVEL,"time_stamp DTF: finalize");
	MPI_Barrier(gl_comps[gl_my_comp_id].comm);
    gl_stats.st_fin = MPI_Wtime() - gl_stats.walltime;
	
    /*Send any unsent file notifications
      and delete buf files*/
    if(gl_out_msg_q != NULL){
		DTF_DBG(VERBOSE_ERROR_LEVEL, "Send msg queue not empty:");
		dtf_msg_t *msg = gl_out_msg_q;
		while(msg != NULL){
			//DTF_DBG(VERBOSE_DBG_LEVEL, "%p", (void*)msg);
			DTF_DBG(VERBOSE_ERROR_LEVEL, "%d", msg->tag);
			msg = msg->next;
		}
		while(gl_out_msg_q != NULL)
			progress_send_queue();
	}
		
    finalize_files();
    DTF_DBG(VERBOSE_ERROR_LEVEL, "1");
    
    if(gl_out_msg_q != NULL)
		DTF_DBG(VERBOSE_ERROR_LEVEL, "Finalize message queue");
	
    while(gl_out_msg_q != NULL)
		progress_send_queue();
 DTF_DBG(VERBOSE_ERROR_LEVEL, "1");
    assert(gl_finfo_req_q == NULL);
   
	finfo = gl_finfo_list;
	while(finfo != NULL){
		gl_finfo_list = gl_finfo_list->next;
		dtf_free(finfo, sizeof(file_info_t));
		finfo = gl_finfo_list;
	}
	
    finalize_comp_comm();
    print_stats();
    //destroy inrracomp communicator
    err = MPI_Comm_free(&gl_comps[gl_my_comp_id].comm);
    CHECK_MPI(err);
 DTF_DBG(VERBOSE_ERROR_LEVEL, "1");

    clean_config();

    if(gl_msg_buf != NULL)
        dtf_free(gl_msg_buf, gl_conf.data_msg_size_limit);

  //  if(gl_stats.malloc_size != MAX_COMP_NAME )
    //  DTF_DBG(VERBOSE_ERROR_LEVEL, "DTF STAT: DTF memory leak size: %lu", gl_stats.malloc_size - MAX_COMP_NAME);
    assert(gl_finfo_req_q == NULL);
    assert(gl_out_msg_q == NULL);


	DTF_DBG(VERBOSE_DBG_LEVEL,"DTF: finalized");
    dtf_free(gl_my_comp_name, MAX_COMP_NAME);
    lib_initialized = 0;
    fflush(stdout);
    fflush(stderr);
    return 0;
}

_EXTERN_C_ int dtf_transfer_v2(const char *filename, int ncid, int it )
{
	char *s = getenv("DTF_IGNORE_ITER");
	if(it < 0)
		return dtf_transfer(filename, ncid);
		
	if(s != NULL){
		if(it > atoi(s)){
			DTF_DBG(VERBOSE_DBG_LEVEL, "Match io call for iter %d", it);
			return dtf_transfer(filename, ncid);
		} else 
			DTF_DBG(VERBOSE_DBG_LEVEL, "Ignore match io call for iter %d", it);
	} else 
		return dtf_transfer(filename, ncid);
    return 0;
}

/*
 * Start non-blocking data transfer. Processes will try to progress
 * with data transfer as much as they can but if there is no more work 
 * to do at the moment, the process will simply return. 
 * The user needs to eventually call dtf_transfer_complete or 
 * dtf_transfer_complete_all to ensure the completion of the data tranfer.
 * 
 * NOTE: The user cannot call dtf_transfer_start or dtf_transfer for the 
 * same file if there is already an active data transfer
 * */
//_EXTERN_C_ int dtf_transfer_start(const char *filename)
//{
	
//}

/*
 * Process will block until the data transfer for this file is completed.
 * */
_EXTERN_C_ int dtf_transfer_complete(const char *filename, int ncid)
{
	return 0;
}

/*
 * Process will block until all active data transfers are completed.
 * */
_EXTERN_C_ int dtf_transfer_complete_all()
{
	return 0;
}


/*called by user to do explicit matching*/
/*
    User must specify either filename or ncid.
*/

_EXTERN_C_ int dtf_transfer(const char *filename, int ncid)
{
    file_buffer_t *fbuf;
	double t_start = MPI_Wtime();
	
    if(!lib_initialized) return 0;
    DTF_DBG(VERBOSE_DBG_LEVEL, "call match io for %s (ncid %d)", filename, ncid);
  
    fbuf = find_file_buffer(gl_filebuf_list, filename, ncid);
    if(fbuf == NULL){

        if( (filename != NULL) && (strlen(filename) == 0) )
            DTF_DBG(VERBOSE_ERROR_LEVEL, "DTF Warning: file (%s) with ncid %d is not treated by DTF (not in configuration file). Matching ignored.", filename, ncid);
        else
            DTF_DBG(VERBOSE_ERROR_LEVEL, "DTF Warning: file %s (ncid %d) is not treated by DTF (not in configuration file). Matching ignored.", filename, ncid);
        return 0;
    }
    if((fbuf->ioreq_log != NULL) && gl_conf.do_checksum){
		io_req_log_t *ioreq = fbuf->ioreq_log;
		while(ioreq != NULL){
			
			if(ioreq->dtype == MPI_DOUBLE || ioreq->dtype == MPI_FLOAT){
				double checksum = compute_checksum(ioreq->user_buf, ioreq->ndims, ioreq->count, ioreq->dtype);
				if(ioreq->rw_flag == DTF_READ)
					DTF_DBG(VERBOSE_ERROR_LEVEL, "read req %d, checksum %.4f", ioreq->id, checksum);
				else
					DTF_DBG(VERBOSE_ERROR_LEVEL, "write req %d, checksum %.4f", ioreq->id, checksum);
			}
			
			//delete
			fbuf->ioreq_log = ioreq->next;
			
			if(ioreq->rw_flag == DTF_READ)
				fbuf->rreq_cnt--;
			else
				fbuf->wreq_cnt--;
			
			if(gl_conf.buffered_req_match && (ioreq->rw_flag == DTF_WRITE)) dtf_free(ioreq->user_buf, ioreq->user_buf_sz);
			if(ioreq->start != NULL) dtf_free(ioreq->start, ioreq->ndims*sizeof(MPI_Offset));
			if(ioreq->count != NULL)dtf_free(ioreq->count, ioreq->ndims*sizeof(MPI_Offset));
			dtf_free(ioreq, sizeof(io_req_log_t));
			ioreq = fbuf->ioreq_log;
		}
	}
	
	if(gl_scale && (fbuf->iomode == DTF_IO_MODE_FILE) && (fbuf->writer_id == gl_my_comp_id)){

		fbuf->is_ready = 1;	
		//Check for any incoming messages
		progress_comm();
		if(fbuf->fready_notify_flag == RDR_NOT_NOTIFIED){
			if(fbuf->root_writer == gl_my_rank){
				while(fbuf->root_reader == -1)
					progress_comm();
				notify_file_ready(fbuf);
			}
		}
	}
	 
    if(fbuf->iomode != DTF_IO_MODE_MEMORY) return 0;

    dtf_tstart();
    match_ioreqs(fbuf);
    dtf_tend();
    
    gl_stats.transfer_time += MPI_Wtime() - t_start;
    return 0;
}

_EXTERN_C_ void dtf_print(const char *str)
{
    if(!lib_initialized) return;
    DTF_DBG(VERBOSE_ERROR_LEVEL, "%s", str);
}


/*User controled timers*/
_EXTERN_C_ void dtf_time_start()
{
    if(!lib_initialized) return;
  
    gl_stats.user_timer_start = MPI_Wtime();
    DTF_DBG(VERBOSE_DBG_LEVEL, "user_time start");
    
    
}
_EXTERN_C_ void dtf_time_end()
{
    double tt;
    if(!lib_initialized) return;
    tt = MPI_Wtime() - gl_stats.user_timer_start;
    gl_stats.user_timer_accum += tt;
    gl_stats.user_timer_start = 0;
    
	DTF_DBG(VERBOSE_DBG_LEVEL, "user_time end  %.6f", tt);
    
 //   DTF_DBG(VERBOSE_DBG_LEVEL, "time_stat: user time %.4f", tt);
}

/************************************************  Fortran Interfaces  *********************************************************/

_EXTERN_C_ void dtf_time_start_()
{
    dtf_time_start();
}
_EXTERN_C_ void dtf_time_end_()
{
    dtf_time_end();
}

void dtf_init_(const char *filename, char *module_name, int* ierr)
{
    *ierr = dtf_init(filename, module_name);
}

void dtf_finalize_(int* ierr)
{
    *ierr = dtf_finalize();
}

void dtf_transfer_(const char *filename, int *ncid, int *ierr)
{
    *ierr = dtf_transfer(filename, *ncid);
}

void dtf_transfer_complete_(const char *filename, int *ncid)
{
	dtf_transfer_complete(filename, *ncid);
}

void dtf_transfer_complete_all_()
{
	dtf_transfer_complete_all();
}

void dtf_print_(const char *str)
{
    dtf_print(str);
}

void dtf_print_data_(int *varid, int *dtype, int *ndims, MPI_Offset* count, void* data)
{
    dtf_print_data(*varid, *dtype, *ndims, count, data);
}

