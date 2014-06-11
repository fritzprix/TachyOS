/*
 * atcmd_bt.h
 *
 *  Created on: 2014. 6. 1.
 *      Author: innocentevil
 */

#ifndef ATCMD_BT_H_
#define ATCMD_BT_H_

#include "../core/port/tch_stdtypes.h"
#include "../core/fmo_lldabs.h"
#include "../lld/fmo_gpio.h"

typedef struct _atcmd_bt_bdc_t atcmd_bt_bdc;
typedef enum {STANDBY,PEND,CONNECT} atcmd_bt_status;
typedef uint64_t atcmd_bt_addr;
typedef struct _atcmd_bt_device_t atcmd_bt_device;
typedef struct _atcmd_bt_server_instance_t atcmd_bt_server_instance;
typedef struct _atcmd_bt_server_adapter_t atcmd_bt_server_adapter;


struct _atcmd_bt_bdc_t {
	tch_gpio_t    ctrl_port;
	tch_gpio_t    ev_port;
	uint8_t       ctrl_pin;
	uint8_t       ev_pin;
};

struct _atcmd_bt_device_t {
	const char* (*getAddress)(atcmd_bt_device* device);
	const char* (*getName)(atcmd_bt_device* device);
	uint16_t (*getRssi)(atcmd_bt_device* device);
	BOOL (*connect)(atcmd_bt_device* device);
	BOOL (*disconnect)(atcmd_bt_device* device);
	tch_istream* (*openInputStream)(atcmd_bt_device* device);
	tch_ostream* (*openOutputStream)(atcmd_bt_device* device);
};


struct _atcmd_bt_server_adapter_t {
	BOOL (*onConnected)(atcmd_bt_device* device);
	BOOL (*onDisconnected)(atcmd_bt_device* device);
};

struct _atcmd_bt_server_instance_t {
	void (*close)();
};


atcmd_bt_server_instance* tch_hld_atcmdBt_openServer(const char* deviceName,uint32_t baudrate,atcmd_bt_server_adapter* adapter,tch_pwrMgrCfg pcfg);

#endif /* ATCMD_BT_H_ */
