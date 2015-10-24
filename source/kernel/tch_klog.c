/*
 * tch_klog.c
 *
 *  Created on: 2015. 10. 23.
 *      Author: innocentevil
 */


#include "kernel/tch_kernel.h"
#include "kernel/tch_klog.h"
#include <stdarg.h>

#define KLOG_SIZE				512

static char log_buffer[1024];
static uint32_t c_idx;
static uint32_t p_idx;
static char* args_type[32];
static uint32_t arg_cnt;


static void __klog(const char* fmt,va_list va);

void tch_klog_init(void){
	mset(log_buffer,0,sizeof(log_buffer));
	mset(args_type,0,sizeof(args_type));
	p_idx = c_idx =  0;
	arg_cnt = 0;
}

void tch_klog(const char* fmt,...){
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

void tch_klog_flush(struct tch_file* fp){
	/*	ssize_t (*write)(struct tch_file* filp,const char* bp,size_t len);
	 */
	if(c_idx < p_idx)
	{

	}
}

static void __klog(const char* fmt,va_list va){
	if(p_idx == c_idx)
	{
		// force flush
	}
	p_idx += vformat(&log_buffer[p_idx],va);
	if(p_idx > KLOG_SIZE)
	{
		mcpy(log_buffer,&log_buffer[KLOG_SIZE - 1],(size_t) (p_idx - KLOG_SIZE));
		p_idx -= KLOG_SIZE;
	}
}





