/*
 * tch_idle.h
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */

#ifndef TCH_IDLE_H_
#define TCH_IDLE_H_


extern void idle_init();
extern void set_system_busy();
extern void clear_system_busy();
extern BOOL is_system_busy();

#endif /* TCH_IDLE_H_ */
