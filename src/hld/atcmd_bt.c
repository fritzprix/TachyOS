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
#include "../util/tch_lib.h"



/**
 * static atcmd_bt_addr tch_hld_bt_device_getAddress(atcmd_bt_device* device);
static const char* tch_hld_bt_device_getName(atcmd_bt_device* device);
static uint16_t tch_hld_bt_device_getRssi(atcmd_bt_device* device);
static BOOL tch_hld_bt_device_connect(atcmd_bt_device* device);
static BOOL tch_hld_bt_device_disconnect(atcmd_bt_device* device);
static tch_istream* tch_hld_bt_device_openInputStream(atcmd_bt_device* device);
static tch_ostream* tch_hld_bt_device_openOutputStream(atcmd_bt_device* device);
 */

#ifndef BT_BUFFER_SIZE
#define BT_BUFFER_SIZE 256
#endif
#define BTDEVICE_INTERFACE           {\
	                                   tch_hld_bt_device_getAddress,\
	                                   tch_hld_bt_device_getName,\
	                                   tch_hld_bt_device_getRssi,\
	                                   tch_hld_bt_device_connect,\
	                                   tch_hld_bt_device_disconnect,\
	                                   tch_hld_bt_device_openInputStream,\
	                                   tch_hld_bt_device_openOutputStream}


#define BTSERVER_INTERFACE           {\
	                                   tch_hld_bt_server_close}


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
#define BT_ATCMD_SETUART             ((const char*) "AT+BTUART,")
#define BT_ATCMD_WAIT_SCAN           ((const char*) "AT+BTSCAN")


#define BT_PARITY_NONE              ((const char*) "N")
#define BT_PARITY_EVEN              ((const char*) "E")
#define BT_PARITY_ODD               ((const char*) "O")

#define BT_STOPBIT_1B               ((uint8_t) 1)
#define BT_STOPBIT_2B               ((uint8_t) 2)

/***
 *  public function
 */
/**
 * Remote BT device Handle Interface
 */
static const char* tch_hld_bt_device_getAddress(atcmd_bt_device* device);
static const char* tch_hld_bt_device_getName(atcmd_bt_device* device);
static uint16_t tch_hld_bt_device_getRssi(atcmd_bt_device* device);
static BOOL tch_hld_bt_device_connect(atcmd_bt_device* device);
static BOOL tch_hld_bt_device_disconnect(atcmd_bt_device* device);
static tch_istream* tch_hld_bt_device_openInputStream(atcmd_bt_device* device);
static tch_ostream* tch_hld_bt_device_openOutputStream(atcmd_bt_device* device);



/**
 * Local BT server Handle Interface
 */
static void tch_hld_bt_server_close();



/***
 *
 *  private function
 */
typedef struct _atcmd_bt_prototype_t atcmd_bt_prototype;
typedef struct _atcmd_btdevice_prototype_t atcmd_btdevice_prototype;

static BOOL tch_hld_btc_setDeviceName(const char* name);
static BOOL tch_hld_btc_hreset();
static BOOL tch_hld_btc_sreset();
static void tch_hld_btc_sendcmd(const char* cmdStr);
static uint32_t tch_hld_btc_getNextAT(uint8_t* buf,uint32_t to);
static BOOL tch_hld_btc_nextMatch(const char* pattern,uint32_t to);
static BOOL tch_hld_btc_setUsart(uint32_t brate,const char* p,uint8_t sb);
static BOOL tch_hld_btc_setMode(uint8_t mode);
static BOOL tch_hld_btc_quitBypass();
static BOOL tch_hld_btc_waitScan(atcmd_btdevice_prototype* device);
static THREAD_ROUTINE(tch_hld_btc_serverLooper);
static DECLARE_IO_EVLISTENER(tch_hld_btc_iev_listener);
static DECLARE_TASK_FN(tch_hld_btc_disconnect_handler);

typedef struct _atcmd_bt_prototype_t atcmd_bt_prototype;
typedef struct _atcmd_btdevice_prototype_t atcmd_btdevice_prototype;
typedef struct _atcmd_btstream_buffer_t atcmd_btstream_buffer;

struct _atcmd_btdevice_prototype_t {
	atcmd_bt_device   _pix;
	char              _addr[20];
	uint32_t           status;
	uint64_t           addri;
	tch_istream*       instr;
	tch_ostream*       outstr;

};

struct _atcmd_bt_prototype_t {
	uint16_t                  status;
	tch_gpio_instance*        res_handle;
	tch_gpio_instance*        ev_handle;
	tch_mtx_lock              lock;
	tch_iostream_buffer       istream_buffer;
	atcmd_bt_server_adapter*  adapter;
	tchThread_t*              server_thread;
	volatile BOOL             server_active;
	tch_genericList_queue_t   server_evQue;
	atcmd_bt_server_instance  server_interface;
};

uint8_t BT_InputBuffer[BT_BUFFER_SIZE];





static atcmd_bt_bdc BT_BDC = {
		GPIO_A,
		GPIO_A,
		1,
		0
};

static uint32_t BTLooperStack[1 << 9];

__attribute__((section(".data")))  static atcmd_bt_prototype BT_StaticInstance = {
		0,
		NULL,
		NULL,
		MTX_INIT,
		IOSTREAM_INIT,
		NULL,
		NULL,
		TRUE,
		GENERIC_LIST_QUEUE_INIT,
		BTSERVER_INTERFACE
};

/**
 * 	atcmd_bt_device   _pix;
	char              _addr[20];
	uint32_t           status;
	uint64_t           addri;
	tch_istream*       instr;
	tch_ostream*       outstr;
 */
__attribute__((section(".data"))) static atcmd_btdevice_prototype BTDevice_StaticInstance = {
		BTDEVICE_INTERFACE,
		{0,},
		0,
		0,
		NULL,
		NULL
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

	iocfg.GPIO_Mode = GPIO_Mode_IN;
	iocfg.GPIO_OSpeed = GPIO_OSpeed_2M;
	iocfg.GPIO_Otype = GPIO_Otype_PP;
	iocfg.GPIO_PuPd = GPIO_PuPd_Float;

	ins->ev_handle = tch_lld_gpio_init(BT_BDC.ev_port,1 << BT_BDC.ev_pin,&iocfg,pcfg);


	tch_usart_cfg ucfg;
	tch_lld_usart_initCfg(&ucfg);
	ucfg.USART_StopBit = USART_StopBit_1B;
	usart2->open(usart2,115200,pcfg,&ucfg);         /// open usart port : 115200 , stop 1b , no parity
	tch_hld_btc_hreset();                            /// bt reset 2800ms pulse
	tch_hld_btc_sendcmd("AT");
	if(tch_hld_btc_nextMatch("OK",1000)){
		tch_printCstr("BT Host Interface OK! \n");
	}else{
		tch_printCstr("BT Host Interface Error!\n");
	}

	ins->server_active = TRUE;
	ins->server_thread = tchThread_create(BTLooperStack,1 << 9,tch_hld_btc_serverLooper,THREAD_PRIORITY_HIGH,NULL,"BTlooper");


	tch_hld_btc_setUsart(baudrate,BT_PARITY_NONE,1);
	tch_hld_btc_setDeviceName(deviceName);
	tch_hld_btc_sreset();
	tch_hld_btc_setMode(2);
	tchThread_start(ins->server_thread);
	return &ins->server_interface;
}



BOOL tch_hld_btc_hreset(){
	tch_printCstr("BT Hard Reset\n");
	atcmd_bt_prototype* ins = (atcmd_bt_prototype*) &BT_StaticInstance;
	ins->res_handle->out(ins->res_handle,1 << BT_BDC.ctrl_pin,bSet);
	tchThread_sleep(250);
	ins->res_handle->out(ins->res_handle,1 << BT_BDC.ctrl_pin,bClear);
	if(!tch_hld_btc_nextMatch(BT_RES_BTSLAVE,1000)){  /// check 'BTWIN SPP Slave mode start' string match
		return FALSE;
	}
	if(tch_hld_btc_nextMatch(BT_RES_OK,1000)){       /// check 'OK' string match
		tch_printCstr("OK\n");
		return TRUE;
	}
	tch_printCstr("Not OK\n");
	return FALSE;

}

BOOL tch_hld_btc_sreset(){
	tch_hld_btc_sendcmd(BT_ATCMD_SOFTRST);
	if(!tch_hld_btc_nextMatch(BT_RES_OK,1000)){       /// check 'OK' string match
		return FALSE;
	}
	tch_printCstr("BT Soft Reset\n");
	if(!tch_hld_btc_nextMatch(BT_RES_BTSLAVE,1000)){  /// check 'BTWIN SPP Slave mode start' string match
		return FALSE;
	}
	if(tch_hld_btc_nextMatch(BT_RES_OK,1000)){       /// check 'OK' string match
		tch_printCstr("OK\n");
		return TRUE;
	}
	tch_printCstr("Not OK\n");
	return FALSE;

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
	*(tbuf - 1) = '\0';
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
	}
	tch_printCstr("BT Usart Baudrate configuration failed\n");
	return FALSE;

}
BOOL tch_hld_btc_setMode(uint8_t mode){
	uint8_t buf[20];
	char cbuf[10];
	tch_strconcat(buf,BT_ATCMD_SETBTMODE,tch_itoa(mode,cbuf,10));
	tch_hld_btc_sendcmd(buf);
	if(tch_hld_btc_nextMatch("OK",100)){
		tch_printCstr("Mode Change Successful!\n");
		return TRUE;
	}
	tch_printCstr("Mode Change Failed");
	return FALSE;
}

BOOL tch_hld_btc_quitBypass(){
	return TRUE;
}

BOOL tch_hld_btc_waitScan(atcmd_btdevice_prototype* device){
	uint8_t buf[50];
	char mbuf[50];
	tch_hld_btc_sendcmd(BT_ATCMD_WAIT_SCAN);
	if(tch_hld_btc_nextMatch("OK",100)){
		tch_printCstr("Wait Connection Request...\n");
	}
	tch_hld_btc_getNextAT(buf,100);
	char* spltlist[5];
	tch_strSplit(buf,buf,' ',spltlist);
	if(tch_strcmp(spltlist[0],"CONNECT")){
		tch_strconcat(mbuf,"BT Connected from ",spltlist[1]);
		tch_strconcat(mbuf,mbuf,"\n");
		tch_printCstr(mbuf);
		tch_strcpy(device->_addr,spltlist[1]);
		return TRUE;
	}
	return FALSE;
}


const char* tch_hld_bt_device_getAddress(atcmd_bt_device* device){
	atcmd_btdevice_prototype* ins = (atcmd_btdevice_prototype*) device;
	return ins->_addr;
}

const char* tch_hld_bt_device_getName(atcmd_bt_device* device){
	atcmd_btdevice_prototype* ins = (atcmd_btdevice_prototype*) device;
	return ins->_addr;
}

uint16_t tch_hld_bt_device_getRssi(atcmd_bt_device* device){

}

BOOL tch_hld_bt_device_connect(atcmd_bt_device* device){

}

BOOL tch_hld_bt_device_disconnect(atcmd_bt_device* device){

}

tch_istream* tch_hld_bt_device_openInputStream(atcmd_bt_device* device){
	return usart2->openInputStream(usart2);
}

tch_ostream* tch_hld_bt_device_openOutputStream(atcmd_bt_device* device){
	return usart2->openOutputStream(usart2);
}



void tch_hld_bt_server_close(){

}


BOOL tch_hld_btc_setDeviceName(const char* name){
	uint8_t buf[100];
	tch_strconcat(buf,"AT+BTNAME=",name);
	tch_hld_btc_sendcmd(buf);
	if(tch_hld_btc_nextMatch("OK",1000)){
		tch_printCstr("BT Name Changed\n");
		return TRUE;
	}
	tch_printCstr("BT Name Not Ok");
	return FALSE;
}



THREAD_ROUTINE(tch_hld_btc_serverLooper){
	atcmd_bt_prototype* ins = &BT_StaticInstance;
	while(ins->server_active){
		if(tch_hld_btc_waitScan(&BTDevice_StaticInstance)){
			if(ins->adapter){
				ins->adapter->onConnected((atcmd_bt_device*)&BTDevice_StaticInstance);
			}
		}
		tch_gpio_evt_cfg evcfg;
		evcfg.GPIO_Evt_Edge_Mode = GPIO_Evt_Edge_Rise;
		evcfg.GPIO_Evt_Type = GPIO_Evt_Interrupt;
		ins->ev_handle->registerIoEvent(ins->ev_handle,1 << BT_BDC.ev_pin,&evcfg,tch_hld_btc_iev_listener);
		tchThread_wait(&ins->server_evQue);
		if(tch_hld_btc_nextMatch("DISCONNECT",100)){
			if(ins->adapter){
				ins->adapter->onDisconnected((atcmd_bt_device*) &BTDevice_StaticInstance);
			}
		}

	}
	return NULL;
}

DECLARE_IO_EVLISTENER(tch_hld_btc_iev_listener){
	tch_postSysTask(tch_hld_btc_disconnect_handler,NULL,TASK_PRIOR_NORMAL);
}

DECLARE_TASK_FN(tch_hld_btc_disconnect_handler){
	atcmd_bt_prototype* ins = (atcmd_bt_prototype*) &BT_StaticInstance;
	ins->ev_handle->unregisterIoEvent(ins->ev_handle,1 << BT_BDC.ev_pin);
	if(ins->server_evQue.entry){
		if(ins->adapter){
			ins->adapter->onDisconnected((atcmd_bt_device*) &BTDevice_StaticInstance);
		}
		tchThread_wake(&ins->server_evQue);
		tch_printCstr("BT Disconnected\n");
	}
}
