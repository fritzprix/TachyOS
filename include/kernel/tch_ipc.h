/*
 * tch_ipc.h
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_IPC_H_
#define TCH_IPC_H_

/***
 *  message type
 */
typedef struct _tch_msgQue_def_t tch_msgQueDef_t;
typedef void* tch_msgQue_id;

/***
 * mail type
 */
typedef struct _tch_mailQue_def_t tch_mailQueDef_t;
typedef void* tch_mailQue_id;


/**
 *    CMSIS RTOS Compatible message queue
 */

struct _tch_msgque_ix_t {
	tch_msgQue_id (*initMsgQ)(const tch_msgQueDef_t* que);
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
