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

extern void tch_klog_init(void);
extern void tch_klog(const char*,...);
extern void tch_klog_flush(struct tch_file* fp);


#if defined(__cplusplus)
}
#endif



#endif /* TCH_KLOG_H_ */
