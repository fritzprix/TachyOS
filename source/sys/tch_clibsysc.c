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
#include <sys/stat.h>
#include <sys/times.h>
#include <errno.h>

#undef errno
extern int errno;

char* __env[1] = {0};
char** environ = __env;

__attribute__((section(".data")))static char* heap_end = NULL;


char* _sbrk_r(struct _reent* reent,size_t incr){
	if(heap_end == NULL)
		heap_end = (char*)&Heap_Base;
	char *prev_heap_end;
	prev_heap_end = heap_end;
	if ((uint32_t)heap_end + incr > (uint32_t) &Heap_Limit) {
		return NULL;
	}
	heap_end += incr;
	return prev_heap_end;
}

long _write_r(void* reent,int fd,const void* buf,size_t cnt){
	return cnt;
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

int _isatty_r(int file){
	return 1;
}

void exit(int code){
	while(1);
}

void abort(void){
	while(1);
}


int _open_r(void *reent,const char *file, int flags, int mode){
	return -1;
}

int _getpid(void){
	return -1;
}


time_t _times(void* _reent){
	return (time_t) 0;
}

int _gettimeofday(void* _reent){
	return -1;
}
