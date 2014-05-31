/*
 * fmo_gpio.c
 *
 *  Created on: 2014. 4. 12.
 *      Author: innocentevil
 */

#include "fmo_gpio.h"
#include "hw_descriptor_types.h"
#include "../core/fmo_synch.h"
#include "../core/port/cortex_v7m_port.h"




static void tch_gpio_out(tch_gpio_instance* self,uint16_t pin,tch_gpio_bState nstate);
static tch_gpio_bState tch_gpio_in(tch_gpio_instance* self,uint16_t pin);
static int tch_gpio_registerIoEvent(tch_gpio_instance* self,uint16_t pin,tch_gpio_evt_cfg* ev_cfg,tch_gpio_EvtListener listener);
static int tch_gpio_unregisterIoEvent(tch_gpio_instance* self,uint16_t pin);
static void tch_gpio_free(tch_gpio_instance* self,uint16_t pin);



/**
 *   =================== Macro Variable ====================
 */


#define GPIO_AF_Msk                                   (uint8_t) ((1 << 4) - 1)
#define EXTICR_Msk                                    (uint8_t) ((1 << 4) - 1)

#define _GPIO_PUBLIC_INS_INIT                          {\
	                                                    tch_gpio_out,\
	                                                    tch_gpio_in,\
	                                                    tch_gpio_registerIoEvent,\
	                                                    tch_gpio_unregisterIoEvent,\
	                                                    tch_gpio_free\
	                                                    }


/**
 *   ==================== Macro Function ====================
 */
#ifdef FEATURE_MTHREAD
#define _GPIO_PROTOTYPE_INTI(x) {\
	                             _GPIO_PUBLIC_INS_INIT,\
	                             0,\
	                             0,\
	                             x,\
	                             MTX_INIT\
                                 }
#define LOCK_INS(ins_p)         {tch_Mtx_lockt(&ins_p->lock,MTX_TRYMODE_WAIT);}
#define UNLOCK_INS(ins_p)       {tch_Mtx_unlockt(&ins_p->lock);}
#else
#define _GPIO_PROTOTYPE_INTI(x) {\
	                             _GPIO_PUBLIC_INS_INIT,\
	                             0,\
	                             0,\
	                             x\
                                 }
#define LOCK_INS(ins_p)
#define UNLOCK_INS(ins_p)
#endif

/*CLEAR_GPIO_MODE(_hw_desc_p,pos);\*/
#define ACCESS_HW(_hw_desc_p) ((GPIO_TypeDef*) _hw_desc_p->_hw)
#define CLEAR_GPIO_MODE(_hw_desc_p,pos) {	ACCESS_HW(_hw_desc_p)->MODER &= ~(GPIO_Mode_Msk << (pos << 1));}
#define SET_GPIO_MODE(_hw_desc_p,pos,mode) {\
	CLEAR_GPIO_MODE(_hw_desc_p,pos);\
	ACCESS_HW(_hw_desc_p)->MODER |= (mode << (pos << 1));\
	}
#define CLEAR_GPIO_OTYPE(_hw_desc_p,pos) {ACCESS_HW(_hw_desc_p)->OTYPER &= ~(GPIO_Otype_Msk << (pos << 0));}
#define SET_GPIO_OTYPE(_hw_desc_p,pos,otype) {\
	CLEAR_GPIO_OTYPE(_hw_desc_p,pos);\
	ACCESS_HW(_hw_desc_p)->OTYPER |= (otype << (pos << 0));\
	}
#define CLEAR_GPIO_OSPEED(_hw_desc_p,pos) {ACCESS_HW(_hw_desc_p)->OSPEEDR &= ~(GPIO_OSpeed_Msk << (pos << 1));}
#define SET_GPIO_OSPEED(_hw_desc_p,pos,ospeed) {\
	CLEAR_GPIO_OSPEED(_hw_desc_p,pos);\
	ACCESS_HW(_hw_desc_p)->OSPEEDR |= (ospeed << (pos << 1));\
	}

#define CLEAR_GPIO_PuPd(_hw_desc_p,pos) {ACCESS_HW(_hw_desc_p)->PUPDR &= ~(GPIO_PuPd_Msk << (pos << 1));}
#define SET_GPIO_PuPd(_hw_desc_p,pos,pupd) {\
	CLEAR_GPIO_PuPd(_hw_desc_p,pos);\
	ACCESS_HW(_hw_desc_p)->PUPDR |= (pupd << (pos << 1));\
	}
#define GPIO_READ_IN(_hw_desc_p,pin) (ACCESS_HW(_hw_desc_p)->IDR & pin)
#define GPIO_WRITE_OUT(_hw_desc_p,pin,bstate){\
	if(bstate){ACCESS_HW(_hw_desc_p)->ODR |= pin;}\
	else{ACCESS_HW(_hw_desc_p)->ODR &= ~pin;}\
}



typedef struct _tch_gpio_prototype tch_gpio_prototype;

struct _tch_gpio_prototype{
	tch_gpio_instance       _public_instance;
	uint16_t                lp_flag;
	uint16_t                evt_flag;
	const uint8_t           self_idx;
#ifdef FEATURE_MTHREAD
	mtx_lock                lock;
#endif
};



/**
 *  =============== GPIO Hw Description ===================
 *
 */
static gpio_hw_descriptor GPIO_HWs[] = {
		{
				GPIOA,
				0,
				&RCC->AHB1ENR,
				&RCC->AHB1LPENR,
				RCC_AHB1ENR_GPIOAEN,
				RCC_AHB1LPENR_GPIOALPEN
		},
		{
				GPIOB,
				0,
				&RCC->AHB1ENR,
				&RCC->AHB1LPENR,
				RCC_AHB1ENR_GPIOBEN,
				RCC_AHB1LPENR_GPIOBLPEN
		},
		{
				GPIOC,
				0,
				&RCC->AHB1ENR,
				&RCC->AHB1LPENR,
				RCC_AHB1ENR_GPIOCEN,
				RCC_AHB1LPENR_GPIOCLPEN
		},
		{
				GPIOD,
				0,
				&RCC->AHB1ENR,
				&RCC->AHB1LPENR,
				RCC_AHB1ENR_GPIODEN,
				RCC_AHB1LPENR_GPIODLPEN
		},
		{
				GPIOE,
				0,
				&RCC->AHB1ENR,
				&RCC->AHB1LPENR,
				RCC_AHB1ENR_GPIOEEN,
				RCC_AHB1LPENR_GPIOELPEN
		},
		{
				GPIOF,
				0,
				&RCC->AHB1ENR,
				&RCC->AHB1LPENR,
				RCC_AHB1ENR_GPIOFEN,
				RCC_AHB1LPENR_GPIOFLPEN
		},
		{
				GPIOG,
				0,
				&RCC->AHB1ENR,
				&RCC->AHB1LPENR,
				RCC_AHB1ENR_GPIOGEN,
				RCC_AHB1LPENR_GPIOGLPEN
		},
		{
				GPIOH,
				0,
				&RCC->AHB1ENR,
				&RCC->AHB1LPENR,
				RCC_AHB1ENR_GPIOHEN,
				RCC_AHB1LPENR_GPIOHLPEN
		}
};

/**
 *  ================= Ext. Interrupt Hw Description ==================
 *
 */
static exti_hw_descriptor EXTI_HWs[] = {
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI0_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI1_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI2_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI3_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI4_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				MTX_INIT,
				0,
				0,
				NULL,
				EXTI15_10_IRQn
		}
};


/**
 * ================  static(internal) gpio instances ====================
 */
static tch_gpio_prototype GPIO_static_Instances[] = {
		_GPIO_PROTOTYPE_INTI(GPIO_A),
		_GPIO_PROTOTYPE_INTI(GPIO_B),
		_GPIO_PROTOTYPE_INTI(GPIO_C),
		_GPIO_PROTOTYPE_INTI(GPIO_D),
		_GPIO_PROTOTYPE_INTI(GPIO_E),
		_GPIO_PROTOTYPE_INTI(GPIO_F),
		_GPIO_PROTOTYPE_INTI(GPIO_G),
		_GPIO_PROTOTYPE_INTI(GPIO_H)
};


void tch_lld_gpio_cfgInit(tch_gpio_cfg* cfg){
	cfg->GPIO_AF = 0;
	cfg->GPIO_LP_Mode = GPIO_LP_Mode_OFF;
	cfg->GPIO_Mode = GPIO_Mode_IN;
	cfg->GPIO_OSpeed = GPIO_OSpeed_2M;
	cfg->GPIO_Otype = GPIO_OType_OD;
	cfg->GPIO_PuPd = GPIO_PuPd_NOPULL;
}


tch_gpio_instance* tch_lld_getGpio(const tch_gpio_t gpio,uint16_t pin,const tch_gpio_cfg* cfg){
	tch_gpio_prototype* ins = &GPIO_static_Instances[gpio];
	gpio_hw_descriptor* gpio_hwp = &GPIO_HWs[gpio];
	LOCK_INS(ins);                                                                        // thread try lock mtx to set occupation flag for given gpio pin
	if(gpio_hwp->b_ocp_flag & pin){
		UNLOCK_INS(ins);                                                                  // if pin is already occupied to another thread it unlocks mutex and return Null
		return NULL;                                                                      // **Note : caller should check whether returned value is null or not
	}else{
		gpio_hwp->b_ocp_flag |= pin;                                                      // if requested pin is available this thread set flags and unlocks mutex
	}
	UNLOCK_INS(ins);
	if((*gpio_hwp->_clkenr & gpio_hwp->clkmsk) == 0){                                     // Enable clk source ,unless clk source of this gpio is enabled
		*gpio_hwp->_clkenr |= gpio_hwp->clkmsk;
	}
	uint8_t pos = 0;
	while(pin){                                                                           // investigate requested pin value by shifting it to the right
		if(pin & ((uint16_t) 1)){                                                         // and requested pin(bit) is found
			SET_GPIO_MODE(gpio_hwp,pos,cfg->GPIO_Mode);                                   // set moder register
			SET_GPIO_OTYPE(gpio_hwp,pos,cfg->GPIO_Otype);                                 // set otype register
			SET_GPIO_OSPEED(gpio_hwp,pos,cfg->GPIO_OSpeed);                               // set ospeed register
			SET_GPIO_PuPd(gpio_hwp,pos,cfg->GPIO_PuPd);                                   // set pupd register
			if(pos < 8){                                                                  // set alternate function of io pin
				ACCESS_HW(gpio_hwp)->AFR[0] &= ~(GPIO_AF_Msk << (pos << 2));
				ACCESS_HW(gpio_hwp)->AFR[0] |= (cfg->GPIO_AF << (pos << 2));
			}else{
				ACCESS_HW(gpio_hwp)->AFR[1] &= ~(GPIO_AF_Msk << ((pos - 8) << 2));
				ACCESS_HW(gpio_hwp)->AFR[1] |= (cfg->GPIO_AF << ((pos - 8) << 2));
			}
			if(cfg->GPIO_LP_Mode == GPIO_LP_Mode_ON){                                      // if lp mode is required (io pin have to be active when device goes into sleep)
				if((*gpio_hwp->_lpClkenr & gpio_hwp->lpClkmsk) == 0){
					*gpio_hwp->_lpClkenr |= gpio_hwp->lpClkmsk;
				}
				ins->lp_flag |= (uint16_t) (1 << pos);                                     // set bit of io Pin in lp_flag (for checking lp clck mode can be disabled)
			}
		}
		pin >>= 1;
		pos++;
	}
	return (tch_gpio_instance*)ins;
}


void tch_gpio_out(tch_gpio_instance* self,uint16_t pin,tch_gpio_bState nstate){
	tch_gpio_prototype* ins = (tch_gpio_prototype*) self;
	gpio_hw_descriptor* gpio_hwp = &GPIO_HWs[ins->self_idx];
	LOCK_INS(ins);
	GPIO_WRITE_OUT(gpio_hwp,pin,nstate);
	UNLOCK_INS(ins);
}

tch_gpio_bState tch_gpio_in(tch_gpio_instance* self,uint16_t pin){
	tch_gpio_prototype* ins = (tch_gpio_prototype*) self;
	gpio_hw_descriptor* gpio_hwp = &GPIO_HWs[ins->self_idx];
	return (tch_gpio_bState) GPIO_READ_IN(gpio_hwp,pin);
}

int tch_gpio_registerIoEvent(tch_gpio_instance* self,uint16_t pin,tch_gpio_evt_cfg* ev_cfg,tch_gpio_EvtListener listener){
	tch_gpio_prototype* ins = (tch_gpio_prototype*) self;
	exti_hw_descriptor* exti_hwp = NULL;
	uint8_t cnt = 0;
	uint8_t pos = 0;
	if((RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN) == 0){                                      /// To enable external interrupt, SysCfg Block should be enabled first.(exti mux is controlled by this)
		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	}
	while(pin){
		if(pin & ((uint16_t) 1)){
			exti_hwp = &EXTI_HWs[pos];
			tch_Mtx_lockt(&exti_hwp->acc_mtx,MTX_TRYMODE_WAIT);                           /// lock mutex of exti instance which is relavant to given gpio pin
			if(exti_hwp->ev_handler == NULL){
				exti_hwp->ev_handler = (generic_handler) listener;
				exti_hwp->ev_ioport = ins->self_idx;
				if(ev_cfg->GPIO_Evt_Edge_Mode & GPIO_Evt_Edge_Fall){
					EXTI->FTSR |= (1 << pos);
				}
				if(ev_cfg->GPIO_Evt_Edge_Mode & GPIO_Evt_Edge_Rise){
					EXTI->RTSR |= (1 << pos);
				}
				switch(ev_cfg->GPIO_Evt_Type){
				case GPIO_Evt_Interrupt:
					EXTI->IMR |= (1 << pos);
					break;
				case GPIO_Evt_Wakeup:
					EXTI->EMR |= (1 << pos);
					break;
				}
				NVIC_SetPriority(exti_hwp->irq,HANDLER_NORMAL_PRIOR);
				NVIC_EnableIRQ(exti_hwp->irq);
				ins->evt_flag |= (1 << pos);
				EXTI_OCP_SET_GPIO_IDX(exti_hwp,ins->self_idx);
				//exti_hwp->flag |= (ins->self_idx << EXTI_OCP_GPIO_IDX_Pos);              /// exti store bound gpio port in its flag field
				SYSCFG->EXTICR[pos >> 2] |= (ins->self_idx << ((pos % 4) * 4));          /// config exti channel mux
				cnt++;
			}
			tch_Mtx_unlockt(&exti_hwp->acc_mtx);
		}
		pin >>= 1;
		pos++;
	}
	return cnt;
}

int tch_gpio_unregisterIoEvent(tch_gpio_instance* self,uint16_t pin){
	tch_gpio_prototype* ins = (tch_gpio_prototype*) self;
	exti_hw_descriptor* exti_hwp = NULL;
	uint8_t pos = 0;
	uint8_t success_cnt = 0;
	while(pin){
		if(pin & ((uint16_t) 1)){
			exti_hwp = &EXTI_HWs[pos];
			tch_Mtx_lockt(&exti_hwp->acc_mtx,MTX_TRYMODE_WAIT);
			if(exti_hwp->ev_ioport == ins->self_idx){
				if((exti_hwp->ev_handler != NULL) && (EXTI_OCP_GET_GPIO_IDX(exti_hwp) == pos)){
					exti_hwp->ev_handler =  NULL;
					exti_hwp->flag = 0;
					EXTI->FTSR &= ~(1 << pos);
					EXTI->RTSR &= ~(1 << pos);
					EXTI->IMR &= ~(1 << pos);
					EXTI->EMR &= ~(1 << pos);
					NVIC_DisableIRQ(exti_hwp->irq);
					ins->evt_flag &= ~(1 << pos);
					SYSCFG->EXTICR[pos >> 2] &= ~(EXTICR_Msk << ((pos % 4) * 4));
					success_cnt++;
				}
			}
			tch_Mtx_unlockt(&exti_hwp->acc_mtx);
		}
		pin >>= 1;
		pos++;
	}
	return success_cnt;
}

void tch_gpio_free(tch_gpio_instance* self,uint16_t pin){
	tch_gpio_prototype* ins = (tch_gpio_prototype*) self;
	gpio_hw_descriptor* gpio_hwp = &GPIO_HWs[ins->self_idx];
	uint16_t pincp = pin;
	LOCK_INS(ins);
	uint8_t pos = 0;
	while(pin){
		if(pin & ((uint16_t) 1)){
			CLEAR_GPIO_MODE(gpio_hwp,pos);                // set moder register
			CLEAR_GPIO_OTYPE(gpio_hwp,pos);               // set otype register
			CLEAR_GPIO_OSPEED(gpio_hwp,pos);              // set ospeed register
			CLEAR_GPIO_PuPd(gpio_hwp,pos);                // set pupd register
			if(pos < 8){
				ACCESS_HW(gpio_hwp)->AFR[0] &= ~(GPIO_AF_Msk << (pos << 2));
			}else{
				ACCESS_HW(gpio_hwp)->AFR[1] &= ~(GPIO_AF_Msk << ((pos - 8) << 2));
			}
			 tch_gpio_unregisterIoEvent(self,pin);
		}
		pin >>= 1;
		pos++;
	}
	gpio_hwp->b_ocp_flag &= ~pincp;
	ins->lp_flag &= ~pincp;
	if(!ins->lp_flag){
		*gpio_hwp->_lpClkenr &= ~gpio_hwp->lpClkmsk;
	}
	if(!gpio_hwp->b_ocp_flag){
		*gpio_hwp->_clkenr &= ~gpio_hwp->clkmsk;
	}
	UNLOCK_INS(ins);
}


void EXTI0_IRQHandler(void){
	exti_hw_descriptor* exti_hwp = &EXTI_HWs[0];
	if(exti_hwp->ev_handler != NULL){
		exti_hwp->ev_handler(&GPIO_static_Instances[exti_hwp->flag >> EXTI_OCP_GPIO_IDX_Pos]);
	}
	EXTI->PR |= (uint32_t) (1 << 0);
	__DSB();
}

void EXTI1_IRQHandler(void){
	exti_hw_descriptor* exti_hwp = &EXTI_HWs[1];
	if(exti_hwp->ev_handler != NULL){
			exti_hwp->ev_handler(&GPIO_static_Instances[exti_hwp->flag >> EXTI_OCP_GPIO_IDX_Pos]);
	}
	EXTI->PR |= (uint32_t) (1 << 1);
	__DSB();
}

void EXTI2_IRQHandler(void){
	exti_hw_descriptor* exti_hwp = &EXTI_HWs[2];
	if(exti_hwp->ev_handler != NULL){
		exti_hwp->ev_handler(&GPIO_static_Instances[exti_hwp->flag >> EXTI_OCP_GPIO_IDX_Pos]);
	}
	EXTI->PR |= (uint32_t) (1 << 2);
	__DSB();
}

void EXTI3_IRQHandler(void){
	exti_hw_descriptor* exti_hwp = &EXTI_HWs[3];
	if(exti_hwp->ev_handler != NULL){
		exti_hwp->ev_handler(&GPIO_static_Instances[exti_hwp->flag >> EXTI_OCP_GPIO_IDX_Pos]);
	}
	EXTI->PR |= (uint32_t) (1 << 4);
}

void EXTI4_IRQHandler(void){
	exti_hw_descriptor* exti_hwp = &EXTI_HWs[4];
	if(exti_hwp->ev_handler != NULL){
		exti_hwp->ev_handler(&GPIO_static_Instances[exti_hwp->flag >> EXTI_OCP_GPIO_IDX_Pos]);
	}
	EXTI->PR |= (uint32_t) (1 << 4);
}

void EXTI9_5_IRQHandler(void){
	uint32_t ext_pr = (EXTI->PR >> 5);
	exti_hw_descriptor* exti_hwp = NULL;
	uint8_t pos = 0;
	while(ext_pr){
		if(ext_pr & 1){
			exti_hwp = &EXTI_HWs[5 + pos];
			if(exti_hwp->ev_handler != NULL){
				exti_hwp->ev_handler(&GPIO_static_Instances[exti_hwp->flag >> EXTI_OCP_GPIO_IDX_Pos]);
			}
			EXTI->PR |= (uint32_t) (1 << (5 + pos));
		}
		ext_pr >>= 1;
		pos++;
	}
}

void EXTI15_10_IRQHandler(void){
	uint32_t ext_pr = (EXTI->PR >> 10);
	exti_hw_descriptor* exti_hwp = NULL;
	uint8_t pos = 0;
	while(ext_pr){
		if(ext_pr & 1){
			exti_hwp = &EXTI_HWs[10 + pos];
			if(exti_hwp->ev_handler != NULL){
				exti_hwp->ev_handler(&GPIO_static_Instances[exti_hwp->flag >> EXTI_OCP_GPIO_IDX_Pos]);
			}
			EXTI->PR |= (uint32_t) (1 << (10 + pos));
		}
		ext_pr >>= 1;
		pos++;
	}
}
