/*
 * tch_crt0.c
 *
 *  Created on: Jun 27, 2015
 *      Author: innocentevil
 */


/*
 * tch_nclib.c
 *
 *  Created on: 2014. 7. 27.
 *      Author: manics99
 */

#include "tch_kernel.h"
#include "tch_nclib.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/reent.h>
#include <reent.h>
#include <errno.h>

#undef errno
#undef __getreent
int errno;


const tch_stdio_ix STDIO_StaticObj =  {
		.getchar = getchar,
		.perror = perror,
		.putchar = putchar,
		.puts = puts,
		.gets = gets,
		.siscanf = siscanf,
		.siprintf = siprintf,
		.iprintf = iprintf,
		.iscanf = iscanf
};

const tch_stdlib_ix STDLIB_StaticObj = {
		.abs = abs,
		.atof = atof,
		.atoff = atoff,
		.atoi = atoi,
		.atol = atol,
		.calloc = calloc,
		.free = free,
		.malloc = malloc,
		.rand = rand,
		.srand = srand
};


const tch_ctype_ix CTYPE_StaticObj = {
		.isalnum = isalnum,
		.isalpha = isalpha,
		.isascii = isascii,
		.iscntrl = iscntrl,
		.isdigit = isdigit,
		.islower = islower,
		.isprint = isprint,
		.isspace = isspace,
		.isupper = isupper,
		.tolower = tolower,
		.toupper = toupper
};


const tch_string_ix STRING_StaticObj = {
		.memchr = memchr,
		.memcmp = memcmp,
		.memcpy = memcpy,
		.memmove = memmove,
		.memset = memset,
		.strcat = strcat,
		.strchr = strchr,
		.strlen =  strlen,
		.strspn = strspn,
		.strtok = strtok
};


const tch_time_ix TIME_StaticObj = {
		.asctime = asctime,
		.clock = clock,
		.difftime = difftime,
		.gmtime = gmtime,
		.mktime = mktime,
};


const tch_math_ix MATH_StaticObj = {
		.dummy = NULL
};

static tch_ulib_ix NLIB_Instance = {
		.stdio = &STDIO_StaticObj,
		.stdlib = &STDLIB_StaticObj,
		.ctype = &CTYPE_StaticObj,
		.string = &STRING_StaticObj,
		.time = &TIME_StaticObj,
		.math = &MATH_StaticObj
};
const tch_ustdlib_ix* uStdLib = &NLIB_Instance;


char* __env[1] = {0};
char** environ = __env;

__attribute__((section(".data")))static char* heap_end = NULL;
__attribute__((aligned(8))) static char LIBC_HEAP[6000];



static __FILE default_stdin;
static __FILE default_stdout;
static __FILE default_stderr;

tch_ustdlib_ix* tch_initCrt0(struct crt_param* param){
	return &NLIB_Instance;
}


/**
 * libc system call stub
 */

void* _sbrk_r(struct _reent* reent,ptrdiff_t incr){
	if(heap_end == NULL)
		//heap_end = (char*)&Sys_Heap_Base;
		heap_end = (char*) LIBC_HEAP;
	char *prev_heap_end;
	prev_heap_end = heap_end;
	if ((uint32_t)heap_end + incr > ((uint32_t) LIBC_HEAP + 6000 * sizeof(char))) {
		if(!tch_port_isISR()){
			tch_threadExit(current,tchErrorNoMemory);
		}
		return NULL;
	}
	heap_end += incr;
	return prev_heap_end;
}

_ssize_t _write_r(struct _reent * reent, int fd, const void * buf, size_t cnt){
	__FILE* fp = NULL;
	switch(fd) {
		case STDERR_FILENO:
		fp = current->reent._stderr;
		break;
		case STDOUT_FILENO:
		fp = current->reent._stdout;
		break;
		case STDIN_FILENO:
		fp = current->reent._stdin;
		break;
	}
	return fp->_write(reent,fp,buf,cnt);
}

int _close_r(struct _reent* reent, int fd){
	__FILE* fp = NULL;
	switch(fd){
	case STDERR_FILENO:
		fp = current->reent._stderr;
		break;
	case STDOUT_FILENO:
		fp = current->reent._stdout;
		break;
	case STDIN_FILENO:
		fp = current->reent._stdin;
		break;
	}
	return fp->_close(reent,fp);
}

off_t _lseek_r(struct _reent* reent,int fd, off_t pos, int whence){
	__FILE* fp = NULL;
	switch(fd){
	case STDERR_FILENO:
		fp = current->reent._stderr;
		break;
	case STDOUT_FILENO:
		fp = current->reent._stdout;
		break;
	case STDIN_FILENO:
		fp = current->reent._stdin;
		break;
	}
	return fp->_seek(reent,fp,pos,whence);
}

/**	tchStatus (*getc)(tch_usartHandle handle,uint8_t* rc,uint32_t timeout);
 *
 */
_ssize_t _read_r(struct _reent* reent,int fd, void *buf, size_t cnt){
	__FILE* fp = NULL;
	switch(fd){
	case STDERR_FILENO:
		fp = current->reent._stderr;
		break;
	case STDOUT_FILENO:
		fp = current->reent._stdout;
		break;
	case STDIN_FILENO:
		fp = current->reent._stdin;
		break;
	}
	return fp->_read(reent,fp->_cookie,buf,cnt);
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
	reent->_errno = EINVAL;
	return 0;
}

int _link_r(struct _reent* reent,const char *old, const char *new){
	reent->_errno = EMLINK;
	return -1;
}

int _unlink_r(struct _reent* reent, const char *file){
	return -1;
}

int _isatty_r(struct _reent* reent, int fd){
	return 1;
}

void exit(int code){
	tch_threadExit(current,code);
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


