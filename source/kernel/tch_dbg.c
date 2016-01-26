/*
 * tch_dbg.c
 *
 *  Created on: 2016. 1. 25.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_dbg.h"
#include "kernel/tch_lwtask.h"
#include "kernel/tch_err.h"
#include <stdarg.h>

#define DBG_LOG_FILE_SIZE				((uint32_t) 0x0400)
#define DBG_BUFFER_SIZE					((uint32_t) 256)

struct dbg_info {
	dbg_level 		lvl;
	tchStatus		err_code;
};

static char dbg_log_file[DBG_LOG_FILE_SIZE];
static char dbg_log_buffer[DBG_BUFFER_SIZE];

static file* dbg_log_fio;
static uint32_t p_idx;
static uint32_t c_idx;
static uint32_t update_cnt;
static int flush_tskId;

static DECLARE_LWTASK(dbg_flush);


DECLARE_SYSCALL_3(dbg_print,struct dbg_info* , const char*, va_list*, tchStatus);


static __USER_API__ void tch_dbg_print(dbg_level level,tchStatus code, const char* fmt, ...);


__USER_RODATA__ tch_dbg_api_t Debug_IX = {
		.Normal = 0,
		.Warn = 1,
		.Error = 2,
		.print = tch_dbg_print
};

__USER_RODATA__ const tch_dbg_api_t* Debug = &Debug_IX;

DEFINE_SYSCALL_3(dbg_print,struct dbg_info*,info, const char*, format, va_list*, va, tchStatus)
{
	if(update_cnt > (DBG_LOG_FILE_SIZE >> 1))
		tch_dbg_flush();
	size_t lsz = vformat(dbg_log_buffer, format, *va);
	if((p_idx + lsz) >= DBG_LOG_FILE_SIZE)
	{
		mcpy(&dbg_log_file[p_idx],dbg_log_buffer, DBG_LOG_FILE_SIZE - p_idx );
		mcpy(dbg_log_file, &dbg_log_buffer[DBG_LOG_FILE_SIZE - p_idx],p_idx + lsz - DBG_LOG_FILE_SIZE);
	}
	else
	{
		mcpy(&dbg_log_file[p_idx],dbg_log_buffer, lsz);
	}
	update_cnt += lsz;
	p_idx += lsz;
	update_cnt &= (DBG_LOG_FILE_SIZE - 1);
	p_idx &= (DBG_LOG_FILE_SIZE - 1);
	if(info->lvl == Debug->Error)
		Thread->exit(current, info->err_code);
	return tchOK;
}

void tch_dbg_init(file* dbg_fio)
{
	if(!dbg_fio)
		KERNEL_PANIC("No I/O device for debug available");

	mset(dbg_log_file, 0 , sizeof(dbg_log_file));
	mset(dbg_log_buffer, 0, sizeof(dbg_log_buffer));

	// initialize I/O device for Debug log output
	dbg_log_fio = dbg_fio;
	dbg_log_fio->ops->open(dbg_log_fio);

	// register Debug I/O flushing task
	flush_tskId = tch_lwtsk_registerTask(dbg_flush,TSK_PRIOR_HIGH);

	p_idx = 0;
	c_idx = 0;
	update_cnt = 0;

}

void tch_dbg_flush(void)
{
	if(!update_cnt)
		return;
	tch_lwtsk_request(flush_tskId, NULL ,FALSE);
}


static __USER_API__ void tch_dbg_print(dbg_level level, tchStatus code, const char* fmt, ...)
{
	if(update_cnt >= (DBG_LOG_FILE_SIZE) >> 1)
		tch_dbg_flush();

	struct dbg_info info;
	info.err_code = code;
	info.lvl = level;

	va_list arg_list;
	va_start(arg_list, fmt);
	if(tch_port_isISR())
	{
		__dbg_print(&info, fmt,  &arg_list);
	}
	else
	{
		__SYSCALL_3(dbg_print, &info, fmt, (uword_t) &arg_list);
	}
}

static DECLARE_LWTASK(dbg_flush)
{
	if(!update_cnt)
		return;
	uint32_t cnt;
	tch_port_atomicBegin();
	cnt = update_cnt;
	update_cnt = 0;
	tch_port_atomicEnd();

	if((c_idx + cnt) >= DBG_LOG_FILE_SIZE)
	{
		dbg_log_fio->ops->write(dbg_log_fio, &dbg_log_file[c_idx], DBG_LOG_FILE_SIZE - c_idx);
		dbg_log_fio->ops->write(dbg_log_fio, dbg_log_file, c_idx + cnt - DBG_LOG_FILE_SIZE);
	}
	else
	{
		dbg_log_fio->ops->write(dbg_log_fio,&dbg_log_file[c_idx], cnt);
	}
	c_idx += cnt;
	c_idx &= (DBG_LOG_FILE_SIZE - 1);
}
