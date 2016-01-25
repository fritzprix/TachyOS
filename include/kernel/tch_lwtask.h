/*
 * tch_lwtask.h
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */

#ifndef TCH_LWTASK_H_
#define TCH_LWTASK_H_


#if defined(__cpluspluse)
extern "C" {
#endif


#define DECLARE_LWTASK(fn)                  void fn(int id,const tch_core_api_t* env,void* arg)
typedef void (*lwtask_routine)(int id,const tch_core_api_t* env,void* arg);
typedef uint8_t tsk_prior_t;

#define TSK_PRIOR_REALTIME		((uint8_t) 6)
#define TSK_PRIOR_HIGH			((uint8_t) 5)
#define TSK_PRIOR_NORMAL		((uint8_t) 4)
#define TSK_PRIOR_LOW			((uint8_t) 3)
#define TSK_PRIOR_NG			((uint8_t) 2)

extern void __lwtsk_start_loop(void);
extern void __lwtsk_init(void);

int tch_lwtsk_registerTask(lwtask_routine fnp,tsk_prior_t prior);
void tch_lwtsk_unregisterTask(int tsk_id);

void tch_lwtsk_request(int tsk_id,void* arg, BOOL canblock);
void tch_lwtsk_cancel(int tsk_id);


#if defined(__cpluseplus)
}
#endif


#endif /* TCH_LWTASK_H_ */
