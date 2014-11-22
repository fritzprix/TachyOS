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



tch_UartHandle* stdio_port;


tchStatus tch_kernel_initCrt0(tch* env){
	// initialize standard i/o stream device
	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = env->Device->usart->Parity.Parity_Non;
	ucfg.StopBit = env->Device->usart->StopBit.StopBit1B;
	ucfg.UartCh = 2;
	stdio_port = env->Device->usart->allocUart(env,&ucfg,osWaitForever,ActOnSleep);
	return osOK;

}

uint32_t tch_kHeapAvail(void){
	return ((uint32_t) &Heap_Limit) - (uint32_t) heap_end;
}


void* _sbrk_r(struct _reent* reent,ptrdiff_t incr){
	if(heap_end == NULL)
		heap_end = (char*)&Heap_Base;
	char *prev_heap_end;
	prev_heap_end = heap_end;
	if ((uint32_t)heap_end + incr > (uint32_t) &Heap_Limit) {
		if(!tch_port_isISR()){
			Thread->terminate(tch_currentThread,osErrorNoMemory);
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
		stdio_port->write(stdio_port,buf,cnt);
		return cnt;
	}
	return -1;
}

int _close_r(struct _reent* reent, int fd){
	return -1;
}

off_t _lseek_r(struct _reent* reent,int fd, off_t pos, int whence){
	return 0;
}

_ssize_t _read_r(struct _reent* reent,int fd, void *buf, size_t cnt){
	return cnt;
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
	return (time_t) 0;
}

int _gettimeofday(void* _reent){
	return -1;
}

