/*!
 * \defgroup TCH_CONDV tch_condv
 * @{ tch_condv.h
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_CONDV_H_
#define TCH_CONDV_H_

#include "tch_Typedef.h"


#if defined(__cplusplus)
extern "C"{
#endif

typedef struct _tch_condv_cb_t tch_condvCb;
extern tch_condvId tch_condvInit(tch_condvCb* condv,BOOL is_static);



#if defined(__cplusplus)
}
#endif

/**@}*/

#endif /* TCH_CONDV_H_ */
