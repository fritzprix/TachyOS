/*
 * tch_kobj.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2015. 8. 22.
 *      Author: innocentevil
 */


#include "kernel/tch_kobj.h"
#include "kernel/tch_kernel.h"
#include "kernel/util/cdsl_dlist.h"



tchStatus tch_registerKobject(tch_kobj* obj, tch_kobjDestr destfn){
	if(!obj || !destfn)
		return tchErrorParameter;
	cdsl_dlistInit(&obj->lhead);
	obj->__destr_fn = destfn;
	cdsl_dlistPutTail(&current_mm->kobj_list,&obj->lhead);
	return tchOK;
}

tchStatus tch_unregisterKobject(tch_kobj* obj){
	if(!obj)
		return tchErrorParameter;

	if(!obj->lhead.next && !obj->lhead.prev)
		return tchErrorParameter;

	cdsl_dlistRemove(&obj->lhead);
	obj->lhead.next = obj->lhead.prev = NULL;
	return tchOK;
}


tchStatus tch_destroyAllKobjects(){
	while(!cdsl_dlistIsEmpty(&current_mm->kobj_list)){
		tch_kobj* obj = cdsl_dlistDequeue(&current_mm->kobj_list);
		obj = container_of(obj,tch_kobj,lhead);
		obj->__destr_fn(obj);
	}
	return tchOK;
}


tchStatus __tch_noop_destr(tch_kobj* obj){return tchOK; }
