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

static char log_file[KLOG_FILE_SIZE];
static uint32_t p_idx;
static uint32_t update_cnt;
static char* args_type[32];
static uint32_t arg_cnt;


static void __klog(const char* fmt,va_list va);

void tch_klog_init(void){
	mset(log_file,0,sizeof(log_file));
	mset(args_type,0,sizeof(args_type));
	update_cnt = 0;
	p_idx =  0;
	arg_cnt = 0;
}

void tch_klog_print(const char* fmt,...){
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

size_t tch_klog_dump(char* rb)
{
	uint32_t idx;
	int offset;
	if(!rb)
		return 0;
	offset = p_idx - update_cnt;
	idx = 0;
	if(offset < 0)
	{
		offset = KLOG_FILE_SIZE - offset;
	}
	while(update_cnt--)
	{
		rb[idx++] = log_file[offset++];
		idx &= (KLOG_FILE_SIZE - 1);
		offset &= (KLOG_FILE_SIZE - 1);
	}
	return idx;
}

static void __klog(const char* fmt,va_list va){
	/*
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
	p_idx &= (KLOG_FILE_SIZE - 1);*/
}

