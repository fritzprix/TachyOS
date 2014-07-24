/*
 * tch_clibsysc.c
 *
 *  Created on: 2014. 7. 23.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

char* __env[1] = {0};
char** environ = __env;

char* _sbrk_r(void* reent,size_t incr){
	return Sys->tch_api.Mem->alloc(incr);
}

long _write_r(void* reent,int fd,const void* buf,size_t cnt){
	return cnt;
}

int open(const char* name,int flags,int mode){
	return -1;
}
int _close_r(void *reent, int fd){
	return -1;
}

off_t _lseek_r(void *reent,int fd, off_t pos, int whence){
	return 0;
}

long _read_r(void *reent,int fd, void *buf, size_t cnt){
	return cnt;
}

int _fork_r(void *reent){
	return 0;
}

int _wait_r(void *reent, int *status){
	return 0;
}

int _stat_r(void *reent,const char *file, void* pstat){
	return 0;
}

int _fstat_r(void *reent,int fd, void* pstat){
	*((uint32_t*)reent) = EINVAL;
	return 0;
}

int _link_r(void *reent,const char *old, const char *new){
	errno = EMLINK;
	return -1;
}

int _unlink_r(void *reent, const char *file){
	return -1;
}

void _free_r(struct _reent* reent,void* aptr){
	Sys->tch_api.Mem->free(aptr);
}


int _isatty_r(int file){
	return 1;
}


int getpid(void){
	return 1;
}

int kill(int pid,int sig){
	errno = EINVAL;
	return -1;
}

int fork(void){
	errno = EAGAIN;
	return -1;
}

