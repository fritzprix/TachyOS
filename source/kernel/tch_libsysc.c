/*
 * tch_clibsysc.c
 *
 *  Created on: 2014. 7. 23.
 *      Author: innocentevil
 *
 */


/*  Implementation of Basic Set of  Unix System Call Stub (for Using C Standard library)  */
/*
 *
 */



#include "tch_kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/reent.h>
#include <reent.h>
#include <errno.h>

#undef errno
#undef __getreent
extern int errno;

char* __env[1] = {0};
char** environ = __env;

__attribute__((section(".data")))static char* heap_end = NULL;



void* _sbrk_r(struct _reent* reent,ptrdiff_t incr){
	if(heap_end == NULL)
		heap_end = (char*)&Sys_Heap_Base;
	char *prev_heap_end;
	prev_heap_end = heap_end;
	if ((uint32_t)heap_end + incr > (uint32_t) &Sys_Heap_Limit) {
		if(!tch_port_isISR()){
			Thread->terminate(tch_currentThread,tchErrorNoMemory);
		}
		return NULL;
	}
	heap_end += incr;
	return prev_heap_end;
}

_ssize_t _write_r(struct _reent * reent, int fd, const void * buf, size_t cnt){
	switch(fd){
	case STDIN_FILENO:
	case STDERR_FILENO:
	case STDOUT_FILENO:
		if(tch_board->bd_stdio->write){
			tch_board->bd_stdio->write(buf,cnt);
			return cnt;
		}
	}
	return -1;
}

int _close_r(struct _reent* reent, int fd){
	return -1;
}

off_t _lseek_r(struct _reent* reent,int fd, off_t pos, int whence){
	return 0;
}

/**	tchStatus (*getc)(tch_usartHandle handle,uint8_t* rc,uint32_t timeout);
 *
 */
_ssize_t _read_r(struct _reent* reent,int fd, void *buf, size_t cnt){
	uint8_t* bbuf = (uint8_t*) buf;
	switch(fd){
	case STDERR_FILENO:
	case STDOUT_FILENO:
		return -1;
	case STDIN_FILENO:
		if(tch_board->bd_stdio->read)
			return tch_board->bd_stdio->read(buf,cnt);
	}
	return -1;
}

int _fork_r(struct _reent* reent){
	return 0;
}

int _wait_r(struct _reent* reent, int *status){
	return 0;
}

int _stat_r(struct _reent* reent,const char* file, struct stat* pstat){
	return 0;
}

int _fstat_r(struct _reent* reent,int fd, struct stat* pstat){
	*((uint32_t*)reent) = EINVAL;
	return 0;
}

int _link_r(struct _reent* reent,const char *old, const char *new){
	errno = EMLINK;
	return -1;
}

int _unlink_r(struct _reent* reent, const char *file){
	return -1;
}

int _isatty_r(struct _reent* reent, int fd){
	return 1;
}

void exit(int code){
	Thread->terminate(tch_currentThread,code);
	while(TRUE);
}

void abort(void){
	while(1);
}

int _open_r(struct _reent* reent,const char *file, int flags, int mode){
	return -1;
}

int _getpid(void){
	return -1;
}


time_t _times(void* _reent){
	return (time_t) 0 ;
}

int _gettimeofday(void* _reent){
	return -1;
}

