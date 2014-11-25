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

typedef struct tch_bar_cb_t{
	tch_uobj                 __obj;
	uint32_t                 state;
	tch_lnode_t              wq;
} tch_barCb;


extern void tch_initBar(tch_barCb* bar);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_BAR_H_ */
