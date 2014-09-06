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

/*! \brief condition variable identifier
 */
typedef void* tch_condvId;

/*!
 * \brief posix-like condition variable API struct
 */
struct _tch_condvar_ix_t {

	/*! \brief create posix-like condition variable
	 *  \return initiated \ref tch_condvId
	 */
	tch_condvId (*create)();
	/*! \brief wait on condition variable with unlocking mutex
	 *  \param[in] condv
	 *  \param[in] lock
	 *  \param[in] timeout
	 */
	tchStatus (*wait)(tch_condvId condv,tch_mtxId lock,uint32_t timeout);
	tchStatus (*wake)(tch_condvId condv);
	tchStatus (*wakeAll)(tch_condvId condv);
	tchStatus (*destroy)(tch_condvId condv);
};



#if defined(__cplusplus)
}
#endif

/**@}*/

#endif /* TCH_CONDV_H_ */
