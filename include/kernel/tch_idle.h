/*
 * tch_idle.h
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */

#ifndef TCH_IDLE_H_
#define TCH_IDLE_H_


extern void idle_init();
extern void idle_set_busy();
extern void idle_clear_busy();
extern BOOL idle_is_busy();

#endif /* TCH_IDLE_H_ */
