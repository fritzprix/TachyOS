/*
 * app.c
 *
 *  Created on: 2015. 1. 3.
 *      Author: innocentevil
 */


#include "tch.h"


#define MASTER_ADDR    ((uint8_t) 0x31)
#define SLAVE_ADDR     ((uint8_t) 0x52)

DECLARE_THREADROUTINE(main){

	gpio_config_t iocfg;
	env->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_IN;
	iocfg.PuPd = GPIO_PuPd_PU;
	iocfg.popt = ActOnSleep;

	tch_gpioHandle* ext_wkup = env->Device->gpio->allocIo(env,tch_gpio3,(1 << 12),&iocfg,osWaitForever);

	gpio_event_config_t evcfg;
	evcfg.EvCallback = NULL;
	evcfg.EvEdge = GPIO_EvEdge_Fall;
	evcfg.EvType = GPIO_EvType_Interrupt;
	ext_wkup->registerIoEvent(ext_wkup,12,&evcfg);

	tch_iicCfg iiccfg;
	env->Device->i2c->initCfg(&iiccfg);
	iiccfg.Addr = SLAVE_ADDR;
	iiccfg.AddrMode = IIC_ADDRMODE_7B;
	iiccfg.Baudrate = IIC_BAUDRATE_MID;
	iiccfg.Filter = FALSE;
	iiccfg.OpMode = IIC_OPMODE_FAST;
	iiccfg.Role = IIC_ROLE_SLAVE;

	tch_iicHandle* iic = env->Device->i2c->allocIIC(env,IIc2,&iiccfg,osWaitForever,ActOnSleep);
	uint8_t buf[20];
	uint32_t cnt = 0;
	while(TRUE){
		env->uStdLib->stdio->siprintf(buf,"Hello World : %d",cnt++);
		/*
		ext_wkup->listen(ext_wkup,12,osWaitForever);
		env->uStdLib->stdio->iprintf("\r\nGot Interrupt\n");
		*/
		iic->write(iic,MASTER_ADDR,buf,env->uStdLib->string->strlen(buf));
		env->Thread->yield(1);
		/*env->uStdLib->stdio->iprintf("\r\nWrite Value : %s\n",buf);*/
	}
}
