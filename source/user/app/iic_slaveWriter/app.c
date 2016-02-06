/*
 * app.c
 *
 *  Created on: 2015. 1. 3.
 *      Author: innocentevil
 */


#include "tch.h"
#include "kernel/util/string.h"

#define MASTER_ADDR    ((uint8_t) 0x31)
#define SLAVE_ADDR     ((uint8_t) 0x52)

DECLARE_THREADROUTINE(main){

	tch_hal_module_gpio_t* gpio = ctx->Module->request(MODULE_TYPE_GPIO);
	tch_hal_module_iic_t* i2c = ctx->Module->request(MODULE_TYPE_IIC);

	gpio_config_t iocfg;
	gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_IN;
	iocfg.PuPd = GPIO_PuPd_PU;
	iocfg.popt = ActOnSleep;

	tch_gpioHandle* ext_wkup = gpio->allocIo(ctx,tch_gpio3,(1 << 12),&iocfg,tchWaitForever);

	gpio_event_config_t evcfg;
	evcfg.EvCallback = NULL;
	evcfg.EvEdge = GPIO_EvEdge_Fall;
	evcfg.EvType = GPIO_EvType_Interrupt;
	ext_wkup->registerIoEvent(ext_wkup,12,&evcfg);

	tch_iicCfg iiccfg;
	i2c->initCfg(&iiccfg);
	iiccfg.Addr = SLAVE_ADDR;
	iiccfg.AddrMode = IIC_ADDRMODE_7B;
	iiccfg.Baudrate = IIC_BAUDRATE_MID;
	iiccfg.Filter = FALSE;
	iiccfg.OpMode = IIC_OPMODE_FAST;
	iiccfg.Role = IIC_ROLE_SLAVE;

	tch_iicHandle_t* iic = i2c->allocIIC(ctx,IIc2,&iiccfg,tchWaitForever, ActOnSleep);
	uint8_t buf[20];
	uint32_t cnt = 0;
	while(TRUE){
		iic->write(iic,MASTER_ADDR,buf,strlen(buf));
		Thread->yield(1);
	}
}
