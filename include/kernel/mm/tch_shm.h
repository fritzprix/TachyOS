/*
 * tch_shm.h
 *
 *  Created on: 2015. 7. 19.
 *      Author: innocentevil
 */

#ifndef TCH_SHM_H_
#define TCH_SHM_H_

#include "tch_ktypes.h"

extern void tch_shmInit(int seg_id);


extern void* tchk_shmalloc(size_t sz);
extern void tchk_shmFree(void* mem);
extern uint32_t tchk_shmAvail();
extern tchStatus tchk_shmCleanUp(struct tch_mm* owner);

extern void* tch_shmAlloc(size_t sz);
extern void tch_shmFree(void* mchunk);
extern uint32_t tch_shmAvali();


#endif /* TCH_SHM_H_ */
