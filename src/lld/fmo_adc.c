/*
 * fmo_adc.c
 *
 *  Created on: 2014. 5. 8.
 *      Author: innocentevil
 */


#include "fmo_adc.h"
#include "fmo_dma.h"
#include "fmo_gpio.h"
#include "fmo_timer.h"
#include "../core/fmo_task.h"
#include "hw_descriptor_types.h"
#include "bl_config.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_adc.h"

#define ADC_SQR1_LEN_Pos                    ((uint8_t) 20)

#define PUBLIC_INTERFACE                   {\
	                                         lld_adc_open,\
	                                         lld_adc_close,\
	                                         lld_adc_convert,\
	                                         lld_adc_read,\
	                                         lld_adc_scan,\
                                            }


#define ADC_EvType_EOC                      ((uint8_t) 1)

#define ADC_STATE_OPEN_Pos                  ((uint8_t) 0)
#define ADC_STATE_OPEN_Msk                  ((uint16_t) 1 << ADC_STATE_OPEN_Pos)

#define ADC_STATE_BUSY_Pos                  ((uint8_t) 1)
#define ADC_STATE_BUSY_Msk                  ((uint16_t) 1 << ADC_STATE_BUSY_Pos)

#define ADC_STATE_BURST_BUSY_Pos            ((uint8_t) 2)
#define ADC_STATE_BURST_BUSY_Msk            ((uint16_t) 1 << ADC_STATE_BURST_BUSY_Pos)


#define ADC_SET_OPEN(ins)                   (ins->state |= ADC_STATE_OPEN_Msk)
#define ADC_CLR_OPEN(ins)                   (ins->state &= ~ADC_STATE_OPEN_Msk)
#define ADC_IS_OPEN(ins)                    (ins->state & ADC_STATE_OPEN_Msk)

#define ADC_SET_BUSY(ins)                   (ins->state |= ADC_STATE_BUSY_Msk)
#define ADC_CLR_BUSY(ins)                   (ins->state &= ~ADC_STATE_BUSY_Msk)
#define ADC_IS_BUSY(ins)                    (ins->state & ADC_STATE_BUSY_Msk)



#ifdef FEATURE_MTHREAD
#define ADC_LOCK(ins){\
	tch_Mtx_lockt(&ins->lock,MTX_TRYMODE_WAIT);\
}
#else
#define ADC_LOCK(ins){}
#endif


#ifdef FEATURE_MTHREAD
#define ADC_UNLOCK(ins){\
	tch_Mtx_unlockt(&ins->lock);\
}
#else
#define ADC_UNLOCK(ins){}
#endif

typedef struct _tch_adc_evQue tch_adc_evQue;
typedef struct _tch_adc_prototype tch_adc_prototype;


struct _tch_adc_evQue {
	tch_genericList_queue_t    dma_tcqueue;
};

struct _tch_adc_prototype {
	tch_adc_instance                _pix;
	uint16_t                         state;
	uint8_t                          idx;
	tch_dma_instance*               _dma_handle;
	tch_gpio_instance*              _io_handle;
	tch_pwm_instance*               _tim_handle;
	tch_adc_evQue                    evqueue;
#ifdef FEATURE_MTHREAD
	mtx_lock                         lock;
#endif
};




/**
 *
 * public function
 */
static BOOL lld_adc_open(const tch_adc_instance* self,tch_adc_cfg* cfg,tch_pwrMgrCfg pcfg);
static void lld_adc_close(const tch_adc_instance* self);
static uint16_t lld_adc_convert(const tch_adc_instance* self,uint8_t ch);
static BOOL lld_adc_read(const tch_adc_instance* self,uint16_t* rb,uint32_t size,uint16_t ch);
static BOOL lld_adc_scan(const tch_adc_instance* self,tch_adc_chlist* chlist,uint16_t* rb,uint32_t cnt);





/**
 *
 * private function
 */

static void inline lld_adc_reset(tch_adc_prototype* ins) __attribute__((always_inline));
static void inline lld_adc_setRegChannel(tch_adc_prototype* ins,uint8_t ch,uint8_t order)  __attribute__((always_inline));
static BOOL lld_adc_monitorEvent(const tch_adc_prototype* self,uint16_t evType);
static BOOL lld_adc_handle_dmaEvent(tch_adc_prototype* ins,uint16_t evtype);

static DMA_EVENTLISTENER(lld_adc1_dma_listener);
static DMA_EVENTLISTENER(lld_adc2_dma_listener);
static DMA_EVENTLISTENER(lld_adc3_dma_listener);

static GPT_TIMEOUT_LISTENER(lld_adc1_timer_listener);
static GPT_TIMEOUT_LISTENER(lld_adc2_timer_listener);
static GPT_TIMEOUT_LISTENER(lld_adc3_timer_listener);


/**
 * tch_adc_instance                _pix;
	uint16_t                         state;
	uint8_t                          idx;
	tch_dma_instance*               _dma_handle;
	tch_gpio_instance*              _io_handle;
	tch_gptimer_instance*           _timer_handle;
	tch_adc_readrequest              rreq;
#ifdef FEATURE_MTHREAD
	mtx_lock                         lock;
 */
__attribute__((section(".data"))) static tch_adc_prototype ADC_StaticInstances[] = {
		{
				PUBLIC_INTERFACE,
				0,
				0,
				NULL,
				NULL,
				{GENERIC_LIST_QUEUE_INIT}
#ifdef FEATURE_MTHREAD
				,MTX_INIT
#endif
		},
		{
				PUBLIC_INTERFACE,
				0,
				1,
				NULL,
				NULL,
				{GENERIC_LIST_QUEUE_INIT}
#ifdef FEATURE_MTHREAD
				,MTX_INIT
#endif
		},
		{
				PUBLIC_INTERFACE,
				0,
				2,
				NULL,
				NULL,
				{GENERIC_LIST_QUEUE_INIT}
#ifdef FEATURE_MTHREAD
				,MTX_INIT
#endif
		}
};

/**
 * 	ADC_TypeDef*        _hw;
	uint16_t             state;
	volatile uint32_t*  _clkenr;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       clkmsk;
	const uint32_t       lpclkmsk;
 */
static adc_hw_descriptor ADC_HWs[] = {
		{
				ADC1,
				0,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_ADC1EN,
				RCC_APB2LPENR_ADC1LPEN,
				3
		},
		{
				ADC2,
				0,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_ADC2EN,
				RCC_APB2LPENR_ADC2PEN,
				7
		},
		{
				ADC3,
				0,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_ADC3EN,
				RCC_APB2LPENR_ADC3LPEN,
				10,
		}
};


__attribute__((section(".data"))) static tch_dma_eventListener ADC_DMAListeners[] = {
		lld_adc1_dma_listener,
		lld_adc2_dma_listener,
		lld_adc3_dma_listener
};

const tch_adc_instance* adc1 = (tch_adc_instance*) &ADC_StaticInstances[0];
const tch_adc_instance* adc2 = (tch_adc_instance*) &ADC_StaticInstances[1];
const tch_adc_instance* adc3 = (tch_adc_instance*) &ADC_StaticInstances[2];


BOOL lld_adc_open(const tch_adc_instance* self,tch_adc_cfg* cfg,tch_pwrMgrCfg pcfg){
	tch_adc_prototype* ins = (tch_adc_prototype*) self;
	adc_hw_descriptor* ahw = &ADC_HWs[ins->idx];
	tch_bdc_adc* acfg = &ADC_BD_CFGs[ins->idx];
	if(ADC_IS_OPEN(ins)){
		return FALSE;
	}
#ifdef FEATURE_MTHREAD
	tch_Mtx_lockt(&ins->lock,MTX_TRYMODE_WAIT);
#endif
	ADC_SET_OPEN(ins);
#ifdef FEATURE_MTHREAD
	tch_Mtx_unlockt(&ins->lock);
#endif
	tch_gpio_cfg iocfg;
	tch_lld_gpio_cfgInit(&iocfg);
	iocfg.GPIO_LP_Mode = pcfg == ActOnSleep? GPIO_LP_Mode_ON : GPIO_LP_Mode_OFF;
	iocfg.GPIO_Mode = GPIO_Mode_AN;
	ins->_io_handle = tch_lld_getGpio(acfg->port,acfg->adc_pins,&iocfg);
	if(!ins->_io_handle)
			return FALSE;                                             /// io occupied any other instance, return fail

	if(!(*ahw->_clkenr & ahw->clkmsk)){                                /// check whether clock is enabled or not
		*ahw->_clkenr |= ahw->clkmsk;
	}
	switch(pcfg){
	case ActOnSleep:
		*ahw->_lpclkenr |= ahw->lpclkmsk;
		break;
	case DeactOnSleep:
		*ahw->_lpclkenr &= ~ahw->lpclkmsk;
		break;
	}

	tch_dma_cfg dmacfg;
	tch_lld_DMA_initCfg(&dmacfg);


	ins->_tim_handle = lld_timer_openTimerAsPulseOut(acfg->timer,(1000000 / cfg->ADC_SampleFreqInHz),pcfg);
	ins->_tim_handle->setDuty(ins->_tim_handle,acfg->tevt,(ins->_tim_handle->getMaxDuty(ins->_tim_handle) >> 1));
	lld_timer_setEventGeneration(acfg->timer,acfg->tevt,TRUE);

	dmacfg.DMA_BufferMode = DMA_BufferMode_Normal;
	dmacfg.DMA_Ch = acfg->adc_dma_ch;
	dmacfg.DMA_Dir = DMA_Dir_PeriphToMem;
	dmacfg.DMA_MBurst = DMA_Burst_Single;
	dmacfg.DMA_MDataAlign = DMA_DataAlign_Hword;
	dmacfg.DMA_Minc = DMA_Minc_Enable;
	dmacfg.DMA_PBurst = DMA_Burst_Single;
	dmacfg.DMA_PDataAlign = DMA_DataAlign_Hword;
	dmacfg.DMA_Pinc = DMA_Pinc_Disable;
	dmacfg.DMA_Prior = DMA_Prior_Med;

	ins->_dma_handle = tch_lld_DMA_getInstance(acfg->adc_dma,&dmacfg,pcfg);
	ins->_dma_handle->registerEventListener(ins->_dma_handle,ADC_DMAListeners[ins->idx],DMA_FLAG_EvTC | DMA_FLAG_EvTE);
	lld_adc_reset(ins);

	ahw->_hw->CR1 |= (cfg->ADC_Resolution << ADC_Resolution_Pos) | ADC_CR1_OVRIE;

	uint16_t pin = acfg->adc_pins;
	uint8_t pos = 0;
	uint8_t cnt = 0;
	while(pin){
		if(pin & 1){
			if(pos > 9){
				ahw->_hw->SMPR1 |= (cfg->ADC_SampleHold << (ADC_SampleHold_Offset * (10 - pos)));
			}else{
				ahw->_hw->SMPR2 |= (cfg->ADC_SampleHold << (ADC_SampleHold_Offset * pos));
			}
			lld_adc_setRegChannel(ins,pos,cnt++);
		}
		pin >>= 1;
		pos++;
	}
	ahw->_hw->SQR1 |= (cnt << ADC_SQR1_LEN_Pos);
	ahw->_hw->CR2 |= (ADC_CR2_EOCS | ADC_CR2_ADON | (ahw->extsel << 24));

	return TRUE;
}

void lld_adc_close(const tch_adc_instance* self){

}

uint16_t lld_adc_convert(const tch_adc_instance* self,uint8_t ch){
	tch_adc_prototype* ins = (tch_adc_prototype*) self;
	adc_hw_descriptor* ahw = &ADC_HWs[ins->idx];
	uint16_t av = 0;
	while(ADC_IS_BUSY(ins)){
		tchThread_sleep(0);
	}
	ADC_LOCK(ins);
	ADC_SET_BUSY(ins);
	lld_adc_setRegChannel(ins,ch,0);                      //set channel to convert
	ahw->_hw->CR2 |= ADC_CR2_DMA;
	ins->_dma_handle->setAddress(ins->_dma_handle,DMA_TargetAddress_Mem0,(uint32_t) &av);
	ins->_dma_handle->setAddress(ins->_dma_handle,DMA_TargetAddress_Periph,(uint32_t) &ahw->_hw->DR);
	ins->_dma_handle->beginXfer(ins->_dma_handle,1);
	ahw->_hw->CR2 |= ADC_CR2_SWSTART;
	while(ahw->_hw->CR2 & ADC_CR2_DMA){
		tchThread_sleep(0);
	}
	ADC_UNLOCK(ins);
	return av;
}
/**
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC |
                         RCC_APB2Periph_TIM1 |
                         RCC_APB2Periph_ADC1, ENABLE);
}

void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}

int main(void)
{
  ADC_InitTypeDef   ADC_InitStructure;
  DMA_InitTypeDef   DMA_InitStructure;
  TIM_TimeBaseInitTypeDef   TIM_TimeBaseStructure;
  TIM_OCInitTypeDef         TIM_OCInitStructure;

  RCC_Configuration();

  GPIO_Configuration();

  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period = 150 - 1;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 150 / 2;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
  TIM_OC1Init(TIM1, &TIM_OCInitStructure);

  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)ADC_RegularConvertedValueTab;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 64;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);

  DMA_Cmd(DMA1_Channel1, ENABLE);

  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);

  ADC_RegularChannelConfig(ADC1, ADC_Channel_14, 2, ADC_SampleTime_1Cycles5);

  ADC_ExternalTrigConvCmd(ADC1, ENABLE);

  ADC_DMACmd(ADC1, ENABLE);

  ADC_Cmd(ADC1, ENABLE);

  ADC_ResetCalibration(ADC1);

  while(ADC_GetResetCalibrationStatus(ADC1));

  ADC_StartCalibration(ADC1);

  while(ADC_GetCalibrationStatus(ADC1));

  TIM_Cmd(TIM1, ENABLE);

  TIM_CtrlPWMOutputs(TIM1, ENABLE);

  while(1)
  {
  }


    return(1);
}
 */


BOOL lld_adc_read(const tch_adc_instance* self,uint16_t* rb,uint32_t size,uint16_t ch){
	tch_adc_prototype* ins = (tch_adc_prototype*) self;
	adc_hw_descriptor* ahw = (adc_hw_descriptor*) &ADC_HWs[ins->idx];
	while(ADC_IS_BUSY(ins)){
		tchThread_sleep(0);
	}
	ADC_LOCK(ins);
	ADC_SET_BUSY(ins);
	lld_adc_setRegChannel(ins,ch,0);
	ins->_dma_handle->setAddress(ins->_dma_handle,DMA_TargetAddress_Mem0,(uint32_t) &rb);
	ins->_dma_handle->setAddress(ins->_dma_handle,DMA_TargetAddress_Periph,&ahw->_hw->DR);
	ahw->_hw->CR2 |= (1 << 28);
	ins->_tim_handle->start(ins->_tim_handle);
	ins->_dma_handle->beginXfer(ins->_dma_handle,size);
	while(ADC_IS_BUSY(ins)){
		if(!lld_adc_monitorEvent(ins,ADC_EvType_EOC)){
			ADC_UNLOCK(ins);
			return FALSE;
		}
	}
	ADC_UNLOCK(ins);
	return TRUE;
}

BOOL lld_adc_scan(const tch_adc_instance* self,tch_adc_chlist* chlist,uint16_t* rb,uint32_t cnt){

}

BOOL lld_adc_monitorEvent(const tch_adc_prototype* self,uint16_t evType){
	if(!ADC_IS_OPEN(self)){
		return FALSE;
	}
	switch(evType){
	case ADC_EvType_EOC:
		tchThread_wait(&self->evqueue.dma_tcqueue);
	}
	if(!ADC_IS_OPEN(self)){
		return FALSE;
	}
	return TRUE;
}

BOOL lld_adc_handle_dmaEvent(tch_adc_prototype* ins,uint16_t evtype){
	adc_hw_descriptor* ahw = &ADC_HWs[ins->idx];
	switch(evtype){
	case DMA_FLAG_EvTC:
		ahw->_hw->CR2 &= ~ADC_CR2_DMA;
		if(ins->evqueue.dma_tcqueue.entry){
			tchThread_wake(&ins->evqueue.dma_tcqueue);
		}
		ahw->_hw->CR2 &= ~ADC_CR2_EXTEN;
		ADC_CLR_BUSY(ins);
		return TRUE;
	}
}

DMA_EVENTLISTENER(lld_adc1_dma_listener){
	tch_adc_prototype* ains = &ADC_StaticInstances[0];
	return lld_adc_handle_dmaEvent(ains,evtype);
}

DMA_EVENTLISTENER(lld_adc2_dma_listener){
	tch_adc_prototype* ains = &ADC_StaticInstances[1];
	return lld_adc_handle_dmaEvent(ains,evtype);
}

DMA_EVENTLISTENER(lld_adc3_dma_listener){
	tch_adc_prototype* ains = &ADC_StaticInstances[2];
	return lld_adc_handle_dmaEvent(ains,evtype);
}


static void inline lld_adc_reset(tch_adc_prototype* ins){
	adc_hw_descriptor* ahw = &ADC_HWs[ins->idx];
	ahw->_hw->CR1 = 0;
	ahw->_hw->CR2 = 0;
	ahw->_hw->SR = 0;
	ahw->_hw->SMPR1 = 0;
	ahw->_hw->SMPR2 = 0;
	ahw->_hw->SQR1 = 0;
	ahw->_hw->SQR2 = 0;
	ahw->_hw->SQR3 = 0;
}

static void inline lld_adc_setRegChannel(tch_adc_prototype* ins,uint8_t ch,uint8_t order){
	adc_hw_descriptor* ahw = &ADC_HWs[ins->idx];
	if(order > 12){
		ahw->_hw->SQR1 |= (ch << ((order - 13) * 4));
	}else if(order > 6){
		ahw->_hw->SQR2 |= (ch << ((order - 7) * 4));
	}else{
		ahw->_hw->SQR3 |= (ch << (order * 4));
	}
}


void ADC_IRQHandler(void){
}
