/*
 * tch_lock.h
 *
 *  Created on: 2015. 10. 31.
 *      Author: innocentevil
 */

#ifndef TCH_LOCK_H_
#define TCH_LOCK_H_

#include "cdsl_dlist.h"
#include "kernel/tch_ktypes.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct unlockable lock_t;
typedef tchStatus (*unlock_fn)(lock_t* );

struct unlockable {
	dlistNode_t         lock_ln;
	unlock_fn			unlock_fn;
};

extern tchStatus tch_lock_add(lock_t* lock,unlock_fn fn);
extern tchStatus tch_lock_remove(lock_t* lock);
extern tchStatus tch_lock_unlock(lock_t* lock);
extern tchStatus tch_lock_force_release(dlistEntry_t* lock_list);



#if defined(__cplusplus)
}
#endif



#endif /* TCH_LOCK_H_ */
