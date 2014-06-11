/*
* fmo_synch.h
 *
 *  Created on: 2014. 4. 2.
 *      Author: innocentevil
 */

#ifndef FMO_SYNCH_H_
#define FMO_SYNCH_H_

#include "fmo_thread.h"
#include "port/cortex_v7m_port.h"




#define MTX_INIT                   {0,GENERIC_LIST_QUEUE_INIT}



typedef struct _mtx_wait_queue_t mtx_wait_queue;
typedef struct _tch_mtx_t tch_mtx_lock;
typedef struct _tch_sem_t tch_sem_lock;

typedef struct _tch_async_iobuffer_t tch_async_iobuffer;

struct _mtx_wait_queue_t {
	void* entry;
	void* tail;
};

struct _tch_mtx_t{
	uint32_t key;
	mtx_wait_queue w_q;
};

struct _tch_sem_lock_t {
	uint32_t cnt;
	mtx_wait_queue w_q;
};


struct _tch_async_iobuffer_t {
	void*                   _bp;
	tch_mtx_lock                 bwlock;
	tch_mtx_lock                 brlock;
	tch_genericList_queue_t   bRdQue;
	tch_genericList_queue_t   bWrQue;
	uint16_t                 flag;
	uint32_t                 blen;
	uint32_t                 ridx;
	uint32_t                 widx;
	uint32_t                 wsize;
};

void tch_Mtx_init(tch_mtx_lock* lock);
BOOL tch_Mtx_lockt(tch_mtx_lock* lock,BOOL wait);
void tch_Mtx_unlockt(tch_mtx_lock* lock);
void tch_Mtx_deinit(tch_mtx_lock* lock);

void tch_Sem_init(tch_sem_lock* sem,uint32_t maxcnt);
BOOL tch_Sem_up(tch_sem_lock* sem, BOOL wait);
BOOL tch_Sem_down(tch_sem_lock* sem, BOOL wait);
void tch_Sem_deinit(tch_sem_lock* sem);





void tch_async_iobuffer_init(tch_async_iobuffer* ins,void* bp,uint32_t size,BOOL th_safe);
void tch_async_iobuffer_putc(tch_async_iobuffer* ins,uint8_t b);
uint8_t tch_async_iobuffer_getc(tch_async_iobuffer* ins);
BOOL tch_async_iobuffer_write(tch_async_iobuffer* ins,const uint8_t* bp,uint32_t size);
BOOL tch_async_iobuffer_read(tch_async_iobuffer* ins,uint8_t* bp,uint32_t size);
BOOL tch_async_iobuffer_close(tch_async_iobuffer* ins);


#endif /* FMO_SYNCH_H_ */
