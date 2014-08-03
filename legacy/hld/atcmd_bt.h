/*
 * atcmd_bt.h
 *
 *  Created on: 2014. 6. 1.
 *      Author: innocentevil
 */

#ifndef ATCMD_BT_H_
#define ATCMD_BT_H_

typedef struct _atcmd_bt_bdc_t atcmd_bt_bdc;
typedef struct _atcmd_bt_instance_t atcmd_bt_instance;

struct _atcmd_bt_instance_t {
	BOOL (*setDeviceName)(atcmd_bt_instance* self,const char* name);
};

#endif /* ATCMD_BT_H_ */
