/*
 * fmo_synch.c
 *
 *  Created on: 2014. 4. 2.
 *      Author: innocentevil
 */


#include "fmo_synch.h"
#include "fmo_sched.h"


#define ASYNC_BUFFER_FLAG_THREAD_SAFE                 (uint16_t) (1 << 0)
#define ASYNC_BUFFER_FLAG_WRBUSY                      (uint16_t) (1 << 1)
#define ASYNC_BUFFER_FLAG_RDBUSY                      (uint16_t) (1 << 2)
#define ASYNC_BUFFER_FLAG_CLOSED                      (uint16_t) (1 << 3)

#define ASYNC_BUFFER_SET_THREAD_SAFE(abp)             (abp->flag |= ASYNC_BUFFER_FLAG_THREAD_SAFE)
#define ASYNC_BUFFER_CLR_THREAD_SAFE(abp)             (abp->flag &= ~ASYNC_BUFFER_FLAG_THREAD_SAFE)
#define ASYNC_BUFFER_IS_THREAD_SAFE(abp)              (abp->flag & ASYNC_BUFFER_FLAG_THREAD_SAFE)

#define ASYNC_BUFFER_SET_WRBUSY(abp)                  (abp->flag |= ASYNC_BUFFER_FLAG_WRBUSY)
#define ASYNC_BUFFER_CLR_WRBUSY(abp)                  (abp->flag &= ~ASYNC_BUFFER_FLAG_WRBUSY)
#define ASYNC_BUFFER_IS_WRBUSY(abp)                   (abp->flag & ASYNC_BUFFER_FLAG_WRBUSY)

#define ASYNC_BUFFER_SET_RDBUSY(abp)                  (abp->flag |= ASYNC_BUFFER_FLAG_RDBUSY)
#define ASYNC_BUFFER_CLR_RDBUSY(abp)                  (abp->flag &= ~ASYNC_BUFFER_FLAG_RDBUSY)
#define ASYNC_BUFFER_IS_RDBUSY(abp)                   (abp->flag & ASYNC_BUFFER_FLAG_RDBUSY)




void tch_Mtx_init(mtx_lock* lock){
	lock->key = (uint32_t) NULL;
	tch_genericQue_Init((tch_genericList_queue_t*)&lock->w_q);
}


void tch_Mtx_lockt(mtx_lock* lock,uint16_t try_mode){
	while(1){
		if(!__get_IPSR()){
			__sv_call(SVC_MTX_LOCK,(uint32_t) lock,try_mode);
		}else{
			__pndsv_call(SVC_MTX_LOCK,(uint32_t)lock,try_mode);
		}
		if(lock->key){                                                                 //// check whether thread lock mtx and return,otherwise retry lock mtx
			return;
		}
	}
}

void tch_Mtx_unlockt(mtx_lock* lock){
	if(!__get_IPSR()){
		__sv_call(SVC_MTX_UNLOCK,(uint32_t)lock,0);
	}else{
		__pndsv_call(SVC_MTX_UNLOCK,(uint32_t) lock,0);
	}
}

void tch_async_iobuffer_init(tch_async_iobuffer* ins,void* bp,uint32_t size,BOOL th_safe){
	ins->_bp = bp;
	tch_genericQue_Init(&ins->bRdQue);
	tch_genericQue_Init(&ins->bWrQue);
	tch_Mtx_init(&ins->brlock);
	tch_Mtx_init(&ins->bwlock);

	ins->blen = size;
	ins->ridx = 0;
	ins->widx = 0;
	ins->wsize = 0;
	if(th_safe){ASYNC_BUFFER_SET_THREAD_SAFE(ins);}
}

void tch_async_iobuffer_putc(tch_async_iobuffer* ins,uint8_t b){
	uint8_t* bp = ins->_bp;
	if(!__get_IPSR()){
		while(ASYNC_BUFFER_IS_WRBUSY(ins)){
			tchThread_sleep(1);
			// check stream is closed
		}
		if(ASYNC_BUFFER_IS_THREAD_SAFE(ins)){
			tch_Mtx_lockt(&ins->bwlock,MTX_TRYMODE_WAIT);
			// check stream is closed
			ASYNC_BUFFER_SET_WRBUSY(ins);
			tch_Mtx_unlockt(&ins->bwlock);
		}else{
			ASYNC_BUFFER_SET_WRBUSY(ins);
		}
		while(ins->wsize == ins->blen){
			tchThread_wait(&ins->bWrQue);
			// check stream is closed
		}
		*(bp + ins->widx++) = b;
		ins->wsize++;
		if(ins->widx == ins->blen){
			ins->widx = 0;
		}
		if(ins->bRdQue.entry != NULL){
			tchThread_wake(&ins->bRdQue);
		}
	}else{
		if(ins->wsize == ins->blen){
			return;
		}
		*(bp + ins->widx++) = b;
		ins->wsize++;
		if(ins->widx == ins->blen){
			ins->widx = 0;
		}
		if(ins->bRdQue.entry != NULL){
			tchThread_wake(&ins->bRdQue);
		}
	}
}

uint8_t tch_async_iobuffer_getc(tch_async_iobuffer* ins){
	uint8_t* bp = ins->_bp;
	uint8_t rdX = 0;
	if(!__get_IPSR()){
		while(ASYNC_BUFFER_IS_RDBUSY(ins)){
			// check stream is closed
			tchThread_sleep(1);
		}
		if(ASYNC_BUFFER_IS_THREAD_SAFE(ins)){
			tch_Mtx_lockt(&ins->brlock,MTX_TRYMODE_WAIT);
			// check stream is closed, if closed thread will unlock mtx and return
			ASYNC_BUFFER_SET_RDBUSY(ins);
			tch_Mtx_unlockt(&ins->brlock);
		}
		while(ins->ridx == ins->widx){
			tchThread_wait(&ins->bWrQue);
			// check stream is closed
		}
		rdX = *(bp + ins->ridx++);
		ins->wsize--;
		if(ins->ridx == ins->blen){
			ins->ridx = 0;
		}
		ASYNC_BUFFER_CLR_RDBUSY(ins);
		return rdX;
	}else{
		if(ins->ridx == ins->widx){
			return 0;
		}
		rdX = *(bp + ins->ridx++);
		ins->wsize--;
		if(ins->ridx == ins->blen){
			ins->ridx = 0;
		}
		return rdX;
	}
}

BOOL tch_async_iobuffer_write(tch_async_iobuffer* ins,const uint8_t* bp,uint32_t size){
	uint8_t* sbp = bp;
	if(!__get_IPSR()){
		while(ASYNC_BUFFER_IS_WRBUSY(ins)){
			tchThread_sleep(1);
			// check stream is closed
		}
		if(ASYNC_BUFFER_IS_THREAD_SAFE(ins)){
			tch_Mtx_lockt(&ins->bwlock,MTX_TRYMODE_WAIT);
			// check stream is closed, if closed thread will unlock mtx and return
			ASYNC_BUFFER_SET_WRBUSY(ins);
			tch_Mtx_unlockt(&ins->bwlock);
		}
		while(size--){
			while(ins->wsize == ins->blen){
				tchThread_wait(&ins->bWrQue);
				// check stream is closed
			}
			*((uint8_t*)ins->_bp + ins->widx++) = *sbp++;
			ins->wsize++;
			if(ins->widx == ins->blen){
				ins->widx = 0;
			}
		}
		if(ins->bRdQue.entry != NULL){
			tchThread_wake(&ins->bRdQue);
		}
		ASYNC_BUFFER_CLR_WRBUSY(ins);
		return TRUE;
	}else{
		while(size--){
			if(ins->wsize == ins->blen){
				return FALSE;
			}
			*((uint8_t*)ins->_bp + ins->widx++) = *sbp++;
			ins->wsize++;
			if(ins->widx == ins->blen){
				ins->widx = 0;
			}
		}
		return TRUE;
	}
}

BOOL tch_async_iobuffer_read(tch_async_iobuffer* ins,uint8_t* bp,uint32_t size){
	uint8_t* sbp = ins->_bp;
	if(!__get_IPSR()){
		while(ASYNC_BUFFER_IS_RDBUSY(ins)){
			tchThread_sleep(1);
			// check stream is closed
		}
		if(ASYNC_BUFFER_IS_THREAD_SAFE(ins)){
			tch_Mtx_lockt(&ins->brlock,MTX_TRYMODE_WAIT);
			// check stream is closed
			ASYNC_BUFFER_SET_RDBUSY(ins);
			tch_Mtx_unlockt(&ins->brlock);
		}
		while(size--){
			while(ins->ridx == ins->widx){
				tchThread_wait(&ins->bRdQue);
				// check stream is closed
			}
			*bp++ = *(sbp + ins->ridx++);
			ins->wsize--;
			if(ins->ridx == ins->blen){
				ins->ridx = 0;
			}
		}
		if(ins->bWrQue.entry != NULL){
			tchThread_wake(&ins->bWrQue);
		}
		ASYNC_BUFFER_CLR_RDBUSY(ins);
		return TRUE;
	}else{
		while(size--){
			if(ins->ridx == ins->widx){
				return FALSE;
			}
			*bp++ = *(sbp + ins->ridx++);
			ins->wsize--;
			if(ins->ridx == ins->blen){
				ins->ridx = 0;
			}
		}
		if(ins->bWrQue.entry != NULL){
			tchThread_wake(&ins->bWrQue);
		}
		return TRUE;
	}
}

BOOL tch_async_iobuffer_close(tch_async_iobuffer* ins){

}

