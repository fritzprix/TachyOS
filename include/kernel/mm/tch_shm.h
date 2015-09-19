/*
 * tch_shm.h
 *
 *  Created on: 2015. 7. 19.
 *      Author: innocentevil
 */

#ifndef TCH_SHM_H_
#define TCH_SHM_H_

#include "tch_ktypes.h"
#include "tch_mm.h"

extern void tch_shmInit(int seg_id);
extern void* tch_shmAlloc(size_t sz);
extern tchStatus tch_shmFree(void* mchunk);
extern tchStatus tch_shmCleanup(tch_threadId tid);
extern void tch_shmStat(mstat* stat);


#endif /* TCH_SHM_H_ */
