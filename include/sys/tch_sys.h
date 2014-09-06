/*
 * tch_sys.h
 *
 *  Created on: 2014. 9. 6.
 *      Author: innocentevil
 */

#ifndef TCH_SYS_H_
#define TCH_SYS_H_

#if defined(__cplusplus)
extern "C"{
#endif

typedef struct _tch_msgq_karg_t{
	uword_t     msg;
	uint32_t    timeout;
}tch_msgq_karg;

extern tchStatus tch_msgq_kput(tch_msgQue_id,tch_msgq_karg* arg);
extern tchStatus tch_msgq_kget(tch_msgQue_id,tch_msgq_karg* arg);
extern tchStatus tch_msgq_kdestroy(tch_msgQue_id);

#if defined(__cplusplus)
}
#endif


#endif /* TCH_SYS_H_ */
