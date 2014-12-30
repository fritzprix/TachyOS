/*
 * tch_pmgr.h
 *
 *  Created on: 2014. 12. 29.
 *      Author: manics99
 */

#ifndef TCH_PMGR_H_
#define TCH_PMGR_H_

#if defined(__cplusplus)
extern "C"{
#endif


typedef struct _tch_pwrMgr_ix {
	tchStatus (*waitOnSleep)(uint32_t wkup_interval);
	tchStatus (*cancelWaitOnSleep)(void);
}tch_pwrMgr_ix;

extern tch_pwrMgr_ix* tch_pwrMgrInit(const tch* env);
extern BOOL tch_pwrMgrWakeupHandler(time_t* tm);


#if defined(__cplusplus)
}
#endif


#endif /* TCH_PMGR_H_ */
