/*
 * tch_ipc.h
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_IPC_H_
#define TCH_IPC_H_


#define TCH_MSGQ_HEAD_SIZE             (sizeof(uint32_t) * 2 + sizeof(void*) + 4 * sizeof(void*))
#define TCH_MAILQ_HEAD_SIZE

#define tch_msgQDef(name,size)\
uint8_t msgQ_##name_pool[size * sizeof(void*) + TCH_MSGQ_HEAD_SIZE];\
static tch_msgQueDef_t msgQ_##name = {size,sizeof(void*),msgQ_##name_pool + TCH_MSGQ_HEAD_SIZE}


#define tch_mailQDef(name,size,type)\
uint8_t mailQ_##name_pool[size + sizeof(type) + TCH_MAILQ_HEAD_SIZE];\
static tch_mailQueDef_t mailQ_##name = {size,sizeof(type),mailQ_##name_pool}

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
} tch_mailQueDef_t;

typedef void* tch_mailQue_id;


/**
 *    CMSIS RTOS Compatible message queue
 */

struct _tch_msgque_ix_t {
	tch_msgQue_id (*create)(const tch_msgQueDef_t* que);
	osStatus (*put)(tch_msgQue_id,uint32_t msg,uint32_t millisec);
	osEvent (*get)(tch_msgQue_id,uint32_t millisec);
};

struct _tch_mailbox_ix_t {
	tch_mailQue_id (*create)(const tch_mailQueDef_t* que);
	void* (*alloc)(tch_mailQue_id qid,uint32_t millisec);
	void* (*calloc)(tch_mailQue_id qid,uint32_t millisec);
	osStatus (*put)(tch_mailQue_id qid);
	osEvent (*get)(tch_mailQue_id qid,uint32_t millisec);
	osStatus (*free)(tch_mailQue_id qid,void* mail);
};


#endif /* TCH_IPC_H_ */
