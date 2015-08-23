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
	rb_treeNode_t			rbnode;					// id -> object searching
	tch_kobjDestr		__destr_fn;
};

extern int tch_registerKobject(tch_kobj* obj);
extern tchStatus tch_unregisterKojbect(int id);
extern tch_kobj* tch_obtainKobject(int id);




#if defined(__cplusplus)
}
#endif

#endif /* TCH_KOBJ_H_ */
