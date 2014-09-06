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

struct _tch_bar_ix_t {
	tch_barId (*create)();
	tchStatus (*wait)(tch_barId bar,uint32_t timeout);
	tchStatus (*signal)(tch_barId bar,tchStatus result);
	tchStatus (*destroy)(tch_barId bar);
};

#if defined(__cplusplus)
}
#endif

#endif /* TCH_BAR_H_ */
