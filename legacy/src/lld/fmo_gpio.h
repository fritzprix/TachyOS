/*
 * fmo_gpio.h
 *
 *  Created on: 2014. 4. 12.
 *      Author: innocentevil
 */

#ifndef FMO_GPIO_H_
#define FMO_GPIO_H_

#include "../core/port/tch_stdtypes.h"


#define GPIO_Mode_IN                   (uint8_t) (0)
#define GPIO_Mode_OUT                  (uint8_t) (1)
#define GPIO_Mode_AF                   (uint8_t) (2)
#define GPIO_Mode_AN                   (uint8_t) (3)
#define GPIO_Mode_Msk                  (GPIO_Mode_IN |\
		                                GPIO_Mode_OUT|\
		                                GPIO_Mode_AF|\
		                                GPIO_Mode_AN)

#define GPIO_Otype_PP                   (uint8_t) (0)
#define GPIO_Otype_OD                   (uint8_t) (1)
#define GPIO_Otype_Msk                  (GPIO_Otype_PP | GPIO_Otype_OD)


#define GPIO_OSpeed_2M                  (uint8_t) (0)
#define GPIO_OSpeed_25M                 (uint8_t) (1)
#define GPIO_OSpeed_50M                 (uint8_t) (2)
#define GPIO_OSpeed_100M                (uint8_t) (3)
#define GPIO_OSpeed_Msk                 (GPIO_OSpeed_2M|\
		                                 GPIO_OSpeed_25M|\
		                                 GPIO_OSpeed_50M|\
		                                 GPIO_OSpeed_100M)

#define GPIO_PuPd_Float                 (uint8_t) (0)
#define GPIO_PuPd_PU                    (uint8_t) (1)
#define GPIO_PuPd_PD                    (uint8_t) (2)
#define GPIO_PuPd_Msk                   (GPIO_PuPd_Float|\
		                                 GPIO_PuPd_PU|\
		                                 GPIO_PuPd_PD)


#define GPIO_Evt_Edge_Rise              (uint8_t) (1)
#define GPIO_Evt_Edge_Fall              (uint8_t) (2)
#define GPIO_Evt_Edge_Msk               (GPIO_Evt_Edge_Fall | GPIO_Evt_Edge_Rise)

#define GPIO_Evt_Wakeup                 (uint8_t) (0)
#define GPIO_Evt_Interrupt              (uint8_t) (1)
#define GPIO_Evt_Msk                    (GPIO_Evt_Wakeup | GPIO_Evt_Interrupt)

#define GPIO_LP_Mode_ON                 (uint8_t) (0)
#define GPIO_LP_Mode_OFF                (uint8_t) (1)
#define GPIO_LP_Mode_Msk                (GPIO_LP_Mode_OFF | GPIO_LP_Mode_ON)

#define GPIO_CFG_INIT                   {\
	                                      GPIO_Mode_IN,\
	                                      GPIO_Otype_OD,\
	                                      GPIO_OSpeed_2M,\
	                                      GPIO_PuPd_Float,\
	                                      0,\
	                                      GPIO_LP_Mode_OFF\
	                                     }

#define DECLARE_IO_EVLISTENER(fn)     void fn(tch_gpio_instance* ins)


typedef struct _tch_gpio_instance_t tch_gpio_instance;
typedef struct _tch_gpio_cfg_t tch_gpio_cfg;
typedef struct _tch_gpio_evt_cfg_t tch_gpio_evt_cfg;
typedef void (*tch_gpio_EvtListener)(tch_gpio_instance* ins);
typedef uint8_t tch_gpio_t;
typedef enum { 	bClear = 0,bSet = 1 }tch_gpio_bState;


struct _tch_gpio_evt_cfg_t {
	uint8_t GPIO_Evt_Edge_Mode;
	uint8_t GPIO_Evt_Type;
};

struct _tch_gpio_instance_t {
	void (*out)(tch_gpio_instance* self,uint16_t pin,tch_gpio_bState nstate);
	tch_gpio_bState (*in)(tch_gpio_instance* self,uint16_t pin);
	int (*registerIoEvent)(tch_gpio_instance* self,uint16_t pin,tch_gpio_evt_cfg* ev_cfg,tch_gpio_EvtListener listener);
	int (*unregisterIoEvent)(tch_gpio_instance* self,uint16_t pin);
	void (*free)(tch_gpio_instance* self,uint16_t pin);
};

struct _tch_gpio_cfg_t {
	uint8_t GPIO_Mode;
	uint8_t GPIO_Otype;
	uint8_t GPIO_OSpeed;
	uint8_t GPIO_PuPd;
	uint8_t GPIO_AF;
};


#define GPIO_A                     (tch_gpio_t) (0)
#define GPIO_B                     (tch_gpio_t) (1)
#define GPIO_C                     (tch_gpio_t) (2)
#define GPIO_D                     (tch_gpio_t) (3)
#define GPIO_E                     (tch_gpio_t) (4)
#define GPIO_F                     (tch_gpio_t) (5)
#define GPIO_G                     (tch_gpio_t) (6)
#define GPIO_H                     (tch_gpio_t) (7)

void tch_lld_gpio_cfgInit(tch_gpio_cfg* cfg);
tch_gpio_instance* tch_lld_gpio_init(const tch_gpio_t gpio,uint16_t pin,const tch_gpio_cfg* cfg,tch_pwrMgrCfg pcfg);



#endif /* FMO_GPIO_H_ */
