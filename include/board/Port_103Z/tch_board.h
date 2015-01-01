/*
 * tch_board.h
 *
 *  Created on: 2014. 12. 9.
 *      Author: innocentevil
 */

#ifndef TCH_BOARD_H_
#define TCH_BOARD_H_


#include "tch.h"

#if defined(__cplusplus)
extern "C"{
#endif

extern tchStatus tch_boardInit(const tch* env);
extern tchStatus tch_boardDeinit(const tch* env);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_BOARD_H_ */
