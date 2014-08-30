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

typedef void* tch_barId;
typedef struct tch_bar_def_t{
	uint8_t                 status;
	tch_lnode_t             wq;
} tch_barDef;

struct _tch_bar_ix_t {
	tch_barId (*create)(tch_barDef* bar);
	tchStatus (*wait)(tch_barDef* bar,uint32_t timeout);
	tchStatus (*signal)(tch_barDef* bar,tchStatus result);
	tchStatus (*destroy)(tch_barDef* bar);
};

#if defined(__cplusplus)
}
#endif

#endif /* TCH_BAR_H_ */
