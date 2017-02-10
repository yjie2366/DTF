#ifndef VARS_H_INCLUDED
#define VARS_H_INCLUDED

typedef unsigned char byte;

#define VERBOSE_ERROR_LEVEL   0
#define VERBOSE_WARNING_LEVEL 1
#define VERBOSE_DBG_LEVEL     2
#define VERBOSE_ALL_LEVEL     3

#define DTF_TIMEOUT       1200 /* it's long because letkf and obs are hanging for a long time before they get data from scale*/
#define DTF_UNDEFINED      -1

#include <string.h>
#include <mpi.h>
#include <assert.h>

extern struct file_buffer* gl_filebuf_list;        /*List of all file buffers*/
extern struct component *gl_comps;                 /*List of components*/
extern int gl_my_comp_id;                          /*Id of this compoinent*/
extern int gl_ncomp;                               /*Number of components*/
extern int gl_verbose;
extern int gl_my_rank;                         /*For debug messages*/
extern struct dtf_config gl_conf;                 /*Framework settings*/
extern int frt_indexing;
extern struct stats gl_stats;
char _buff[1024];
char *gl_my_comp_name;

char error_string[1024];

#define DTF_DBG(dbg_level, ...) do{  \
    if(gl_verbose >= dbg_level){  \
                memset(_buff,0,1024);                         \
                snprintf(_buff,1024,__VA_ARGS__);             \
                fprintf(stdout, "%s %d: %s\n", gl_my_comp_name, gl_my_rank, _buff);  \
                fflush(stdout);   \
    }           \
}while(0)

#define CHECK_MPI(errcode) do{   \
        if (errcode != MPI_SUCCESS) {   \
           int length_of_error_string;  \
           MPI_Error_string(errcode, error_string, &length_of_error_string);  \
           DTF_DBG(VERBOSE_DBG_LEVEL, "error is: %s", error_string);       \
           MPI_Abort(MPI_COMM_WORLD, errcode);                              \
        }                                                                   \
} while(0)

#endif // VARS_H_INCLUDED