/*
 * tch_idle.h
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */

#ifndef TCH_IDLE_H_
#define TCH_IDLE_H_


extern void tch_idleInit();
extern void tch_kernelSetBusyMark();
extern void ttch_kernelClrBusyMark();
extern BOOL tch_kernelIsBusy();

#endif /* TCH_IDLE_H_ */
