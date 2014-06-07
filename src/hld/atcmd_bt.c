/*
 * atcmd_bt.c
 *
 *  Created on: 2014. 6. 1.
 *      Author: innocentevil
 */

#include "atcmd_bt.h"
#include "../lld/fmo_gpio.h"
#include "../lld/fmo_usart.h"
#include "../core/fmo_synch.h"
#include "../core/fmo_lldabs.h"



/***
 *  public function
 */
static atcmd_bt_addr tch_hld_bt_device_getAddress(atcmd_bt_device* device);
static const char* tch_hld_bt_device_getName(atcmd_bt_device* device);
static uint16_t tch_hld_bt_device_getRssi(atcmd_bt_device* device);
static BOOL tch_hld_bt_device_connect(atcmd_bt_device* device);
static BOOL tch_hld_bt_device_disconnect(atcmd_bt_device* device);
static tch_istream* tch_hld_bt_device_openInputStream(atcmd_bt_device* device);
static tch_ostream* tch_hld_bt_device_openOutputStream(atcmd_bt_device* device);


static BOOL tch_hld_bt_server_setDeviceName(const char* name);
static const char* tch_hld_bt_server_getDeviceName();
static atcmd_bt_status tch_hld_bt_server_getStatus();
static atcmd_bt_addr tch_hld_bt_server_getRecentTargetAddr();
static BOOL tch_hld_bt_server_startListenToTarget(atcmd_bt_addr baddr,uint32_t to);
static BOOL tch_hld_bt_server_startListen(uint32_t to);
static void tch_hld_bt_server_close();



/***
 *
 *  private function
 */
static void tch_hld_btc_reset();


typedef struct _atcmd_bt_prototype_t atcmd_bt_prototype;

struct _atcmd_bt_prototype_t {
	uint16_t              status;
	tch_gpio_instance*    res_handle;
	tch_mtx_lock          lock;
	tch_iostream_buffer   istream_buffer;
};



static atcmd_bt_bdc BT_BDC = {
		GPIO_A,
		4
};

static atcmd_bt_prototype BT_StaticInstance = {
		0,
		NULL,
		MTX_INIT,
		IOSTREAM_INIT
};



atcmd_bt_server_instance* tch_hld_atcmdBt_openServer(const char* deviceName,uint32_t baudrate,atcmd_bt_server_adapter* adapter,tch_pwrMgrCfg pcfg){


	atcmd_bt_prototype* ins = (atcmd_bt_prototype*) &BT_StaticInstance;

	/**
	 * Initialize Ctrl IO
	 */
	tch_gpio_cfg iocfg;
	tch_lld_gpio_cfgInit(&iocfg);

	iocfg.GPIO_Mode = GPIO_Mode_OUT;
	iocfg.GPIO_OSpeed = GPIO_Speed_2MHz;
	iocfg.GPIO_Otype = GPIO_Otype_PP;
	iocfg.GPIO_PuPd = GPIO_PuPd_Float;

	ins->res_handle = tch_lld_gpio_init(BT_BDC.ctrl_port,1 << BT_BDC.ctrl_pin,&iocfg,pcfg);
	ins->res_handle->out(ins->res_handle,1 << BT_BDC.ctrl_pin,bSet);


	tch_usart_cfg ucfg;
	tch_lld_usart_initCfg(&ucfg);
	usart2->open(usart2,115200,pcfg,&ucfg);
	tchThread_sleep(500);

	uint32_t blen = usart2->available(usart2);




}



void tch_hld_btc_reset(){
	atcmd_bt_prototype* ins = (atcmd_bt_prototype*) &BT_StaticInstance;
}


atcmd_bt_addr tch_hld_bt_device_getAddress(atcmd_bt_device* device){

}

const char* tch_hld_bt_device_getName(atcmd_bt_device* device){

}

uint16_t tch_hld_bt_device_getRssi(atcmd_bt_device* device){

}

BOOL tch_hld_bt_device_connect(atcmd_bt_device* device){

}

BOOL tch_hld_bt_device_disconnect(atcmd_bt_device* device){

}

tch_istream* tch_hld_bt_device_openInputStream(atcmd_bt_device* device){

}

tch_ostream* tch_hld_bt_device_openOutputStream(atcmd_bt_device* device){

}

BOOL tch_hld_bt_server_setDeviceName(const char* name){

}

const char* tch_hld_bt_server_getDeviceName(){

}

atcmd_bt_status tch_hld_bt_server_getStatus(){

}

atcmd_bt_addr tch_hld_bt_server_getRecentTargetAddr(){

}

BOOL tch_hld_bt_server_startListenToTarget(atcmd_bt_addr baddr,uint32_t to){

}

BOOL tch_hld_bt_server_startListen(uint32_t to){

}
void tch_hld_bt_server_close(){

}
