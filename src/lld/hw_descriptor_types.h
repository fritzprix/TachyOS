/*
 * hw_descriptor_types.h
 *
 *  Created on: 2014. 4. 2.
 *      Author: innocentevil
 */

#ifndef HW_DESCRIPTOR_TYPES_H_
#define HW_DESCRIPTOR_TYPES_H_

#include "stm32f4xx.h"
#include "../core/port/tch_stdtypes.h"
#include "../core/fmo_synch.h"
#include "fmo_gpio.h"



typedef struct _timer_hw_desc_t timer_hw_descriptor;
typedef struct _gpio_hw_desc_t gpio_hw_descriptor;
typedef struct _exti_hw_desc_t exti_hw_descriptor;
typedef struct _dma_common_hw_desc_t dma_com_hw_desc_t;
typedef struct _dma_hw_desc_t dma_hw_descriptor;
typedef struct _usart_hw_desc_t usart_hw_descriptor;
typedef struct _adc_hw_desc_t adc_hw_descriptor;

typedef enum _pwr_ctrl {ActOnSleep,DeactOnSleep} tch_pwrMgrCfg;
typedef void (*generic_handler)(void*);


#define TIMER_FEATURE_RES16B                   (uint32_t) (1 << 0)
#define TIMER_FEATURE_RES32B                   (uint32_t) (1 << 1)
#define TIMER_FEATURE_RESMSK                   (TIMER_RES_16B | TIMER_RES_32B)

#define TIMER_FEATURE_PWM                      (uint32_t) (1 << 2)
#define TIMER_FEATURE_COMPARE                  (uint32_t) (1 << 3)
#define TIMER_FEATURE_PULSE_INPUT              (uint32_t) (1 << 4)

#define TIMER_FEATURE_CH_1                      (uint32_t) (1 << 5)
#define TIMER_FEATURE_CH_2                      (uint32_t) (1 << 6)
#define TIMER_FEATURE_CH_3                      (uint32_t) (1 << 7)
#define TIMER_FEATURE_CH_4                      (uint32_t) (1 << 8)




struct _timer_hw_desc_t {
	TIM_TypeDef*       _hw;
	uint32_t            status_flags;
	const uint32_t      feature_flags;
	volatile uint32_t*  clken;
	volatile uint32_t* _lpCklenr;
	const uint32_t      clkmsk;
	const uint32_t      lpClkmsk;
	IRQn_Type           irq;
};


struct _gpio_hw_desc_t {
	GPIO_TypeDef*      _hw;
	uint16_t            b_ocp_flag;
	volatile uint32_t* _clkenr;
	volatile uint32_t* _lpClkenr;
	const uint32_t      clkmsk;
	const uint32_t      lpClkmsk;
};

#define EXTI_EVTYPE_FLAG_Pos                          (uint8_t)  (0)
#define EXTI_EVTYPE_FLAG_Msk                          (uint16_t) (1 << EXTI_EVTYPE_FLAG_Pos)

#define EXTI_OCP_GPIO_IDX_Pos                         (uint8_t)  (1)
#define EXTI_OCP_GPIO_IDX_Msk                         (uint16_t) (1 << EXTI_OCP_GPIO_IDX_Pos)
#define EXTI_OCP_SET_GPIO_IDX(ins,idx)                (ins->flag |= (idx << 1))
#define EXTI_OCP_CLR_GPIO_IDX(ins,idx)                (ins->flag &= ~(idx << 1))
#define EXTI_OCP_GET_GPIO_IDX(ins)                    (ins->flag >> EXTI_OCP_GPIO_IDX_Pos)

struct _exti_hw_desc_t {
#ifdef FEATURE_MTHREAD
	mtx_lock            acc_mtx;
#endif
	uint16_t            flag;
	tch_gpio_t           ev_ioport;
	generic_handler     ev_handler;
	IRQn_Type           irq;
};

struct _dma_common_hw_desc_t {
	uint8_t occup_flag[2];
	uint8_t lpoccup_flag[2];
};

struct _dma_hw_desc_t {
	DMA_Stream_TypeDef* _hw;
	uint16_t             flag;
	volatile uint32_t*  _isr;
	volatile uint32_t*  _icr;
	const uint32_t       ipos;
	IRQn_Type            irq;
};


struct _usart_hw_desc_t {
	USART_TypeDef*      _hw;
	uint16_t             flag;
	volatile uint32_t*  _clkenr;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       clkmsk;
	const uint32_t       lpclkmsk;
	IRQn_Type            irq;
};

struct _adc_hw_desc_t {
	ADC_TypeDef*        _hw;
	uint16_t             state;
	volatile uint32_t*  _clkenr;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       clkmsk;
	const uint32_t       lpclkmsk;
	const uint8_t        extsel;
};


#endif /* HW_DESCRIPTOR_TYPES_H_ */
