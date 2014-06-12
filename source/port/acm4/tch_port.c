/*
 * tch_port.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */


static void tch_acm4_kernel_lock(void);
static void tch_acm4_kernel_unlock(void);
static void tch_acm4_switchContext(void* nth,void* cth) __attribute__((always_naked));
static void tch_acm4_saveContext(void* cth) __attribute__((always_naked));
static void tch_acm4_restoreContext(void* cth) __attribute__((always_naked));
static int tch_acm4_enterSvFromUsr(int sv_id,void* arg1,void* arg2);
static int tch_acm4_enterSvFromIsr(int sv_id,void* arg1,void* arg2);



static tch_port_ix _PORT_OBJ = {
		tch_acm4_kernel_lock,
		tch_acm4_kernel_unlock,
		tch_acm4_switchContext,
		tch_acm4_saveContext,
		tch_acm4_restoreContext,
		tch_acm4_enterSvFromUsr,
		tch_acm4_enterSvFromIsr
};
















void tch_acm4_kernel_lock(void){

}


void tch_acm4_kernel_unlock(void){

}


void tch_acm4_switchContext(void* nth,void* cth) __attribute__((always_naked)){

}

void tch_acm4_saveContext(void* cth) __attribute__((always_naked)){

}

void tch_acm4_restoreContext(void* cth) __attribute__((always_naked)){

}

int tch_acm4_enterSvFromUsr(int sv_id,void* arg1,void* arg2){

}

int tch_acm4_enterSvFromIsr(int sv_id,void* arg1,void* arg2){

}

