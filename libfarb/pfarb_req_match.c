#include "pfarb_req_match.h"
#include "pfarb_common.h"
#include "pfarb_util.h"
#include "pfarb_mem.h"
#include "pfarb.h"

static void init_master_db(file_buffer_t *fbuf)
{
    fbuf->iodb = malloc(sizeof(struct master_db));
    assert(fbuf->iodb != NULL);
    fbuf->iodb->ritems = NULL;
    fbuf->iodb->witems = NULL;
    fbuf->iodb->nritems = 0;
    fbuf->iodb->nranks_completed = 0;
    fbuf->iodb->nmst_completed = 0;
}

static void print_read_dbitem(read_db_item_t *dbitem)
{
    read_chunk_rec_t *tmp;
    FARB_DBG(VERBOSE_ALL_LEVEL, "Updated read dbitem for rank %d. %d chunks:", dbitem->rank, (int)dbitem->nchunks);
    tmp = dbitem->chunks;
    while(tmp != NULL){
        FARB_DBG(VERBOSE_ALL_LEVEL, "       (var %d, %d, %d)", tmp->var_id, (int)tmp->offset, (int)tmp->data_sz);
        tmp = tmp->next;
    }
}

static void print_write_dbitem(write_db_item_t *dbitem)
{
    write_chunk_rec_t *tmp;
    FARB_DBG(VERBOSE_ALL_LEVEL, "Write dbitem for var %d.", dbitem->var_id);
    tmp = dbitem->chunks;
    while(tmp != NULL){
        FARB_DBG(VERBOSE_ALL_LEVEL, "       (r %d, off %llu, sz %llu)", tmp->rank, tmp->offset, tmp->data_sz);
        tmp = tmp->next;
    }
}

io_req_t *find_io_req(io_req_t *list, int var_id)
{
    io_req_t *ioreq = list;
    while(ioreq != NULL){
        if(ioreq->var_id == var_id)
            break;
        ioreq = ioreq->next;
    }
    return ioreq;
}

static void pack_vars(file_buffer_t *fbuf, int *bufsz, void **buf)
{
    farb_var_t *var;
    size_t sz = 0, offt = 0;

    sz = sizeof(fbuf->ncid) +sizeof(fbuf->var_cnt);
    var = fbuf->vars;
    while(var != NULL){
        sz += sizeof(var->id) + sizeof(var->el_sz) + sizeof(var->ndims) + sizeof(MPI_Offset)*var->ndims;
        var = var->next;
    }

    FARB_DBG(VERBOSE_DBG_LEVEL, "Packing vars: sz %lu", sz);

    *buf = malloc(sz);
    assert(*buf != NULL);
//    memcpy(*buf, fbuf->file_path, MAX_FILE_NAME);
//    offt += MAX_FILE_NAME;

    *((int*)(*buf+offt)) = fbuf->ncid;
    offt += sizeof(int);

    *((int*)(*buf+offt)) = fbuf->var_cnt;
    offt += sizeof(int);

    var = fbuf->vars;
    while(var != NULL){
        *((int*)(*buf+offt)) = var->id;
        offt += sizeof(int);
        *((MPI_Offset*)(*buf+offt)) = var->el_sz;
        offt += sizeof(MPI_Offset);
        *((int*)(*buf+offt)) = var->ndims;
        offt += sizeof(int);
        memcpy(*buf+offt, (void*)var->shape, sizeof(MPI_Offset)*var->ndims);
        offt += sizeof(MPI_Offset)*var->ndims;

        var = var->next;
    }
    FARB_DBG(VERBOSE_ALL_LEVEL, "offt %lu", offt);
    assert(offt == sz);
    *bufsz = (int)sz;

}

static void unpack_vars(file_buffer_t *fbuf, int bufsz, void *buf)
{
    int i, varid, var_cnt;
    farb_var_t *var;
    size_t offt = 0;
   // char filename[MAX_FILE_NAME];

    fbuf->ncid = *((int*)buf);
    assert(fbuf->ncid > -1);
    offt += sizeof(int);

    var_cnt = *((int*)(buf+offt));
    offt += sizeof(int);
    FARB_DBG(VERBOSE_DBG_LEVEL, "unpack nvars %d", var_cnt);

    for(i = 0; i < var_cnt; i++){
        varid = *((int*)(buf+offt));
        offt += sizeof(int);
        var = find_var(fbuf->vars, varid);
        assert(var == NULL);
        var = new_var(varid, 0, 0, NULL);
        var->el_sz = *((MPI_Offset*)(buf+offt));
        offt+=sizeof(MPI_Offset);
        var->ndims = *((int*)(buf+offt));
        offt += sizeof(int);

        if(var->ndims > 0){
            var->shape = malloc(var->ndims*sizeof(MPI_Offset));
            assert(var->shape != NULL);
            memcpy((void*)var->shape, buf+offt, sizeof(MPI_Offset)*var->ndims);
            offt += sizeof(MPI_Offset)*var->ndims;
        } else
            var->shape = NULL;

        add_var(&(fbuf->vars), var);
        fbuf->var_cnt++;
    }
    FARB_DBG(VERBOSE_ALL_LEVEL, "Finished unpacking");
}

//static void add_io_db_item(io_db_item_t **list, io_db_item_t *item)
//{
//    if(*list == NULL){
//        *list = item;
//        return;
//    }
//
//    io_db_item_t *tmp = *list;
//    while( (tmp->offset < item->offset) && (tmp->next != NULL)){
//        tmp = tmp->next;
//    }
//
//    if(tmp->offset < item->offset){
//        item->next = tmp->next;
//        tmp->next = item;
//    } else {
//        item->next = tmp;
//        tmp = item;
//    }
//}

static void do_matching(file_buffer_t *fbuf)
{
    int mlc_ranks = 5;
    int mlc_buf   = 512;
    int i;
    read_chunk_rec_t  *rchunk;
    write_chunk_rec_t *wchunk;
    int rank_idx, var_id, nwriters;
    MPI_Offset matchsz, start_offt;
    write_db_item_t *witem = NULL;
    read_db_item_t *ritem = fbuf->iodb->ritems;
    int *writers;
    void **sbuf;
    int *bufsz;
    size_t *offt;
    /*Try to match as many read and write
    requests as we can*/
    while(ritem != NULL){

        writers = (int*)malloc(mlc_ranks*sizeof(int));
        assert(writers != NULL);
        sbuf = (void**)malloc(mlc_ranks*sizeof(void*));
        assert(sbuf != NULL);
        bufsz = (int*)malloc(mlc_ranks*sizeof(int));
        assert(bufsz != NULL);
        offt = (size_t*)malloc(mlc_ranks*sizeof(size_t));
        assert(offt != NULL);
        nwriters = 0;

        for(i = 0; i < mlc_ranks; i++){
            sbuf[i] = NULL;
            bufsz[i] = 0;
            offt[i] = 0;
        }

        rchunk = ritem->chunks;
        while(rchunk != NULL){
            var_id = rchunk->var_id;

            if( (witem == NULL) || (witem->var_id != var_id)){
                //find the write record for this var_id
                witem = fbuf->iodb->witems;
                while(witem != NULL){
                    if(witem->var_id == var_id)
                        break;
                    witem = witem->next;
                }
                if(witem == NULL)
                    /*No match right now*/
                    continue;
            }
            matchsz = 0;
            //TODO any better way than comparing each rchunk with each wchunk every time?
            wchunk = witem->chunks;
            while(wchunk != NULL){

                if( (rchunk->offset >= wchunk->offset) && (rchunk->offset < wchunk->offset + wchunk->data_sz)){
                    start_offt = rchunk->offset;
                    if( start_offt + rchunk->data_sz <= wchunk->offset + wchunk->data_sz)
                        matchsz = rchunk->data_sz;
                    else
                        matchsz = wchunk->offset + wchunk->data_sz - start_offt;
                } else if( (wchunk->offset > rchunk->offset) && (wchunk->offset < rchunk->offset + rchunk->data_sz)){
                    start_offt = wchunk->offset;

                    if(wchunk->offset + wchunk->data_sz >= rchunk->offset + rchunk->data_sz)
                        matchsz = rchunk->offset + rchunk->data_sz - start_offt;
                    else
                        matchsz = wchunk->data_sz;
                } else {
                    wchunk = wchunk->next;
                    continue;
                }

                /*Find send buffer for this rank*/
                if(nwriters == 0){
                    writers[nwriters] = wchunk->rank;
                    rank_idx = 0;
                    nwriters++;
                } else {
                    for(i = 0; i < nwriters; i++)
                        if(writers[i] == wchunk->rank)
                            break;
                    if(i == nwriters){
                        /*add new send buffer*/
                        if(nwriters % mlc_ranks == 0 ){
                            void *tmp;
                            void **tmp1;
                            tmp = realloc((void*)writers, (nwriters+mlc_ranks)*sizeof(int));
                            assert(tmp != NULL);
                            writers = (int*)tmp;

                            tmp = realloc((void*)offt, (nwriters+mlc_ranks)*sizeof(size_t));
                            assert(tmp != NULL);
                            offt = (size_t*)tmp;

                            tmp = realloc((void*)bufsz, (nwriters+mlc_ranks)*sizeof(int));
                            assert(tmp != NULL);
                            bufsz = (int*)tmp;

                            tmp1 = realloc(sbuf, (nwriters+mlc_ranks)*sizeof(void*));
                            assert(tmp1 != NULL);
                            sbuf = tmp1;
                        }
                        writers[nwriters] = wchunk->rank;
                        offt[nwriters] = 0;
                        bufsz[nwriters] = 0;
                        sbuf[nwriters] = NULL;
                        rank_idx = nwriters;
                        nwriters++;
                    } else
                        rank_idx = i;
                }

                FARB_DBG(VERBOSE_ALL_LEVEL, "rank idx %d, nranks %d", rank_idx, nwriters);
                if(offt[rank_idx] + sizeof(int) + sizeof(MPI_Offset)*2 > bufsz[rank_idx]){
                    void *tmp;
                    tmp = realloc(sbuf[rank_idx], bufsz[rank_idx] + mlc_buf);
                    assert(tmp != NULL);
                    sbuf[rank_idx] = tmp;
                    bufsz[rank_idx] += mlc_buf;
                    if(offt[rank_idx] == 0) {
                        *(int*)(sbuf[rank_idx] + offt[rank_idx]) = ritem->rank; //to whom writer should send the data
                        offt[rank_idx] += sizeof(int);
                        *(int*)(sbuf[rank_idx] + offt[rank_idx]) = fbuf->ncid;  //data from what file
                        offt[rank_idx] += sizeof(int);
                    }
                }

                *(int*)(sbuf[rank_idx] + offt[rank_idx]) = rchunk->var_id;
                offt[rank_idx] += sizeof(int);
                *(MPI_Offset*)(sbuf[rank_idx] + offt[rank_idx]) = start_offt;
                offt[rank_idx] += sizeof(MPI_Offset);
                *(MPI_Offset*)(sbuf[rank_idx] + offt[rank_idx]) = matchsz;
                offt[rank_idx] += sizeof(MPI_Offset);

                FARB_DBG(VERBOSE_ALL_LEVEL, "Will ask %d for data (%d, %d)", wchunk->rank, (int)start_offt, (int)matchsz);
                /*Check if all the data of this chunk has been matched*/
                if(matchsz == rchunk->data_sz){
                    break;
                } else if(matchsz > 0){
                    /* For now, we managed to match only a part of memory chunk*/
                    FARB_DBG(VERBOSE_ALL_LEVEL, "Matched part of req: (%d, %d) of (%d, %d)", (int)start_offt, (int)matchsz, (int)rchunk->offset, (int)rchunk->data_sz);
                    if(start_offt == rchunk->offset){
                        rchunk->offset = start_offt + matchsz;
                        rchunk->data_sz -= matchsz;
                    } else if(start_offt + matchsz == rchunk->offset + rchunk->data_sz){
                        rchunk->data_sz -= matchsz;
                    } else {
                        FARB_DBG(VERBOSE_ALL_LEVEL, "Matched in the middle of a chunk");
                        /*We matched data in the middle of the chunk,
                        hence, we have to split it into 2 chunks now*/
                        read_chunk_rec_t *chunk = malloc(sizeof(read_chunk_rec_t));
                        assert(chunk != NULL);
                        chunk->var_id = rchunk->var_id;
                        chunk->offset = start_offt + matchsz;
                        chunk->data_sz = rchunk->offset + rchunk->data_sz - chunk->offset;
                        rchunk->data_sz -= matchsz + chunk->data_sz;
                        /*Instert the new chunk and skip it*/
                        chunk->next = rchunk->next;
                        if(chunk->next != NULL)
                            chunk->next->prev = chunk;
                        rchunk->next = chunk;
                        chunk->prev = rchunk;
                        ritem->nchunks++;
                    }
                }
                wchunk = wchunk->next;
            }
            if(matchsz == rchunk->data_sz){
                FARB_DBG(VERBOSE_ALL_LEVEL, "Matched all chunk (%d, %d). Will delete (nchunk %d)", (int)rchunk->offset, (int)rchunk->data_sz, (int)ritem->nchunks);
                /*delete this chunk*/
                read_chunk_rec_t *tmp = rchunk;
                if(rchunk == ritem->chunks)
                    ritem->chunks = ritem->chunks->next;
                if(rchunk->next != NULL)
                    rchunk->next->prev = NULL;
                if(rchunk->prev != NULL)
                    rchunk->prev->next = rchunk->next;

                rchunk = rchunk->next;
                free(tmp);
                ritem->nchunks--;
                continue;
            }
            rchunk = rchunk->next;
        }
        if(nwriters > 0){
            int errno;
            int completed;
            MPI_Request *sreqs = (MPI_Request*)malloc(nwriters*sizeof(MPI_Request));
            assert(sreqs != NULL);

            for(i = 0; i < nwriters; i++){
                FARB_DBG(VERBOSE_DBG_LEVEL, "Send data req to wrt %d", writers[i]);
                errno = MPI_Isend(sbuf[i], offt[i], MPI_BYTE, writers[i], IO_DATA_REQ_TAG, gl_comps[gl_my_comp_id].intercomm, &sreqs[i]);
                CHECK_MPI(errno);
            }

            completed = 0;
            while(!completed){
                progress_io_matching();
                errno = MPI_Testall(nwriters, sreqs, &completed, MPI_STATUSES_IGNORE);
                CHECK_MPI(errno);
            }
            free(sreqs);
            for(i = 0; i < nwriters; i++)
                free(sbuf[i]);

            FARB_DBG(VERBOSE_ALL_LEVEL, "Matched rreq for rank %d", ritem->rank);
            print_read_dbitem(ritem);

        }

        /*If we matched all chunks for this rank, then delete this ritem*/
        if(ritem->nchunks == 0){
            read_db_item_t *tmp = ritem;
            FARB_DBG(VERBOSE_DBG_LEVEL, "Delete db read item of rank %d (left ritems %d)", ritem->rank,  (int)(fbuf->iodb->nritems - 1));
            if(ritem == fbuf->iodb->ritems)
                fbuf->iodb->ritems = fbuf->iodb->ritems->next;
            if(ritem->prev != NULL)
                ritem->prev->next = ritem->next;
            if(ritem->next != NULL)
                ritem->next->prev = ritem->prev;

            ritem = ritem->next;
            free(tmp);
            fbuf->iodb->nritems--;
        } else {
            FARB_DBG(VERBOSE_ALL_LEVEL, "Not all chunks for rreq from rank %d have been matched (%d left)", ritem->rank, (int)ritem->nchunks);
            ritem = ritem->next;
        }

        free(writers);
        free(offt);
        free(bufsz);
        free(sbuf);
    }
}

void clean_iodb(master_db_t *iodb)
{
    write_db_item_t *witem;
    read_db_item_t *ritem;

    write_chunk_rec_t *wrec;
    read_chunk_rec_t *rrec;

    witem = iodb->witems;
    while(witem != NULL){
        wrec = witem->chunks;
        while(wrec != NULL){
            witem->chunks = witem->chunks->next;
            free(wrec);
            wrec = witem->chunks;
        }

        iodb->witems = iodb->witems->next;
        free(witem);
        witem = iodb->witems;
    }

    ritem = iodb->ritems;
    while(ritem != NULL){
        rrec = ritem->chunks;
        while(rrec != NULL){
            ritem->chunks = ritem->chunks->next;
            free(rrec);
            rrec = ritem->chunks;
        }

        iodb->ritems = iodb->ritems->next;
        free(ritem);
        ritem = iodb->ritems;
    }

    iodb->witems = NULL;
    iodb->ritems = NULL;
    iodb->nritems = 0;
    iodb->nranks_completed = 0;
}

///*Add the list of write requests received from rank*/
//static void update_ioreq_db(int rank, int bufsz, void* buf)
//{
//    size_t offt = 0;
//    int var_id, ncid;
//    MPI_Offset offset, chunk_sz;
//    io_db_item_t *dbitem;
//    file_buffer_t *fbuf;
//
//    ncid = *(int*)(data);
//    offt += sizeof(int);
//
//    file_buffer_t *fbuf = find_file_buffer(gl_filebuf_list, NULL, ncid);
//    assert(fbuf != NULL);
//
//    var_id = *(int*)(data + offt);
//    offt += sizeof(int);
//    var = find_var(fbuf->vars, var_id);
//    assert(var != NULL);
//
//    offset = *(MPI_Offset*)(data + offt);
//    offt += sizeof(MPI_Offset);
//    chunk_sz = *(MPI_Offset*)(data + offt);
//    offt += sizeof(MPI_Offset);
//    //TODO next
//    db_item = new_io_db_item(offset, chunk_sz, rank);
//    add_io_db_item(&(var->io_db), db_item);
//
//    while(buf_offt != datasz){
//        var_id = *(int*)(data);
//        buf_offt += sizeof(int);
//        if(var_id != var->id){
//            var = find_var(fbuf->vars, var_id);
//            assert(var != NULL);
//        }
//        offset = *(MPI_Offset*)(data + buf_offt);
//        buf_offt += sizeof(MPI_Offset);
//        chunk_sz = *(MPI_Offset*)(data + buf_offt);
//        buf_offt += sizeof(MPI_Offset);
//
//        db_item = new_io_db_item(offset, chunk_sz, rank);
//        insert_io_db_item(&(var->io_db), db_item);
//    }
//    print_io_db(fbuf);
//
//    do_matching(fbuf);
//}

//static void send_ioreq_list(file_buffer_t *fbuf)
//{
//    int i;
//    farb_var_t *var;
//    io_req_t *ioreq;
//    MPI_Offset last_offt, offt_range, chunk_offt;
//    contig_mem_chunk_t *chunk;
//    int mst, mst2, err;
//    int nelems;
//    MPI_Offset chunksz;
//    MPI_Request *sreqs;
//    int completed = 0;
//
//    void **sbuf = (void**)malloc(gl_conf.nmasters*sizeof(void*));
//    assert(sbuf != NULL);
//    int *sbufsz = (int*)malloc(gl_conf.nmasters*sizeof(int));
//    assert(sbufsz != NULL);
//    int *sbuf_offt = (int*)malloc(gl_conf.nmasters*sizeof(int));
//    assert(sbuf_offt != NULL);
//    memset(sbufsz, 0, gl_conf.nmasters*sizeof(int));
//    memset(sbuf_offt, 0, gl_conf.nmasters*sizeof(int));
//
//    for(i = 0; i < gl_conf.nmasters; i++)
//        sbuf[i] = NULL;
//
//    ioreq = fbuf->ioreqs;
//
//    while(ioreq != NULL){
//        if(ioreq->rw_flag != FARB_WRITE){
//            ioreq = ioreq->next;
//            continue;
//        }
//        var = find_var(fbuf->vars, ioreq->var_id);
//        assert(var != NULL);
//        /*Find out the range of offsets each master holds data about*/
//        last_offt = to_1d_index(var->ndims, var->shape, var->shape)*var->el_sz;
//        offt_range = (MPI_Offset)(last_offt/gl_conf.nmasters);
//        FARB_DBG(VERBOSE_DBG_LEVEL, "Offset range for var %d is %d", var->id, (int)offt_range);
//
//        /*Build a list of (offt, data_sz) tuples that will be sent to
//          corresponding masters*/
//        assert(ioreq->mem_chunks != NULL);
//        chunk = ioreq->mem_chunks;
//        while(chunk != NULL){
//            chunk_offt = chunk->offset;
//            mst = (int)(chunk_offt / offt_range);
//            if(mst >= gl_conf.nmasters)
//                mst = gl_conf.nmasters - 1;
//            mst2 = (int)((chunk->offset + chunk->data_sz)/offt_range);
//            if(mst2 >= gl_conf.nmasters)
//                mst2 = gl_conf.nmasters - 1;
//
//            while(chunk_offt != chunk->offset + chunk->data_sz){
//                /*check if whole memory chunk should be
//                sent to the same master*/
//
//                if(mst2 != mst ) //chunk will have to be split
//                    chunksz = (mst+1)*offt_range - chunk_offt;
//                else
//                    chunksz = chunk->offset + chunk->data_sz - chunk_offt;
//
//                if(sbuf[mst] == NULL){
//                    sbuf[mst] = malloc(1024);
//                    assert(sbuf[mst] != NULL);
//                    sbufsz[mst] = 1024;
//                    *((int*)sbuf[mst]) = fbuf->ncid;
//                    sbuf_offt[mst] += sizeof(int);
//                }else if(sbufsz[mst] - sbuf_offt[mst] <= (int)(sizeof(int)+sizeof(MPI_Offset)*2)){
//                    void* tmp = realloc(sbuf[mst], sbufsz[mst]+1024);
//                    assert(tmp != NULL);
//                    sbuf[mst] = tmp;
//                    sbufsz[mst] += 1024;
//                }
//                FARB_DBG(VERBOSE_ALL_LEVEL, "var %d, chunk offt %d, sz %d", var->id, (int)chunk_offt, (int)chunksz);
//                *(int*)(sbuf[mst] + sbuf_offt[mst]) = var->id;
//                sbuf_offt[mst] += sizeof(int);
//                *(MPI_Offset*)(sbuf[mst] + sbuf_offt[mst]) = chunk_offt;
//                sbuf_offt[mst] += sizeof(MPI_Offset);
//                *(MPI_Offset*)(sbuf[mst] + sbuf_offt[mst]) = chunksz;
//                sbuf_offt[mst] += sizeof(MPI_Offset);
//                chunk_offt += chunksz;
//                mst++;
//            }
//            chunk = chunk->next;
//        }
//        ioreq = ioreq->next;
//    }
//    sreqs = (MPI_Request*)malloc(gl_conf.nmasters*sizeof(MPI_Request));
//    assert(sreqs != NULL);
//    /*Send the info to masters*/
//    for(mst = 0; mst < gl_conf.nmasters; mst++){
//        if(sbuf_offt[mst] > 0){
//            FARB_DBG(VERBOSE_ALL_LEVEL, "Send my list of sz %d to mst %d", sbuf_offt[mst], mst);
//            err = MPI_Isend(sbuf[mst], sbuf_offt[mst], MPI_BYTE, gl_conf.masters[mst], IOREQS_LIST_TAG, MPI_COMM_WORLD, &sreqs[mst]);
//            CHECK_MPI(err);
//        } else
//            sreqs[mst] = MPI_REQUEST_NULL;
//    }
//
//    while(!completed){
//        err = MPI_Testall(gl_conf.nmasters, sreqs, &completed, MPI_STATUSES_IGNORE);
//        CHECK_MPI(err);
//        progress_io_matching(fbuf->file_path);
//    }
//
//    for(mst = 0; mst < gl_conf.nmasters; mst++)
//        if(sbuf[mst] != NULL)
//            free(sbuf[mst]);
//}


static write_chunk_rec_t *new_write_chunk_rec(int rank, MPI_Offset offset, MPI_Offset data_sz)
{
    write_chunk_rec_t *chunk = malloc(sizeof(write_chunk_rec_t));
    assert(chunk != NULL);
    chunk->rank = rank;
    chunk->offset = offset;
    chunk->data_sz = data_sz;
    chunk->next = NULL;
    chunk->prev = NULL;
    return chunk;
}

/*
    update the database with io request from writer
*/
static void parse_ioreq_wrt_mst(void *buf, int bufsz, int rank)
{
    int ncid, var_id;
    file_buffer_t *fbuf;
    farb_var_t *var;
    write_db_item_t *dbitem;
    write_chunk_rec_t *new_chunk;
    int mst1, mst2, mst;
    MPI_Offset offset, data_sz, offt_range;

    size_t offt = 0;

    ncid = *((int*)buf);
    offt += sizeof(int);
    var_id = *(int*)(buf+offt);
    offt += sizeof(int);
    FARB_DBG(VERBOSE_DBG_LEVEL, "Master recv write req from %d for ncid %d, var %d", rank, ncid, var_id);

    fbuf = find_file_buffer(gl_filebuf_list, NULL, ncid);
    assert(fbuf != NULL);

    if(fbuf->iodb == NULL)
        init_master_db(fbuf);

    /*Find corresponding record in the database*/
    assert(fbuf->iodb!=NULL);
    dbitem = fbuf->iodb->witems;
    while(dbitem != NULL){
        if(dbitem->var_id == var_id)
            break;
        dbitem = dbitem->next;
    }

    if(dbitem == NULL){
        dbitem = (write_db_item_t*)malloc(sizeof(write_db_item_t));
        assert(dbitem != NULL);
        dbitem->var_id = var_id;
        dbitem->next = NULL;
        dbitem->chunks = NULL;
        dbitem->last = NULL;

        //enqueue
        if(fbuf->iodb->witems == NULL)
            fbuf->iodb->witems = dbitem;
        else{
            dbitem->next = fbuf->iodb->witems;
            fbuf->iodb->witems = dbitem;
        }
    }

    /*add mem chunks to the list for future matching.
      Pick only those mem chunks whose offsets are within
      the range I'm responsible for*/
    var = find_var(fbuf->vars, var_id);
    assert(var != NULL);
    FARB_DBG(VERBOSE_ALL_LEVEL, "last idx %d, offt %d", (int)last_1d_index(var->ndims, var->shape), (int)(last_1d_index(var->ndims, var->shape)*var->el_sz));
    offt_range = (MPI_Offset)((last_1d_index(var->ndims, var->shape) + 1)/gl_conf.nmasters);
    offt_range *= var->el_sz;
    FARB_DBG(VERBOSE_ALL_LEVEL, "offt_range %d", (int)offt_range);

    FARB_DBG(VERBOSE_ALL_LEVEL, "List before update");
    print_write_dbitem(dbitem);
    while(offt != (size_t)bufsz){

        offset = *(MPI_Offset*)(buf+offt);
        offt+=sizeof(MPI_Offset);
        data_sz = *(MPI_Offset*)(buf+offt);
        offt+=sizeof(MPI_Offset);

        if(offt_range == 0){//Number of elements in the variable is less than then number of masters
            mst1 = mst2 = 0;
        } else {
            mst1 = (int)(offset/offt_range);
            if(mst1 >= gl_conf.nmasters)
                mst1 = gl_conf.nmasters - 1;
            mst2 = (int)( (offset+data_sz-1)/offt_range);
            if(mst2 >= gl_conf.nmasters)
                mst2 = gl_conf.nmasters - 1;
        }

        FARB_DBG(VERBOSE_ALL_LEVEL, "--> (%d, %d): mst1 %d mst2 %d", (int)offset, (int)data_sz, mst1, mst2);
        if( (gl_conf.masters[mst1] != gl_my_rank) && (gl_conf.masters[mst2] != gl_my_rank))
            /*This master is not responsible for this data*/
            continue;
        else if(gl_conf.masters[mst1] != gl_my_rank){
            for(mst = 0; mst < gl_conf.nmasters; mst++)
                if(gl_conf.masters[mst] == gl_my_rank)
                    break;
            FARB_DBG(VERBOSE_DBG_LEVEL, "Changing offset from %d to %d", (int)offset, (int)(offt_range*mst));
            offset = offt_range*mst;
        } else if (gl_conf.masters[mst2] != gl_my_rank){
            for(mst = 0; mst < gl_conf.nmasters; mst++)
                if(gl_conf.masters[mst] == gl_my_rank)
                    break;
            FARB_DBG(VERBOSE_DBG_LEVEL, "Changing data_sz from %d to %d", (int)data_sz, (int)(offt_range*(mst+1) - offset));
            data_sz = offt_range*(mst+1) - offset;
        }

        new_chunk = new_write_chunk_rec(rank, offset, data_sz);

        FARB_DBG(VERBOSE_ALL_LEVEL, "new chunk (r %d, off %llu, sz %llu)", new_chunk->rank, new_chunk->offset, new_chunk->data_sz);

        if(dbitem->chunks == NULL){
            dbitem->chunks = new_chunk;
            dbitem->last = new_chunk;
        } else {
            write_chunk_rec_t *chunk;

            chunk = dbitem->chunks;
            while( (chunk != NULL) && (chunk->offset < new_chunk->offset))
                chunk = chunk->next;

            if(chunk == NULL){ //insert to the end
                /*check for overlapping*/
                if(new_chunk->offset >= dbitem->last->offset &&
                    (new_chunk->offset < dbitem->last->offset+dbitem->last->data_sz))
                    FARB_DBG(VERBOSE_ERROR_LEVEL, "FARB Warning: multiple writes of the same area (by processes %d and %d). \
                            Overwrite order is not defined. ", dbitem->last->rank, new_chunk->rank);
                dbitem->last->next = new_chunk;
                new_chunk->prev = dbitem->last;
                dbitem->last = new_chunk;
            } else {
                    /*check for overlapping*/
                    if(chunk->prev != NULL){
                        if((new_chunk->offset >= chunk->prev->offset) &&
                           (new_chunk->offset < chunk->prev->offset+chunk->prev->data_sz))
                            FARB_DBG(VERBOSE_ERROR_LEVEL, "FARB Warning: multiple writes of the same area (by processes %d and %d). \
                                    Overwrite order is not defined. ", chunk->prev->rank, new_chunk->rank);
                    }

                   if( (new_chunk->offset + new_chunk->data_sz - 1 >= chunk->offset) &&
                       (new_chunk->offset + new_chunk->data_sz - 1 < chunk->offset+chunk->data_sz))
                        FARB_DBG(VERBOSE_ERROR_LEVEL, "FARB Warning: multiple writes of the same area (by processes %d and %d). \
                                    Overwrite order is not defined. ", chunk->rank, new_chunk->rank);
                   //insert before this chunk
                   if(chunk == dbitem->chunks){
                        new_chunk->next = chunk;
                        chunk->prev = new_chunk;
                        dbitem->chunks = new_chunk;
                   } else {
                        chunk->prev->next = new_chunk;
                        new_chunk->prev = chunk->prev;
                        new_chunk->next = chunk;
                        chunk->prev = new_chunk;
                   }
            }
        }

//                if(new_chunk->offset + new_chunk->data_sz - 1 < chunk->offset){
//                    /*insert before this chunk*/
//                    if(dbitem->chunks == chunk)
//                        dbitem->chunks = new_chunk;
//                    else{
//                        new_chunk->prev = chunk->prev;
//                        new_chunk->prev->next = new_chunk;
//                    }
//                    new_chunk->next = chunk;
//                    chunk->prev = new_chunk;
//                } else { //overlap
//
//                    if(chunk->offset < new_chunk->offset){
//                        if(new_chunk->offset+new_chunk->data_sz >= chunk->offset+chunk->data_sz){ //shrink cur chunk
//                            chunk->data_sz = new_chunk->offset - chunk->offset;
//                            if(chunk->next == NULL){
//                                chunk->next = new_chunk;
//                                new_chunk->prev = chunk;
//                                dbitem->last = new_chunk;
//                            } else {
//                                /*delete all that overlap*/
//                                chunk = chunk->next;
//                                while(chunk != NULL){
//                                    if(chunk->offset+chunk->data_sz <= new_chunk->offset+new_chunk->data_sz){
//                                        write_chunk_rec_t *tmp = chunk->next;
//                                        //delete
//                                        chunk->prev->next = chunk->next;
//                                        if(chunk->next != NULL)
//                                            chunk->next->prev = chunk->prev;
//                                        else
//                                            dbitem->last = chunk->prev;
//                                        free(chunk);
//                                        chunk = tmp;
//                                    } else
//                                        break;
//                                }
//
//                                if(chunk == NULL){
//                                    dbitem->last
//                                } else {
//
//                                }
//
//
//                            }
//                        } else { //split cur chunk
//                            write_chunk_rec_t *tmp = new_write_chunk_rec(chunk->rank,
//                                                                        new_chunk->offset + new_chunk->data_sz,
//                                                                        chunk->offset + chunk->data_sz - new_chunk->offset - new_chunk->data_sz);
//                            tmp->next = chunk->next;
//                            tmp->prev = new_chunk;
//                            new_chunk->next = tmp;
//                            new_chunk->prev = chunk;
//                            chunk->data_sz = new_chunk->offset - chunk->offset;
//                            chunk->next = new_chunk;
//                            new_chunk->prev = chunk;
//                        }
//                    }
//
//                }
//            }



//            if(chunk->offset < new_chunk->offset){ //this is last chunk
//
//                if(new_chunk->offset < chunk->offset+chunk->data_sz){ //overlaps
//
//                    if(chunk->rank == new_chunk->rank){
//                        FARB_DBG(VERBOSE_ERROR_LEVEL, "FARB Warning: multiple writes of the same area issued by \
//                                 rank %d. Overwrite order is not defined. ", chunk->rank);
//
//                        if(new_chunk->offset + new_chunk->data_sz >= chunk->offset + chunk->data_sz){
//                            FARB_DBG(VERBOSE_DBG_LEVEL, "Extend chunk from %llu to %llu", chunk->data_sz,
//                                     new_chunk->offset + new_chunk->data_sz - chunk->offset);
//                            chunk->data_sz = new_chunk->offset + new_chunk->data_sz - chunk->offset;
//                        }
//                        free(new_chunk);
//                    } else {
//                        FARB_DBG(VERBOSE_ERROR_LEVEL, "FARB Warning: write overlaps between ranks %d and %d. \
//                                 Overwrite order is not defined.", chunk->rank, new_chunk->rank);
//                        FARB_DBG(VERBOSE_DBG_LEVEL, "Shrink chunk sz from %llu to %lld", chunk->data_sz, new_chunk->offset - chunk->offset);
//                        chunk->data_sz = new_chunk->offset - chunk->offset;
//                        /*insert*/
//                        assert(chunk->next == NULL);
//                        new_chunk->prev = chunk;
//                        chunk->next = new_chunk;
//                    }
//                } else { //no overlap
//                    /*insert*/
//                    assert(chunk->next == NULL);
//                    new_chunk->prev = chunk;
//                    chunk->next = new_chunk;
//                }
//            } else if(chunk->offset > new_chunk->offset){ //first chunk
//                if( (new_chunk->offset + new_chunk->data_sz - 1 >= chunk->offset) &&
//                    (new_chunk->offset + new_chunk->data_sz - 1 < chunk->offset + chunk->data_sz)){ //overlap
//                    if(chunk->rank == new_chunk->rank){
//                        FARB_DBG(VERBOSE_ERROR_LEVEL, "FARB Warning: multiple writes of the same area issued by \
//                                 rank %d. Data may be overwritten out of FIFO order. ", chunk->rank);
//                        chunk->offset = new_chunk->offset;
//                        chunk->data_sz =
//                    } else {
//                        FARB_DBG(VERBOSE_ERROR_LEVEL, "FARB Warning: write overlaps between ranks %d and %d. \
//                                 Overwrite order is not defined.", chunk->rank, new_chunk->rank);
//                    }
//                }
//            }
//            /*Check for any overlaping with existing chunks*/

        }




//       if(dbitem->chunks == NULL)
//            dbitem->chunks = chunk;
//        else{
//            tmp = dbitem->chunks;
//            while(tmp->next != NULL)
//                tmp = tmp->next;
//            tmp->next = chunk;
//            chunk->prev = tmp;
//        }

        //FARB_DBG(VERBOSE_ALL_LEVEL, "offt %d, bufsz %d", (int)offt, (int)bufsz);

    FARB_DBG(VERBOSE_ALL_LEVEL, "List after update");
    print_write_dbitem(dbitem);
}
/*
    update the database with io request from reader
*/
static void parse_ioreq_rdr_mst(void *buf, int bufsz, int rank)
{
    int ncid, var_id;
    file_buffer_t *fbuf;
    farb_var_t *var;
    read_db_item_t *dbitem;
    read_chunk_rec_t *chunk, *tmp;
    int mst1, mst2, mst;
    MPI_Offset offset, data_sz, offt_range;

    size_t offt = 0;

    ncid = *((int*)buf);
    offt += sizeof(int);
    var_id = *(int*)(buf+offt);
    offt += sizeof(int);

    fbuf = find_file_buffer(gl_filebuf_list, NULL, ncid);
    assert(fbuf != NULL);

    if(fbuf->iodb == NULL)
        init_master_db(fbuf);

    /*Find corresponding record in the database*/
    dbitem = fbuf->iodb->ritems;
    while(dbitem != NULL){
        if(dbitem->rank == rank)
            break;
        dbitem = dbitem->next;
    }

    if(dbitem == NULL){
        dbitem = (read_db_item_t*)malloc(sizeof(read_db_item_t));
        assert(dbitem != NULL);
        dbitem->rank = rank;
        dbitem->next = NULL;
        dbitem->prev = NULL;
        dbitem->chunks = NULL;
        dbitem->nchunks = 0;

        //enqueue
        if(fbuf->iodb->ritems == NULL)
            fbuf->iodb->ritems = dbitem;
        else{
            dbitem->next = fbuf->iodb->ritems;
            fbuf->iodb->ritems->prev = dbitem;
            fbuf->iodb->ritems = dbitem;
        }
        fbuf->iodb->nritems++;
    }
    FARB_DBG(VERBOSE_DBG_LEVEL, "Mst recv rreq from rdr %d for ncid %d, var %d (ritems %d)", rank, ncid, var_id, (int)fbuf->iodb->nritems);

    /*add mem chunks to the list for future matching.
      Pick only those mem chunks whose offsets are within
      the range I'm responsible for*/
    var = find_var(fbuf->vars, var_id);
    assert(var != NULL);
    offt_range = (MPI_Offset)((last_1d_index(var->ndims, var->shape)+1)/gl_conf.nmasters);
    offt_range *= var->el_sz;
    FARB_DBG(VERBOSE_ALL_LEVEL, "offt_range %d", (int)offt_range);

    while(offt != (size_t)bufsz){

        offset = *(MPI_Offset*)(buf+offt);
        offt+=sizeof(MPI_Offset);
        data_sz = *(MPI_Offset*)(buf+offt);
        offt+=sizeof(MPI_Offset);
        if(offt_range == 0){
            mst1 = mst2 = 0;
        } else {
            mst1 = (int)(offset/offt_range);
            if(mst1 >= gl_conf.nmasters)
                mst1 = gl_conf.nmasters - 1;
            mst2 = (int)( (offset+data_sz-1)/offt_range);
            if(mst2 >= gl_conf.nmasters)
                mst2 = gl_conf.nmasters - 1;
        }
        FARB_DBG(VERBOSE_ALL_LEVEL, "--> (%d, %d): mst1 %d mst2 %d", (int)offset, (int)data_sz, mst1, mst2);
        if( (gl_conf.masters[mst1] != gl_my_rank) && (gl_conf.masters[mst2] != gl_my_rank))
            /*This master is not responsible for this data*/
            continue;
        else if(gl_conf.masters[mst1] != gl_my_rank){
            for(mst = 0; mst < gl_conf.nmasters; mst++)
                if(gl_conf.masters[mst] == gl_my_rank)
                    break;
            assert(mst != gl_conf.nmasters);
            FARB_DBG(VERBOSE_DBG_LEVEL, "Changing offset from %d to %d", (int)offset, (int)(offt_range*mst));
            offset = offt_range*mst;
        } else if(gl_conf.masters[mst2] != gl_my_rank){
            for(mst = 0; mst < gl_conf.nmasters; mst++)
                if(gl_conf.masters[mst] == gl_my_rank)
                    break;
            assert(mst != gl_conf.nmasters);
            FARB_DBG(VERBOSE_DBG_LEVEL, "Changing data_sz from %d to %d", (int)data_sz, (int)(offt_range*(mst+1) - offset));
            data_sz = offt_range*(mst+1) - offset;
        }

        chunk = (read_chunk_rec_t*)malloc(sizeof(read_chunk_rec_t));
        assert(chunk != NULL);
        chunk->var_id = var_id;
        chunk->offset = offset;
        chunk->data_sz = data_sz;
        chunk->next = NULL;
        chunk->prev = NULL;

        if(dbitem->chunks == NULL)
            dbitem->chunks = chunk;
        else{
            tmp = dbitem->chunks;
            while(tmp->next != NULL)
                tmp = tmp->next;
            tmp->next = chunk;
            chunk->prev = tmp;
        }
        dbitem->nchunks++;
    }

    assert(dbitem->nchunks > 0);

    print_read_dbitem(dbitem);
}

io_req_t *new_ioreq(int id,
                    int var_id,
                    int ndims,
                    MPI_Offset el_sz,
                    const MPI_Offset *start,
                    const MPI_Offset *count,
                    void *buf,
                    int rw_flag)
{

    io_req_t *ioreq = (io_req_t*)malloc(sizeof(io_req_t));
    assert(ioreq != NULL);
    if(ndims > 0){

        ioreq->user_buf_sz = el_sz * (last_1d_index(ndims, count) + 1);
        FARB_DBG(VERBOSE_ALL_LEVEL, "req %d, var %d, user bufsz %d", id, var_id, (int)ioreq->user_buf_sz);
        ioreq->start = (MPI_Offset*)malloc(sizeof(MPI_Offset)*ndims);
        assert(ioreq->start != NULL);
        memcpy((void*)ioreq->start, (void*)start, sizeof(MPI_Offset)*ndims);
        ioreq->count = (MPI_Offset*)malloc(sizeof(MPI_Offset)*ndims);
        assert(ioreq->count != NULL);
        memcpy((void*)ioreq->count, (void*)count, sizeof(MPI_Offset)*ndims);
    } else{
        ioreq->start = NULL;
        ioreq->count = NULL;
        ioreq->user_buf_sz = el_sz;
    }
    ioreq->user_buf = buf;
    ioreq->next = NULL;
    ioreq->mem_chunks = NULL; //will init later
    ioreq->id = id;
    ioreq->var_id = var_id;
    ioreq->completed = 0;
    ioreq->nchunks = 0;
    ioreq->get_sz = 0;
    ioreq->prev = NULL;
    return ioreq;
}

void delete_ioreq(io_req_t **ioreq)
{
     contig_mem_chunk_t *chunk;

    if( (*ioreq)->count != NULL )
        free((*ioreq)->count);
    if((*ioreq)->start != NULL)
        free((*ioreq)->start);

    chunk = (*ioreq)->mem_chunks;
    while(chunk != NULL){
        (*ioreq)->mem_chunks = chunk->next;
        free(chunk);
        chunk = (*ioreq)->mem_chunks;
    }
    free((*ioreq));
}

/* Reader ranks and writer ranks send their read/write
   io request to master(s)
*/
void send_ioreq(int ncid, io_req_t *ioreq, int rw_flag)
{
    contig_mem_chunk_t *chunk;
    void *sbuf = NULL;
    size_t sz = 0, offt = 0;
    int *mst_flag, mst1, mst2, i, errno;
    MPI_Request *sreqs;
    MPI_Offset offt_range;
    farb_var_t *var;
    file_buffer_t *fbuf;

    if(rw_flag == FARB_READ)
        FARB_DBG(VERBOSE_DBG_LEVEL, "Send read request to master for var %d", ioreq->var_id);
    else
        FARB_DBG(VERBOSE_DBG_LEVEL, "Send write request to master for var %d", ioreq->var_id);

    mst_flag = (int*)malloc(gl_conf.nmasters*sizeof(int));
    assert(mst_flag != NULL);
    sreqs = (MPI_Request*)malloc(gl_conf.nmasters*sizeof(MPI_Request));
    assert(sreqs != NULL);
    for(i = 0; i < gl_conf.nmasters; i++){
        mst_flag[i] = 0;
        sreqs[i] = MPI_REQUEST_NULL;
    }

    /*Pack the read request.*/
    offt = 0;
    sz = sizeof(ncid)+sizeof(ioreq->var_id)+sizeof(MPI_Offset)*2*ioreq->nchunks;
    sbuf = malloc(sz);
    assert(sbuf != NULL);

    *(int*)sbuf = ncid;
    offt += sizeof(int);
    *(int*)(sbuf + offt) = ioreq->var_id;
    offt += sizeof(int);

    fbuf = find_file_buffer(gl_filebuf_list, NULL, ncid);
    assert(fbuf != NULL);
    var = find_var(fbuf->vars, ioreq->var_id);
    assert(var != NULL);

    offt_range = (MPI_Offset)(( last_1d_index(var->ndims, var->shape) + 1 )/gl_conf.nmasters);
    offt_range *= var->el_sz;
    FARB_DBG(VERBOSE_DBG_LEVEL, "Offset range is %d", (int)offt_range);

    chunk = ioreq->mem_chunks;
    while(chunk != NULL){
        *(MPI_Offset*)(sbuf+offt) = chunk->offset;
        offt += sizeof(MPI_Offset);
        *(MPI_Offset*)(sbuf+offt) = chunk->data_sz;
        offt += sizeof(MPI_Offset);

        /*Set flag for each master to whom will need to send this request*/
        if(offt_range == 0)
            mst1 = mst2 = 0;
        else {
            mst1 = (int)(chunk->offset/offt_range);
            if(mst1 >= gl_conf.nmasters)
                mst1 = gl_conf.nmasters - 1;
            mst2 = (int)((chunk->offset + chunk->data_sz - 1)/offt_range);
            if(mst2 >= gl_conf.nmasters)
                mst2 = gl_conf.nmasters - 1;
        }
        FARB_DBG(VERBOSE_ALL_LEVEL, "-> (%d, %d): mst1 %d mst2 %d", (int)chunk->offset, (int)chunk->data_sz, mst1, mst2);
        for(i = mst1; i <= mst2; i++)
            mst_flag[i] = 1;
        chunk = chunk->next;
    }
    assert(offt == sz);

//    /*Set flag for each master to whom will need to send this request*/
//    if(offt_range == 0)
//        mst1 = mst2 = 0;
//    else {
//        mst1 = (int)((to_1d_index(var->ndims, var->shape, ioreq->start)*var->el_sz)/offt_range);
//        if(mst1 >= gl_conf.nmasters)
//            mst1 = gl_conf.nmasters - 1;
//        end_coord = (MPI_Offset*)malloc(var->ndims*sizeof(MPI_Offset));
//        assert(end_coord != NULL);
//        for(i = 0; i < var->ndims; i++)
//            end_coord[i] = ioreq->start[i] + ioreq->count[i];
//        mst2 = (int)((to_1d_index(var->ndims, var->shape, end_coord)*var->el_sz)/offt_range);
//        free(end_coord);
//        if(mst2 >= gl_conf.nmasters)
//            mst2 = gl_conf.nmasters - 1;
//    }


    /*Send the read request to the master*/
    for(i = 0; i < gl_conf.nmasters; i++){
        if(!mst_flag[i])
            continue;
        errno = MPI_SUCCESS;
        if(rw_flag == FARB_READ)
            errno = MPI_Isend(sbuf, (int)sz, MPI_BYTE, gl_conf.masters[i], IO_READ_REQ_TAG, gl_comps[fbuf->writer_id].intercomm, &sreqs[i]);
        else{
            if(gl_conf.masters[i] == gl_my_rank)
                parse_ioreq_wrt_mst(sbuf, (int)sz, gl_my_rank);
            else
                errno = MPI_Isend(sbuf, (int)sz, MPI_BYTE, gl_conf.masters[i], IO_WRITE_REQ_TAG, gl_comps[gl_my_comp_id].intercomm, &sreqs[i]);
        }
        CHECK_MPI(errno);
    }
    errno = MPI_Waitall(gl_conf.nmasters, sreqs, MPI_STATUSES_IGNORE);
    CHECK_MPI(errno);
//    while(!completed){
//        errno = MPI_Testall(gl_conf.nmasters, sreqs, &completed, MPI_STATUSES_IGNORE);
//        CHECK_MPI(errno);
//        progress_io_matching();
//    }

    free(sbuf);
    free(mst_flag);
    free(sreqs);
}


int match_ioreqs(file_buffer_t *fbuf)
{
    int errno;
    FARB_DBG(VERBOSE_DBG_LEVEL, "Match ioreqs for file %d", fbuf->ncid);
    assert(fbuf->ioreqs != NULL);
    if(gl_conf.my_master == gl_my_rank){
        if(fbuf->iodb == NULL)
            init_master_db(fbuf);
        //reset
        fbuf->iodb->nranks_completed = 0;
    }

    while(fbuf->ioreq_cnt > 0){
        if( (gl_conf.my_master == gl_my_rank) && (fbuf->writer_id == gl_my_comp_id) )
            do_matching(fbuf);
        progress_io_matching();
    }

    if(fbuf->reader_id == gl_my_comp_id){
        /*Notify my master writer rank that all my io
        requests for this file have been completed*/
        errno = MPI_Send(&(fbuf->ncid), 1, MPI_INT, gl_conf.my_master, IO_DONE_TAG, gl_comps[fbuf->writer_id].intercomm);
        CHECK_MPI(errno);
    }
    return 0;
}

/*writer->reader*/
static void send_data_wrt(void* buf, int bufsz)
{
    int ncid, var_id, rdr_rank, errno;
    io_req_t *ioreq = NULL;
    file_buffer_t *fbuf;
    size_t rofft = 0, sofft = 0;
    void *sbuf = NULL;
    size_t sbufsz = 0;
    MPI_Offset chunk_offt, chunk_sz, dbuf_offt;
    contig_mem_chunk_t *chunk;

    rdr_rank = *(int*)buf;
    rofft += sizeof(int);
    FARB_DBG(VERBOSE_DBG_LEVEL, "Sending data to rank %d", rdr_rank);
    ncid = *(int*)(buf+rofft);
    rofft += sizeof(int);

    fbuf = find_file_buffer(gl_filebuf_list, NULL, ncid);
    assert(fbuf != NULL);

    /*First compute the size of buf to allocate*/

    rofft += sizeof(int)+sizeof(MPI_Offset);
    sbufsz = sizeof(ncid);
    while(rofft < bufsz){
        chunk_sz = *(MPI_Offset*)(buf+rofft);
        sbufsz += sizeof(int)+sizeof(MPI_Offset)*2+(size_t)chunk_sz;
        rofft += sizeof(int)+sizeof(MPI_Offset)*2;
    }

    //FARB_DBG(VERBOSE_ALL_LEVEL, "Alloc buf of sz %d", (int)sbufsz);
    sbuf = malloc(sbufsz);
    assert(sbuf != NULL);
    *(int*)sbuf = ncid;
    sofft = sizeof(int);

    //rewind
    rofft = sizeof(int)*2;
    while(rofft != bufsz){
        var_id = *(int*)(buf+rofft);
        rofft += sizeof(int);

        chunk_offt = *(MPI_Offset*)(buf+rofft);
        rofft += sizeof(MPI_Offset);
        chunk_sz = *(MPI_Offset*)(buf+rofft);
        rofft += sizeof(MPI_Offset);

        /*Save info about the memory chunk*/
        *(int*)(sbuf+sofft) = var_id;
        sofft += sizeof(int);
        *(MPI_Offset*)(sbuf+sofft) = chunk_offt;
        sofft += sizeof(MPI_Offset);
        *(MPI_Offset*)(sbuf+sofft) = chunk_sz;
        sofft += sizeof(MPI_Offset);

        FARB_DBG(VERBOSE_DBG_LEVEL, "Will copy (var %d, offt %d, sz %d)", var_id, (int)chunk_offt, (int)chunk_sz);

        /*Find the ioreq that has info about this mem chunk*/
        ioreq = fbuf->ioreqs;
        while(ioreq != NULL){
            if(ioreq->var_id == var_id){
                chunk = ioreq->mem_chunks;
                while(chunk != NULL){
                    if( (chunk_offt >= chunk->offset) &&
                        (chunk_offt < chunk->offset+chunk->data_sz) &&
                        (chunk_offt+chunk_sz <= chunk->offset+chunk->data_sz))
                        break;

                    chunk = chunk->next;
                }
                if(chunk != NULL)
                    break;
            }
            ioreq = ioreq->next;
        }
        assert(ioreq != NULL);
        assert(chunk != NULL);
        /*Copy data*/
        assert(chunk_offt - ioreq->mem_chunks->offset >= 0);
        dbuf_offt = chunk->usrbuf_offset + chunk_offt - chunk->offset;
        //FARB_DBG(VERBOSE_ALL_LEVEL, "dbuf offt %d, chunk usr buf offt %d (usrbuf sz %d)", (int)dbuf_offt, (int)chunk->usrbuf_offset, (int)ioreq->user_buf_sz);
        assert(dbuf_offt < ioreq->user_buf_sz);
        memcpy(sbuf+sofft, ioreq->user_buf+dbuf_offt, (size_t)chunk_sz);
        sofft += (size_t)chunk_sz;
    }
    FARB_DBG(VERBOSE_ALL_LEVEL, "Sofft %d, sbufsz %d", (int)sofft, (int)sbufsz);
    assert(sofft == sbufsz);
    errno = MPI_Send(sbuf, (int)sbufsz, MPI_BYTE, rdr_rank, IO_DATA_TAG, gl_comps[fbuf->reader_id].intercomm);
    CHECK_MPI(errno);
    free(sbuf);
}

/*writer->reader*/
static void recv_data_rdr(void* buf, int bufsz)
{
    int ncid, var_id;
    io_req_t *ioreq = NULL;
    file_buffer_t *fbuf;
    size_t offt = 0;
    MPI_Offset chunk_offt, chunk_sz, dbuf_offt;
    contig_mem_chunk_t *chunk;

    ncid = *(int*)(buf);
    offt += sizeof(int);

    fbuf = find_file_buffer(gl_filebuf_list, NULL, ncid);
    assert(fbuf != NULL);

    while(offt != bufsz){
        var_id = *(int*)(buf+offt);
        offt += sizeof(int);
        chunk_offt = *(MPI_Offset*)(buf+offt);
        offt += sizeof(MPI_Offset);
        chunk_sz = *(MPI_Offset*)(buf+offt);
        offt += sizeof(MPI_Offset);

        FARB_DBG(VERBOSE_DBG_LEVEL, "Will recv (var %d, offt %d, sz %d)", var_id, (int)chunk_offt, (int)chunk_sz);

        /*Find the ioreq*/
        ioreq = fbuf->ioreqs;
        while(ioreq != NULL){
            if(ioreq->var_id == var_id){
                chunk = ioreq->mem_chunks;
                while(chunk != NULL){
                    if( (chunk_offt >= chunk->offset) &&
                        (chunk_offt < chunk->offset+chunk->data_sz) &&
                        (chunk_offt+chunk_sz <= chunk->offset+chunk->data_sz))
                        break;

                    chunk = chunk->next;
                }
                if(chunk != NULL)
                    break;
            }
            ioreq = ioreq->next;
        }
        assert(ioreq != NULL);
        assert(chunk != NULL);
        /*Copy data*/
        assert(chunk_offt - ioreq->mem_chunks->offset >= 0);
        dbuf_offt = chunk->usrbuf_offset + chunk_offt - chunk->offset;
        FARB_DBG(VERBOSE_ALL_LEVEL, "dbuf offt %d", (int)dbuf_offt);
        assert(dbuf_offt < ioreq->user_buf_sz);
        memcpy(ioreq->user_buf+dbuf_offt, buf+offt, (size_t)chunk_sz);
        offt += (size_t)chunk_sz;

        ioreq->get_sz += chunk_sz;

        FARB_DBG(VERBOSE_ALL_LEVEL, "req %d, var %d, Got %d (expect %d)", ioreq->id, ioreq->var_id, (int)ioreq->get_sz, (int)ioreq->user_buf_sz);

        if(ioreq->get_sz == ioreq->user_buf_sz){
            FARB_DBG(VERBOSE_DBG_LEVEL, "Complete req %d (left %d)", ioreq->id, fbuf->ioreq_cnt-1);
            if(ioreq == fbuf->ioreqs)
                fbuf->ioreqs = fbuf->ioreqs->next;
            //delete this ioreq
            if(ioreq->next != NULL)
                ioreq->next->prev = ioreq->prev;
            if(ioreq->prev != NULL)
                ioreq->prev->next = ioreq->next;
            delete_ioreq(&ioreq);
            fbuf->ioreq_cnt--;
        }
    }
}

/*Send the file header and info about vars to the reader when the writer finishes the def mode*/
void send_file_info(file_buffer_t *fbuf)
{
    void *sbuf;
    int sbuf_sz, errno;
    MPI_Request sreq[3];
    errno = MPI_Isend(fbuf->file_path, MAX_FILE_NAME, MPI_CHAR, gl_my_rank, FILE_READY_TAG, gl_comps[fbuf->reader_id].intercomm, &sreq[0]);
    CHECK_MPI(errno);

    assert(fbuf->hdr_sz > 0);
    errno = MPI_Isend(fbuf->header, (int)fbuf->hdr_sz, MPI_BYTE, gl_my_rank, HEADER_TAG, gl_comps[fbuf->reader_id].intercomm, &sreq[1]);
    CHECK_MPI(errno);

    pack_vars(fbuf, &sbuf_sz, &sbuf);
    assert(sbuf_sz > 0);
    errno = MPI_Isend(sbuf, sbuf_sz, MPI_BYTE, gl_my_rank, VARS_TAG, gl_comps[fbuf->reader_id].intercomm, &sreq[2]);
    CHECK_MPI(errno);

    errno = MPI_Waitall(3, sreq, MPI_STATUSES_IGNORE);
    CHECK_MPI(errno);
    free(sbuf);
}

void progress_io_matching()
{
    MPI_Status status;
    int comp, flag, src, errno;
    file_buffer_t *fbuf;
    int bufsz;
    void *rbuf;
    char filename[MAX_FILE_NAME];
    int ncid;

    for(comp = 0; comp < gl_ncomp; comp++){
        if( gl_comps[comp].intercomm == MPI_COMM_NULL){
            continue;
        }

        errno = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, gl_comps[comp].intercomm, &flag, &status);
        CHECK_MPI(errno);

        if(!flag){
            continue;
        }

        switch(status.MPI_TAG){
            case FILE_READY_TAG:
                src = status.MPI_SOURCE;
                errno = MPI_Recv(filename, MAX_FILE_NAME, MPI_CHAR, src, FILE_READY_TAG, gl_comps[comp].intercomm, &status);
                CHECK_MPI(errno);
                FARB_DBG(VERBOSE_DBG_LEVEL,   "Receive FILE_READY notif for %s", filename);

                fbuf = find_file_buffer(gl_filebuf_list, filename, -1);
                assert(fbuf != NULL);

                /*Receive the header*/
                MPI_Probe(src, HEADER_TAG, gl_comps[comp].intercomm, &status);
                MPI_Get_count(&status, MPI_BYTE, &bufsz);
                fbuf->hdr_sz = (MPI_Offset)bufsz;
                FARB_DBG(VERBOSE_DBG_LEVEL, "Hdr size to receive %d", bufsz);
                fbuf->header = malloc((size_t)bufsz);
                assert(fbuf->header != NULL);
                errno = MPI_Recv(fbuf->header, bufsz, MPI_BYTE, src, HEADER_TAG, gl_comps[comp].intercomm, &status);
                CHECK_MPI(errno);

                /*Receive info about vars*/
                MPI_Probe(src, VARS_TAG, gl_comps[comp].intercomm, &status);
                MPI_Get_count(&status, MPI_BYTE, &bufsz);
                rbuf = malloc(bufsz);
                assert(rbuf != NULL);
                errno = MPI_Recv(rbuf, bufsz, MPI_BYTE, src, VARS_TAG, gl_comps[comp].intercomm, &status);
                CHECK_MPI(errno);
                unpack_vars(fbuf, bufsz, rbuf);
                free(rbuf);
                fbuf->is_ready = 1;
                break;
            case IO_WRITE_REQ_TAG:
                MPI_Get_count(&status, MPI_BYTE, &bufsz);
                rbuf = malloc(bufsz);
                assert(rbuf != NULL);
                src = status.MPI_SOURCE;
                errno = MPI_Recv(rbuf, bufsz, MPI_BYTE, src, IO_WRITE_REQ_TAG, gl_comps[comp].intercomm, &status);
                CHECK_MPI(errno);
                parse_ioreq_wrt_mst(rbuf, bufsz, src);
                free(rbuf);
                break;

            case IO_READ_REQ_TAG:
                MPI_Get_count(&status, MPI_BYTE, &bufsz);
                rbuf = malloc(bufsz);
                assert(rbuf != NULL);
                src = status.MPI_SOURCE;
                errno = MPI_Recv(rbuf, bufsz, MPI_BYTE, src, IO_READ_REQ_TAG, gl_comps[comp].intercomm, &status);
                CHECK_MPI(errno);
                parse_ioreq_rdr_mst(rbuf, bufsz, src);
                free(rbuf);
                break;
            case IO_DATA_REQ_TAG:
                MPI_Get_count(&status, MPI_BYTE, &bufsz);
                rbuf = malloc(bufsz);
                assert(rbuf != NULL);
                src = status.MPI_SOURCE;
                errno = MPI_Recv(rbuf, bufsz, MPI_BYTE, src, IO_DATA_REQ_TAG, gl_comps[comp].intercomm, &status);
                CHECK_MPI(errno);
                send_data_wrt(rbuf, bufsz);
                free(rbuf);
                break;
            case IO_DATA_TAG:
                MPI_Get_count(&status, MPI_BYTE, &bufsz);
                rbuf = malloc(bufsz);
                assert(rbuf != NULL);
                src = status.MPI_SOURCE;
                errno = MPI_Recv(rbuf, bufsz, MPI_BYTE, src, IO_DATA_TAG, gl_comps[comp].intercomm, &status);
                CHECK_MPI(errno);
                FARB_DBG(VERBOSE_DBG_LEVEL, "Recved data from %d", src);
                recv_data_rdr(rbuf, bufsz);
                free(rbuf);
                break;
            case IO_DONE_TAG:
                errno = MPI_Recv(&ncid, 1, MPI_INT, status.MPI_SOURCE, IO_DONE_TAG, gl_comps[comp].intercomm, &status);
                CHECK_MPI(errno);
                fbuf = find_file_buffer(gl_filebuf_list, NULL, ncid);
                assert(fbuf != NULL);
                if(gl_my_rank == gl_conf.my_master){ //I am master


                    if(comp == gl_my_comp_id){
                        fbuf->iodb->nmst_completed++; //this message was from another writer master
                        FARB_DBG(VERBOSE_DBG_LEVEL, "Recevied DONE from mst %d (tot done %u)", status.MPI_SOURCE, fbuf->iodb->nmst_completed);
                    }
                     else{
                        fbuf->iodb->nranks_completed++; //this message was from a reader
                        FARB_DBG(VERBOSE_DBG_LEVEL, "Recevied DONE from reader %d (tot done %u)", status.MPI_SOURCE, fbuf->iodb->nranks_completed);

                        if(fbuf->iodb->nranks_completed == gl_conf.my_workgroup_sz){
                            /*Notify other masters that readers from my workgroup
                              have finished */
                              int i;
                              FARB_DBG(VERBOSE_DBG_LEVEL, "Notify masters my reader ranks completed");
                              for(i = 0; i < gl_conf.nmasters; i++){
                                if(gl_conf.masters[i] == gl_my_rank)
                                    continue;
                                errno = MPI_Send(&ncid, 1, MPI_INT, gl_conf.masters[i], IO_DONE_TAG, gl_comps[gl_my_comp_id].intercomm);
                                CHECK_MPI(errno);
                              }
                              fbuf->iodb->nmst_completed++;
                        }
                    }



                    if(fbuf->iodb->nmst_completed == gl_conf.nmasters){
                        int i;
                        io_req_t *ioreq, *tmp;
                        FARB_DBG(VERBOSE_DBG_LEVEL, "Notify all other ranks that req matching completed");
                        /*Tell other writer ranks that they can complete
                        their write requests*/
                        for(i = 1; i < gl_conf.my_workgroup_sz; i++){
                            errno = MPI_Send(&ncid, 1, MPI_INT, gl_my_rank+i, IO_DONE_TAG, gl_comps[gl_my_comp_id].intercomm);
                            CHECK_MPI(errno);
                        }
                        /*Check that I don't have any read reqs incompleted*/
                        assert(fbuf->iodb->nritems == 0);
                        /*Complete my own write requests*/
                        ioreq = fbuf->ioreqs;
                        while(ioreq != NULL){
                            ioreq->completed = 1;
                            tmp = ioreq;
                            ioreq = ioreq->next;
                            delete_ioreq(&tmp);
                        }
                        fbuf->ioreq_cnt = 0;
                        /*Clean my iodb*/
                        clean_iodb(fbuf->iodb);
                    }

                } else { /*I am writer*/
                    io_req_t *ioreq, *tmp;
                    /*Complete all write requests for this file*/
                    ioreq = fbuf->ioreqs;
                    while(ioreq != NULL){
                        ioreq->completed = 1;
                        tmp = ioreq;
                        ioreq = ioreq->next;
                        delete_ioreq(&tmp);
                    }
                    fbuf->ioreq_cnt = 0;
                }

                break;
            default:
                FARB_DBG(VERBOSE_ERROR_LEVEL, "FARB Error: unknown tag %d", status.MPI_TAG);
                assert(0);
        }

    }
}