/*
 * tch_msgq.h
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */

#ifndef TCH_MSGQ_H_
#define TCH_MSGQ_H_


#include "tch_kobj.h"

#if defined(__cplusplus)
extern "C"{
#endif



typedef struct _tch_msgq_cb {
	tch_kobj      __obj;
	uint32_t      status;
	void*         bp;
	uint32_t      sz;
	uint32_t      pidx;
	uint32_t      gidx;
	cdsl_dlistNode_t   cwq;
	cdsl_dlistNode_t   pwq;
	uint32_t      updated;
}tch_msgqCb;

extern tch_messageQ_api_t MsgQ_IX;
extern tchStatus tch_msgqInit(tch_msgqCb* mq,uint32_t* bp,uint32_t sz);
extern tchStatus tch_msgqDeinit(tch_msgqCb* mq);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_MSGQ_H_ */
