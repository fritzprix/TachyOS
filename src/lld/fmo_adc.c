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

#define ADC_SQR1_LEN_Pos                    ((uint8_t) 20)
#define ADC_CH_CNT                          ((uint8_t) 19)


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


/**
 * Macro for modifying status flag of adc instance
 */
#define ADC_SET_OPEN(ins)                   (ins->state |= ADC_STATE_OPEN_Msk)
#define ADC_CLR_OPEN(ins)                   (ins->state &= ~ADC_STATE_OPEN_Msk)
#define ADC_IS_OPEN(ins)                    (ins->state & ADC_STATE_OPEN_Msk)

#define ADC_SET_BUSY(ins)                   (ins->state |= ADC_STATE_BUSY_Msk)
#define ADC_CLR_BUSY(ins)                   (ins->state &= ~ADC_STATE_BUSY_Msk)
#define ADC_IS_BUSY(ins)                    (ins->state & ADC_STATE_BUSY_Msk)



#ifdef FEATURE_MTHREAD
#define ADC_LOCK(ins){\
	tch_Mtx_lockt(&ins->lock,TRUE);\
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
typedef struct _tch_adc_iohandle_t tch_adc_iohandle;

struct _tch_adc_iohandle_t {
	tch_gpio_instance*     _ain_handle;
	uint8_t                 ain_pin;
};

struct _tch_adc_evQue {
	tch_genericList_queue_t    dma_tcqueue;
};

struct _tch_adc_prototype {
	tch_adc_instance                _pix;
	uint16_t                         state;
	uint8_t                          idx;
	tch_dma_instance*               _dma_handle;
	tch_pwm_instance*               _tim_handle;
	tch_adc_evQue                    evqueue;
#ifdef FEATURE_MTHREAD
	tch_mtx_lock                         lock;
#endif
	tch_adc_iohandle                 io_handles[19];
};




/**
 *
 * public function
 */
static BOOL lld_adc_open(const tch_adc_instance* self,tch_adc_cfg* cfg,tch_pwrMgrCfg pcfg);
static BOOL lld_adc_close(const tch_adc_instance* self);
static uint16_t lld_adc_convert(const tch_adc_instance* self,uint8_t ch);
static BOOL lld_adc_read(const tch_adc_instance* self,uint16_t* rb,uint32_t size,uint16_t ch);
static BOOL lld_adc_scan(const tch_adc_instance* self,uint8_t* chlist,uint16_t* rb,uint32_t cnt);





/**
 *
 * private function
 */

static void inline lld_adc_reset(tch_adc_prototype* ins) __attribute__((always_inline));
static void inline lld_adc_setRegChannel(tch_adc_prototype* ins,uint8_t ch,uint8_t order)  __attribute__((always_inline));
static void inline lld_adc_setRegSampleHold(tch_adc_prototype* ins,uint8_t ch,uint8_t ADC_SampleHold) __attribute__((always_inline));
static void inline lld_adc_setRegScanCount(tch_adc_prototype* ins,uint8_t cnt) __attribute__((always_inline));
static BOOL lld_adc_monitorEvent(const tch_adc_prototype* self,uint16_t evType);
static BOOL lld_adc_handle_dmaEvent(tch_adc_prototype* ins,uint16_t evtype);

static DMA_EVENTLISTENER(lld_adc1_dma_listener);
static DMA_EVENTLISTENER(lld_adc2_dma_listener);
static DMA_EVENTLISTENER(lld_adc3_dma_listener);



/**
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
				,{0,}
		}
#ifndef STM32F401x
		,{
				PUBLIC_INTERFACE,
				0,
				1,
				NULL,
				NULL,
				{GENERIC_LIST_QUEUE_INIT}
#ifdef FEATURE_MTHREAD
				,MTX_INIT
#endif
				,{0,}
		}
		,{
				PUBLIC_INTERFACE,
				0,
				2,
				NULL,
				NULL,
				{GENERIC_LIST_QUEUE_INIT}
#ifdef FEATURE_MTHREAD
				,MTX_INIT
#endif
				,{0,}
		}
#endif
};

/**
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
		}
#ifndef STM32F401x
		,{
				ADC2,
				0,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_ADC2EN,
				RCC_APB2LPENR_ADC2PEN,
				7
		}
		,{
				ADC3,
				0,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_ADC3EN,
				RCC_APB2LPENR_ADC3LPEN,
				10,
		}
#endif
};


__attribute__((section(".data"))) static tch_dma_eventListener ADC_DMAListeners[] = {
		lld_adc1_dma_listener
#ifndef STM32F401x
		,lld_adc2_dma_listener
		,lld_adc3_dma_listener
#endif
};

const tch_adc_instance* adc1 = (tch_adc_instance*) &ADC_StaticInstances[0];
#ifndef STM32F401x
const tch_adc_instance* adc2 = (tch_adc_instance*) &ADC_StaticInstances[1];
const tch_adc_instance* adc3 = (tch_adc_instance*) &ADC_StaticInstances[2];
#endif


void tch_lld_adc_initCfg(tch_adc_cfg* cfg){
	cfg->ADC_ChCnt = 0;
	cfg->ADC_ChCfg_List = NULL;
	cfg->ADC_Resolution = ADC_Resolution_12B;
	cfg->ADC_SampleFreqInHz = 1000;
	cfg->ADC_SampleHold = ADC_SampleHold_3Cycle;
}


/**
 *  Initiate ADC Object
 *  @arg1 self : instance to open
 *  @arg2 cfg  : adc configuration
 *  @arg3 pcfg : power management option (will be inherited to child member instance)
 */
BOOL lld_adc_open(const tch_adc_instance* self,tch_adc_cfg* cfg,tch_pwrMgrCfg pcfg){
	tch_adc_prototype* ins = (tch_adc_prototype*) self;
	adc_hw_descriptor* ahw = &ADC_HWs[ins->idx];
	tch_bdc_adc* acfg = &ADC_BD_CFGs[ins->idx];
	if(ADC_IS_OPEN(ins)){
		return FALSE;
	}
	ADC_LOCK(ins);
	ADC_SET_OPEN(ins);
	ADC_UNLOCK(ins);

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


	tch_gpio_cfg iocfg;
	tch_lld_gpio_cfgInit(&iocfg);
	iocfg.GPIO_Mode = GPIO_Mode_AN;
	if(cfg->ADC_ChCnt == 0){
		ins->io_handles[0].ain_pin = acfg->adc_pin;
		ins->io_handles[0]._ain_handle = tch_lld_gpio_init(acfg->port,1 << acfg->adc_pin,&iocfg,pcfg);
		if(!ins->io_handles[0]._ain_handle)
			return FALSE;                                             /// io occupied any other instance, return fail
		lld_adc_setRegChannel(ins,acfg->adc_pin,0);
		lld_adc_setRegSampleHold(ins,acfg->adc_pin,cfg->ADC_SampleHold);
		lld_adc_setRegScanCount(ins,1);
	}else{
		uint8_t idx = 0;
		while(idx < cfg->ADC_ChCnt){
			ins->io_handles[idx].ain_pin = cfg->ADC_ChCfg_List->pin;
			ins->io_handles[idx]._ain_handle = tch_lld_gpio_init(cfg->ADC_ChCfg_List[idx].port,1 << cfg->ADC_ChCfg_List[idx].pin,&iocfg,pcfg);
			if(!ins->io_handles[idx]._ain_handle){
				return FALSE;
			}
			lld_adc_setRegSampleHold(ins,cfg->ADC_ChCfg_List[idx].ch,cfg->ADC_SampleHold);
		}
		lld_adc_setRegScanCount(ins,cfg->ADC_ChCnt);
	}


	tch_dma_cfg dmacfg;
	tch_lld_dma_cfginit(&dmacfg);


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

	ins->_dma_handle = tch_lld_dma_init(acfg->adc_dma,&dmacfg,pcfg);
	ins->_dma_handle->registerEventListener(ins->_dma_handle,ADC_DMAListeners[ins->idx],DMA_FLAG_EvTC | DMA_FLAG_EvTE);
	lld_adc_reset(ins);

	ahw->_hw->CR1 |= (cfg->ADC_Resolution << ADC_Resolution_Pos) | ADC_CR1_OVRIE;
	ahw->_hw->CR2 |= (ADC_CR2_ADON | (ahw->extsel << 24));

	return TRUE;
}

BOOL lld_adc_close(const tch_adc_instance* self){
	tch_adc_prototype* ins = (tch_adc_prototype*) self;
	adc_hw_descriptor* ahw = &ADC_HWs[ins->idx];
	if(!ADC_IS_OPEN(ins)){
		return FALSE;
	}
	ADC_LOCK(ins);
	ADC_CLR_OPEN(ins);
	ADC_UNLOCK(ins);

	while(ADC_IS_BUSY(ins)){
		tchThread_sleep(1);
	}
	ahw->_hw->CR2 = 0;
	ahw->_hw->CR1 = 0;
	ins->_dma_handle->close(ins->_dma_handle);
	ins->_tim_handle->stop(ins->_tim_handle);
	ins->_tim_handle->close(ins->_tim_handle);
	uint8_t idx = 0;
	while(idx < ADC_CH_CNT){
		if(ins->io_handles[idx]._ain_handle){
			ins->io_handles[idx]._ain_handle->free(ins->io_handles[idx]._ain_handle,1 << ins->io_handles[idx].ain_pin);
		}
		idx++;
	}
	*ahw->_clkenr &= ~(ahw->clkmsk);
	*ahw->_lpclkenr &= ~(ahw->lpclkmsk);
	ahw->status = 0;
	return TRUE;

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
	lld_adc_setRegScanCount(ins,1);
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
	lld_adc_setRegScanCount(ins,1);
	ahw->_hw->CR2 |= ADC_CR2_DMA;
	ins->_dma_handle->setAddress(ins->_dma_handle,DMA_TargetAddress_Mem0,(uint32_t) rb);
	ins->_dma_handle->setAddress(ins->_dma_handle,DMA_TargetAddress_Periph,(uint32_t) &ahw->_hw->DR);
	ahw->_hw->CR2 |= (1 << 28);
	ins->_dma_handle->beginXfer(ins->_dma_handle,size);
	ins->_tim_handle->start(ins->_tim_handle);
	while(ADC_IS_BUSY(ins)){
		if(!lld_adc_monitorEvent(ins,ADC_EvType_EOC)){
			ADC_UNLOCK(ins);
			return FALSE;
		}
	}
	ADC_UNLOCK(ins);
	return TRUE;
}

BOOL lld_adc_scan(const tch_adc_instance* self,uint8_t* chlist,uint16_t* rb,uint32_t cnt){
	tch_adc_prototype* ins = (tch_adc_prototype*) self;
	adc_hw_descriptor* ahw = (adc_hw_descriptor*) &ADC_HWs[ins->idx];
	while(ADC_IS_BUSY(ins)){
		tchThread_sleep(0);
	}
	ADC_LOCK(ins);
	ADC_SET_BUSY(ins);
	uint8_t idx = 0;
	while(idx < cnt){
		lld_adc_setRegChannel(ins,chlist[idx],idx);
	}
	lld_adc_setRegScanCount(ins,cnt);
	ahw->_hw->CR2 |= ADC_CR2_DMA;
	ins->_dma_handle->setAddress(ins->_dma_handle,DMA_TargetAddress_Mem0,(uint32_t) rb);
	ins->_dma_handle->setAddress(ins->_dma_handle,DMA_TargetAddress_Periph,(uint32_t) &ahw->_hw->DR);
	ahw->_hw->CR2 |= (1 << 28);
	ins->_dma_handle->beginXfer(ins->_dma_handle,cnt);
	ins->_tim_handle->start(ins->_tim_handle);
	while(ADC_IS_BUSY(ins)){
		if(!lld_adc_monitorEvent(ins,ADC_EvType_EOC)){
			ADC_UNLOCK(ins);
			return FALSE;
		}
	}
	ADC_UNLOCK(ins);
	return TRUE;
}

BOOL lld_adc_monitorEvent(const tch_adc_prototype* self,uint16_t evType){
	if(!ADC_IS_OPEN(self)){
		return FALSE;
	}
	switch(evType){
	case ADC_EvType_EOC:
		tchThread_wait((tch_genericList_queue_t*)&self->evqueue.dma_tcqueue);
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
		ahw->_hw->CR2 &= ~ADC_CR2_EXTEN;
		ins->_tim_handle->stop(ins->_tim_handle);
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


static void inline lld_adc_setRegSampleHold(tch_adc_prototype* ins,uint8_t ch,uint8_t ADC_SampleHold){
	adc_hw_descriptor* ahw = &ADC_HWs[ins->idx];
	if(ch > 9){
		ahw->_hw->SMPR1 &= ~(7 << ((ch - 10) * 3));
		ahw->_hw->SMPR1 |= ADC_SampleHold << ((ch - 10) * 3);
	}else{
		ahw->_hw->SMPR1 &= ~(7 << (ch * 3));
		ahw->_hw->SMPR1 |= ADC_SampleHold << (ch * 3);
	}
}

static void inline lld_adc_setRegScanCount(tch_adc_prototype* ins,uint8_t cnt){
	adc_hw_descriptor* ahw = &ADC_HWs[ins->idx];
	ahw->_hw->SQR1 &= ~(0xF << ADC_SQR1_LEN_Pos);
	ahw->_hw->SQR1 |= (cnt << ADC_SQR1_LEN_Pos);
}



void ADC_IRQHandler(void){
	//nop
}
