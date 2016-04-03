/*
 * tch_kmod.c
 *
 *  Created on: 2015. 10. 3.
 *      Author: innocentevil
 */


#include "kernel/tch_kernel.h"
#include "kernel/tch_kmod.h"
#include "kernel/tch_err.h"
#include "kernel/string.h"



#define MODULE_FLAG_PRIV			((uint32_t) 1 << 0)
#define MODULE_FLAG_STATIC			((uint32_t) 1 << 1)
#define MODULE_FLAG_DYNAMIC			((uint32_t) 1 << 2)


typedef struct module_header {
	rb_treeNode_t 			rbn;
	int 					owner;
	uint32_t				flag;
	void*					uaccess_ix;
} module_header_t;


__USER_API__ void* kmod_usr_request(int);
__USER_API__ BOOL kmod_usr_chkdep(module_map_t* map);

static rb_treeNode_t* module_root = NULL;
static module_map_t	module_map = {0};

static void* _request_kmod(int type,BOOL ispriv);

__USER_RODATA__ tch_module_api_t Module_IX = {
		.request = kmod_usr_request,
		.chkdep = kmod_usr_chkdep
};

__USER_RODATA__ const tch_module_api_t* Module = &Module_IX;


DECLARE_SYSCALL_1(kmod_usr_request,int,void*);
DECLARE_SYSCALL_1(kmod_usr_chkdep,module_map_t* ,tchStatus);


DEFINE_SYSCALL_1(kmod_usr_request,int,type,void*) {
	return _request_kmod(type,FALSE);
}

DEFINE_SYSCALL_1(kmod_usr_chkdep,module_map_t*, map,tchStatus){
	if(!map)
		return tchErrorParameter;
	if(mcmp(map,&module_map,sizeof(module_map_t)) != 0) {
		mcpy(map,&module_map,sizeof(module_map_t));
		return tchErrorResource;
	}
	return tchOK;
}

BOOL tch_kmod_init(void){
	initv_t* initv = (initv_t*) &_initv_begin;
	initv_t* initv_limit = (initv_t*) &_initv_end;
	while(initv < initv_limit){
		if(!(*initv)())
			return FALSE;
		initv++;
	}
	return TRUE;
}

void tch_kmod_exit(void){
	exitv_t* exitv = (exitv_t*) &_exitv_begin;
	exitv_t* exitv_limit = (exitv_t*) &_exitv_end;
	while(exitv < exitv_limit){
		(*exitv)();
		exitv++;
	}
}

BOOL tch_kmod_register(int type,int owner, __UACESS void* interface,BOOL  ispriv) {

	int idx = type /64;
	int offset = type % 64;
	if((module_map._map[idx] >> offset) & 1)
		return FALSE;			// there is a registered module interface for given type

	module_header_t* module = (module_header_t*) kmalloc(sizeof(module_header_t));
	if(!module)
		return FALSE;

	module_map._map[idx] |= (1 << offset);
	module->flag = MODULE_FLAG_STATIC;
	module->owner = owner;
	module->uaccess_ix = interface;
	cdsl_rbtreeNodeInit(&module->rbn,type);
	module->flag |= ispriv? MODULE_FLAG_PRIV : 0;

	return cdsl_rbtreeInsert(&module_root,&module->rbn);
}

BOOL tch_kmod_unregister(int type,int owner){

	int idx = type / 64;
	int offset = type % 64;
	if(!((module_map._map[idx] >> offset) & 1))
		return FALSE;						// there is a registered module interface for given type
	module_header_t* module = (module_header_t*) cdsl_rbtreeDelete(&module_root,type);
	if(!module)
		return FALSE;

	module = container_of(module,module_header_t,rbn);
	if(module->owner != owner)
		return FALSE;

	module_map._map[idx] &= ~(1 << offset); 		// clear module map
	kfree(module);							// free module header
	return TRUE;
}

BOOL tch_kmod_chkdep(const module_map_t* map){
	return mcmp(map,&module_map,sizeof(module_map_t)) == 0;
}

void* tch_kmod_request(int type){
	return _request_kmod(type,TRUE);
}

static void* _request_kmod(int type,BOOL ispriv){
	int idx = type / 64;
	int offset = type % 64;
	if(!((module_map._map[idx] >> offset) & 1))
		return NULL;
	module_header_t* module = (module_header_t*) cdsl_rbtreeLookup(&module_root,type);
	if(!module)
		KERNEL_PANIC("module map is corrupted");
	module = container_of(module,module_header_t,rbn);
	if((module->flag & MODULE_FLAG_PRIV) && !ispriv)
		return NULL;
	return module->uaccess_ix;
}

__USER_API__ void* kmod_usr_request(int type){
	if(tch_port_isISR())
		return _request_kmod(type,TRUE);
	return (void*) __SYSCALL_1(kmod_usr_request,type);
}

__USER_API__ BOOL kmod_usr_chkdep(module_map_t* map){
	if(tch_port_isISR()){
		return __kmod_usr_chkdep(map) == tchOK;
	}else{
		return __SYSCALL_1(kmod_usr_chkdep,map) == tchOK;
	}
}

