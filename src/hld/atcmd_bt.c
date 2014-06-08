/*
 * atcmd_bt.c
 *
 *  Created on: 2014. 6. 1.
 *      Author: innocentevil
 */

#include "atcmd_bt.h"

#include <stdint.h>

//#include "../core/fmo_lldabs.h"
#include "../core/fmo_synch.h"
#include "../core/fmo_task.h"
#include "../core/fmo_thread.h"
//#include "../lld/fmo_gpio.h"
#include "../lld/fmo_usart.h"
#include "../util/mem.h"



#define BT_RES_OK                    ((const char*) "OK")
#define BT_RES_BTSLAVE               ((const char*) "BTWIN SPP Slave mode start")

#define BT_ATCMD_PING                ((const char*) "AT")
#define BT_ATCMD_SOFTRST             ((const char*) "ATZ")
#define BT_ATCMD_HARTRST             ((const char*) "AT&F")
#define BT_ATCMD_GETBTINFO           ((const char*) "AT+BTINFO?")
#define BT_ATCMD_GETRSSI             ((const char*) "AT+BTRSSI?")
#define BT_ATCMD_GETLQ               ((const char*) "AT+BTLQ?")
#define BT_ATCMD_SETBTSEC            ((const char*) "AT+BTSEC,")
#define BT_ATCMD_SETBTMODE           ((const char*) "AT+BTMODE,")
#define BT_ATCMD_SETBTNAME           ((const char*) "AT+BTNAME=")
#define BT_ATCMD_SETBTKEY            ((const char*) "AT+BTKEY=")
#define BT_ATCMD_SETBTROLE           ((const char*) "AT+BTROLE=")
#define BT_ATCMD_SETUART            ((const char*) "AT+BTUART,")


#define BT_PARITY_NONE              ((const char*) "N")
#define BT_PARITY_EVEN              ((const char*) "E")
#define BT_PARITY_ODD               ((const char*) "O")

#define BT_STOPBIT_1B               ((uint8_t) 1)
#define BT_STOPBIT_2B               ((uint8_t) 2)

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


static BOOL tch_hld_bt_server_setDeviceName(const char* ame);
static uint32_t tch_hld_bt_server_getDeviceName(char* rb);
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
static void tch_hld_btc_sendcmd(const char* cmdStr);
static uint32_t tch_hld_btc_getNextAT(uint8_t* buf,uint32_t to);
static BOOL tch_hld_btc_nextMatch(const char* pattern,uint32_t to);
static BOOL tch_hld_btc_setUsart(uint32_t brate,const char* p,uint8_t sb);
static BOOL tch_hld_btc_setMode(uint8_t mode);
static BOOL tch_hld_btc_quitBypass();

typedef struct _atcmd_bt_prototype_t atcmd_bt_prototype;

struct _atcmd_bt_prototype_t {
	uint16_t              status;
	tch_gpio_instance*    res_handle;
	tch_mtx_lock          lock;
	tch_iostream_buffer   istream_buffer;
	atcmd_bt_server_adapter* adapter;
};



static atcmd_bt_bdc BT_BDC = {
		GPIO_A,
		1
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
	tch_printCstr("BT Initailize Start..\n");
	tch_gpio_cfg iocfg;
	tch_lld_gpio_cfgInit(&iocfg);

	ins->adapter = adapter;

	iocfg.GPIO_Mode = GPIO_Mode_OUT;
	iocfg.GPIO_OSpeed = GPIO_OSpeed_2M;
	iocfg.GPIO_Otype = GPIO_Otype_PP;
	iocfg.GPIO_PuPd = GPIO_PuPd_PD;

	ins->res_handle = tch_lld_gpio_init(BT_BDC.ctrl_port,1 << BT_BDC.ctrl_pin,&iocfg,pcfg);
	ins->res_handle->out(ins->res_handle,1 << BT_BDC.ctrl_pin,bClear);


	tch_usart_cfg ucfg;
	tch_lld_usart_initCfg(&ucfg);
	ucfg.USART_StopBit = USART_StopBit_1B;
	usart2->open(usart2,115200,pcfg,&ucfg);         /// open usart port : 115200 , stop 1b , no parity
	tch_printCstr("BT Reset...\n");
	tch_hld_btc_reset();                            /// bt reset 2800ms pulse


	if(tch_hld_btc_nextMatch(BT_RES_BTSLAVE,1000)){  /// check 'BTWIN SPP Slave mode start' string match
		tch_printCstr("BT Slave Mode start...\n");
	}
	if(tch_hld_btc_nextMatch(BT_RES_OK,1000)){       /// check 'OK' string match
		tch_printCstr("BT OK\n");
	}else{
		tch_printCstr("BT Not OK\n");
		return NULL;
	}



	tch_hld_btc_sendcmd("AT");
	if(tch_hld_btc_nextMatch("OK",1000)){
		tch_printCstr("BT Host Interface OK! \n");
	}else{
		tch_printCstr("BT Host Interface Error!\n");
	}
	tch_hld_btc_setUsart(baudrate,BT_PARITY_NONE,1);
	tch_hld_bt_server_setDeviceName(deviceName);


}



void tch_hld_btc_reset(){
	atcmd_bt_prototype* ins = (atcmd_bt_prototype*) &BT_StaticInstance;
	ins->res_handle->out(ins->res_handle,1 << BT_BDC.ctrl_pin,bSet);
	tchThread_sleep(250);
	ins->res_handle->out(ins->res_handle,1 << BT_BDC.ctrl_pin,bClear);
}

void tch_hld_btc_sendcmd(const char* cmdStr){
	uint8_t buf[100];
	int len = tch_strconcat(buf,cmdStr,"\r");
	usart2->write(usart2,buf,(uint32_t)len,NULL);
}


uint32_t tch_hld_btc_getNextAT(uint8_t* buf,uint32_t to){
	atcmd_bt_prototype* ins = (atcmd_bt_prototype*) &BT_StaticInstance;
	uint8_t* tbuf = buf;
	uint8_t c = 0;
	while((c = usart2->getc(usart2)) != '\n'){
		tchThread_sleep(1);
		to--;
		if(!to){
			return 0;
		}
	};
	while((c = usart2->getc(usart2)) != '\n'){
		*tbuf++ = c;
		to--;
		if(!to){
			return 0;
		}
	}
	return tbuf - buf - 1;
}


BOOL tch_hld_btc_nextMatch(const char* pattern,uint32_t to){
	uint8_t buf[1 << 8];
	uint32_t size = tch_hld_btc_getNextAT(buf,200);
	return tch_memcmp(pattern,buf,size);
}


BOOL tch_hld_btc_setUsart(uint32_t brate,const char* parity,uint8_t sb){
	uint8_t buf[50];
	char cbuf[10];
	tch_strconcat(buf,BT_ATCMD_SETUART,tch_itoa(brate,cbuf,10));
	tch_strconcat(buf,buf,",");
	tch_strconcat(buf,buf,parity);
	tch_strconcat(buf,buf,",");
	tch_strconcat(buf,buf,tch_itoa(sb,cbuf,10));
	tch_hld_btc_sendcmd(buf);
	if(tch_hld_btc_nextMatch("OK",100)){
		tch_printCstr("BT Usart Baudrate is Configured Successfully..\n");
		return TRUE;
	}else{
		tch_printCstr("BT Usart Baudrate configuration failed\n");
		return FALSE;
	}
	return FALSE;

}
BOOL tch_hld_btc_setMode(uint8_t mode){
	uint8_t buf[20];
	char cbuf[10];
	tch_strconcat(buf,BT_ATCMD_SETBTMODE,tch_itoa(mode,cbuf,10));
	tch_hld_btc_sendcmd(buf);
	if(tch_hld_btc_nextMatch("OK",100)){
		tch_printCstr("Mode Changed\n");
		return TRUE;
	}
}

BOOL tch_hld_btc_quitBypass();

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
	uint8_t buf[100];
	tch_strconcat(buf,"AT+BTNAME=",name);
	int size = tch_strconcat(buf,buf,"\r");
	usart3->write(usart3,buf,size,NULL);
	usart2->write(usart2,buf,size,NULL);
	if(tch_hld_btc_nextMatch("OK",1000)){
		tch_printCstr("BT Name Changed\n");
	}else{
		tch_printCstr("BT Name Not Ok");
	}
}

uint32_t tch_hld_bt_server_getDeviceName(char* rb){
	uint8_t buf[100];
	int size = tch_strconcat(buf,"\bt+?0","");
	usart2->write(usart2,buf,size,NULL);

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
