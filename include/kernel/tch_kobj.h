/*
 * tch_kobj.h
 *
 *  Created on: 2015. 8. 22.
 *      Author: innocentevil
 */




#ifndef TCH_KOBJ_H_
#define TCH_KOBJ_H_

#include "tch.h"
#include "tch_types.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct tch_kobject_t tch_kobj;		//<<< kernel object type
typedef tchStatus (*tch_kobjDestr)(tch_kobj* obj);

struct tch_kobject_t {
	dlistNode_t         lhead;		// id -> object searching
	tch_kobjDestr		__destr_fn;
};

extern tchStatus tch_registerKobject(tch_kobj* obj, tch_kobjDestr destfn);
extern tchStatus tch_unregisterKobject(tch_kobj* obj);
extern tchStatus tch_destroyAllKobjects();

#if defined(__cplusplus)
}
#endif

#endif /* TCH_KOBJ_H_ */
