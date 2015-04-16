/*
 * tch_bar.h
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */

#ifndef TCH_BAR_H_
#define TCH_BAR_H_

#include "tch_Typedef.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tch_bar_cb_t tch_barCb;

extern void tchk_barrierInit(tch_barCb* bar,BOOL is_static);
extern tchStatus tchk_barrierDeinit(tch_barCb* bar);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_BAR_H_ */
