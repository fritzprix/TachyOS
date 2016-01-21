/*
 * tch_klog.c
 *
 *  Created on: 2015. 10. 23.
 *      Author: innocentevil
 */


#include "kernel/tch_kernel.h"
#include "kernel/tch_klog.h"
#include <stdarg.h>

#define KLOG_FILE_SIZE				((uint32_t) 0x0400)
#define KLOG_BUFFER_SIZE			((uint32_t) 256)

static char log_file[KLOG_FILE_SIZE];
static char log_buffer[KLOG_BUFFER_SIZE];
static uint32_t p_idx;
static uint32_t update_cnt;
static char* args_type[32];
static uint32_t arg_cnt;

static void __klog(const char* fmt,va_list va);

void tch_klog_init(file_operations_t* log_fio)
{
	mset(log_file,0,sizeof(log_file));
	mset(log_buffer,0,sizeof(log_buffer));
	mset(args_type,0,sizeof(args_type));

	update_cnt = 0;
	p_idx =  0;
	arg_cnt = 0;
}

void tch_klog_print(const char* fmt,...)
{
	if(!fmt)
		return;
	va_list arg_list;
	va_start(arg_list,fmt);
	if(tch_port_isISR())
	{
		__klog(fmt,arg_list);
	}
	else
	{
		tch_port_disableISR();
		__klog(fmt,arg_list);
		tch_port_enableISR();
	}
}

void tch_klog_flush(BOOL force)
{

}


static void __klog(const char* fmt,va_list va){

	size_t lsz = vformat(log_buffer,fmt,va);
	update_cnt += lsz;
	if((p_idx + lsz) > KLOG_FILE_SIZE)
	{
		mcpy(&log_file[p_idx],log_buffer,KLOG_FILE_SIZE - p_idx);						// copy part of buffer at the last area of log file
		mcpy(log_file,log_buffer[KLOG_FILE_SIZE - p_idx],p_idx + lsz - KLOG_FILE_SIZE);	// and the rest part of the buffer is copied into head of log file
		p_idx = p_idx + lsz;
	}
	else
	{
		mcpy(&log_file[p_idx],log_buffer,lsz);
		p_idx += lsz;
	}
	update_cnt &= (KLOG_FILE_SIZE - 1);			// can not bigger than log file size
	p_idx &= (KLOG_FILE_SIZE - 1);
}

