/*
 * tch_klog.h
 *
 *  Created on: 2015. 10. 23.
 *      Author: innocentevil
 */

#ifndef TCH_KLOG_H_
#define TCH_KLOG_H_

#include "kernel/tch_fs.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern void tch_klog_init(file_operations_t* log_fio);
extern void tch_klog_print(const char*,...);
extern void tch_klog_flush(BOOL force);


#if defined(__cplusplus)
}
#endif



#endif /* TCH_KLOG_H_ */
