/*
 * tch_ipc.h
 *
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 *
 *
 * This Module Contains IPC Component for Tachyos. Basic IPC Features are
 *   listed below.
 *
 *        -     Message Queue  : Uni Directional communication channel
 *                               which enables thread communicated to other
 *                               thread or ISR handler
 *
 *        -     Mail Queue     : similar to message queue but mail queue has its
 *                               own mem pool and typically transfer memory pointer
 *                               allocated from its memory pool
 *
 *
 */

#ifndef TCH_IPC_H_
#define TCH_IPC_H_

#include "tch_Typedef.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define TCH_MSGQ_HEAD_SIZE             (sizeof(uint32_t) * 3 + sizeof(void*) + 4 * sizeof(void*))
#define TCH_MAILQ_HEAD_SIZE            (3 * sizeof(uint32_t) + 7 * sizeof(void*))

#define tch_msgQDef(name,size)\
uint8_t msgQ_##name_pool[size * sizeof(void*) + TCH_MSGQ_HEAD_SIZE];\
static tch_msgQueDef_t msgQ_##name = {size,sizeof(void*),msgQ_##name_pool + TCH_MSGQ_HEAD_SIZE}

#define tch_access_msgq(name)\
&msgQ_##name



#define tch_mailQDef(name,size,type)\
uint8_t mailQ_##name_pool[TCH_MAILQ_HEAD_SIZE + size * sizeof(type) + size * sizeof(void*)];\
static tch_mailQueDef_t mailQ_##name = {size,sizeof(type),mailQ_##name_pool + TCH_MAILQ_HEAD_SIZE,mailQ_##name_pool + TCH_MAILQ_HEAD_SIZE + size * sizeof(type)}


#define tch_access_mailq(name)\
&mailQ_##name

/***
 *  message type
 */
typedef struct _tch_msgQue_def_t {
	uint32_t queue_sz;               ///< number of elements in the queue
	uint32_t item_sz;                ///< size of item
	void* pool;                      ///< memory array for msgs
} tch_msgQueDef_t;

typedef void* tch_msgQue_id;

/***
 * mail type
 */
typedef struct _tch_mailQue_def_t{
	uint32_t queue_sz;               ///< number of elements in the queue
	uint32_t item_sz;                ///< size of an item
	void* pool;                      ///< memeory array for mails
	void* queue;
} tch_mailQueDef_t;

typedef void* tch_mailQue_id;


/**
 *    CMSIS RTOS Compatible message queue
 */
/*
struct _tch_msgque_ix_t {
	tch_msgQue_id (*create)(const tch_msgQueDef_t* que);

	tchStatus (*put)(tch_msgQue_id,uint32_t msg,uint32_t millisec);

	osEvent (*get)(tch_msgQue_id,uint32_t millisec);

	tchStatus (*destroy)(tch_msgQue_id);
};*/

/*
struct _tch_mailbox_ix_t {
	tch_mailQue_id (*create)(const tch_mailQueDef_t* que);
	void* (*alloc)(tch_mailQue_id qid,uint32_t millisec);
	void* (*calloc)(tch_mailQue_id qid,uint32_t millisec);
	tchStatus (*put)(tch_mailQue_id qid,void* mail);
	osEvent (*get)(tch_mailQue_id qid,uint32_t millisec);
	tchStatus (*free)(tch_mailQue_id qid,void* mail);
};*/

#if defined(__cplusplus)
}
#endif
#endif /* TCH_IPC_H_ */
