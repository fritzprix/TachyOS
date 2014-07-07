/*
 * tch_ipc.h
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
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_IPC_H_
#define TCH_IPC_H_


/********************************************************************************/
/*   This Module Contains IPC Component for Tachyos. Basic IPC Features are     */
/*   listed below.                                                              */
/*                                                                              */
/*             Message Queue  : Uni Directional communication channel           */
/*                              which enables thread communicated to other      */
/*                              thread or ISR handler                           */
/*                                                                              */
/*             Mail Queue : similar to message queue but mail queue has its     */
/*                          own mem pool and typically transfer memory pointer  */
/*                          allocated from its memory pool                      */
/*                                                                              */
/*                                                                              */
/********************************************************************************/

/**
 * 	tch_mpool_id                  mpool_id;
	tch_msgQue_id                 msgq_id;
	tch_mailQueDef_t*             mailqDef;
 */
#define TCH_MSGQ_HEAD_SIZE             (sizeof(uint32_t) * 3 + sizeof(void*) + 4 * sizeof(void*))
#define TCH_MAILQ_HEAD_SIZE            (6 * sizeof(uint32_t) + 5 * sizeof(void*))

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

struct _tch_msgque_ix_t {
	/**
	 *  Create and initiate Message Queue instance
	 *  ##Note : In CMSIS, this function actually has one more arguement, which is thread. however it is pointless
	 *           because there isn't any operation which is dedicated to a given single thread
	 *
	 *  @arg1 message queue definition
	 *  @return queue ID
	 *
	 */
	tch_msgQue_id (*create)(const tch_msgQueDef_t* que);

	/**
	 * put message in the message queue (for producer side)
	 * ##Note : In ISR Mode, timeout is forced to 0 and execution is not blocked.
	 *
	 *  @arg1 message queue id
	 *  @arg2 msg to send
	 *  @arg3 timeout in millisecond
	 *  @return osOK  : the message is put into the queue.
	 *          osErrorResource  : no memory in the queue was available.
	 *          osErrorTimeoutResource  : no memory in the queue was available during the given time limit.
	 *          osErrorParameter  : a parameter is invalid or outside of a permitted
	 */
	osStatus (*put)(tch_msgQue_id,uint32_t msg,uint32_t millisec);

	/**
	 * Get a Message or Wait for a Message from a Queue
	 * ##Note : In ISR Mode, timeout is forced to 0 and execution is not blocked.
	 *
	 *   @arg1 message queue id
	 *   @arg2 timeout in millisecond
	 *   @return osOK: no message is available in the queue and no timeout was specified.
	 *           osEventTimeout: no message has arrived during the given timeout period.
	 *           osEventMessage: message received, value.p contains the pointer to message.
	 *           osErrorParameter: a parameter is invalid or outside of a permitted range.
	 */
	osEvent (*get)(tch_msgQue_id,uint32_t millisec);
};

struct _tch_mailbox_ix_t {
	tch_mailQue_id (*create)(const tch_mailQueDef_t* que);
	void* (*alloc)(tch_mailQue_id qid,uint32_t millisec);
	void* (*calloc)(tch_mailQue_id qid,uint32_t millisec);
	osStatus (*put)(tch_mailQue_id qid,void* mail);
	osEvent (*get)(tch_mailQue_id qid,uint32_t millisec);
	osStatus (*free)(tch_mailQue_id qid,void* mail);
};


#endif /* TCH_IPC_H_ */
