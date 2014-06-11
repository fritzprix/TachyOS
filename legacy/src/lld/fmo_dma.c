/*
 * fmo_dma.c
 *
 *  Created on: 2014. 4. 14.
 *      Author: innocentevil
 */

#include "fmo_dma.h"



#define PUBLIC_INTERFACE                       {\
	                                              tch_lld_dma_beginXfer,\
	                                              tch_lld_dma_setAddress,\
	                                              tch_lld_dma_registerEventListener,\
	                                              tch_lld_dma_unregisterEventListener,\
	                                              tch_lld_dma_setIncrementMode,\
	                                              tch_lld_dma_close\
                                                }

#define INIT_DMA_INSTANCE(x)                    {\
	                                               PUBLIC_INTERFACE,\
	                                               MTX_INIT,\
	                                               x,\
	                                               0,\
	                                               NULL\
                                                 }


#ifdef FEATURE_MTHREAD
#define DMA_LOCK(ins_p) {tch_Mtx_lockt(&ins_p->acc_lock,TRUE);}
#else
#define DMA_LOCK(ins_p) {}
#endif


#ifdef FEATURE_MTHREAD
#define DMA_UNLOCK(ins_p) {tch_Mtx_unlockt(&ins_p->acc_lock);}
#else
#define DMA_UNLOCK(ins_p) {}
#endif


#define DMA_FLAG_OPEN                          (uint16_t) (1 << 0)
#define DMA_FLAG_BUSY                          (uint16_t) (1 << 1)
#define DMA_FLAG_SLEEPON                       (uint16_t) (1 << 2)

#define DMA_SET_SLEEPON(ins_p)                (ins_p->flag |= DMA_FLAG_SLEEPON)
#define DMA_CLR_SLEEPON(ins_p)                (ins_p->flag &= ~DMA_FLAG_SLEEPON)
#define DMA_IS_SLEEPON(ins_p)                 (ins_p->flag & DMA_FLAG_SLEEPON)

#define DMA_SET_OPENED(ins_p)                 (ins_p->flag |= DMA_FLAG_OPEN)
#define DMA_CLR_OPENED(ins_p)                 (ins_p->flag &= ~DMA_FLAG_OPEN)
#define DMA_IS_OPENED(ins_p)                  (ins_p->flag & DMA_FLAG_OPEN)

#define DMA_SET_BUSY(ins_p)                   (ins_p->flag |= DMA_FLAG_BUSY)
#define DMA_CLR_BUSY(ins_p)                   (ins_p->flag &= ~DMA_FLAG_BUSY)
#define DMA_IS_BUSY(ins_p)                    (ins_p->flag & DMA_FLAG_BUSY)

#define DMA_SET_EvFLAG(ins_p,ev)              (ins_p->flag |= (ev << DMA_FLAG_EvPos))
#define DMA_CLR_EvFLAG(ins_p)                 (ins_p->flag &= ~(DMA_FLAG_EvMsk <<  DMA_FLAG_EvPos))

#define DMA_GET_ISR(dhw_p)                    ((*dhw_p->_isr) >> dhw_p->ipos)
#define DMA_CLR_ISR(dhw_p,isr)                (*dhw_p->_icr |= (isr << dhw_p->ipos))

typedef struct _tch_dma_prototype tch_dma_prototype;


struct _tch_dma_prototype {
	tch_dma_instance            __public__;
	tch_mtx_lock                    acc_lock;
	uint8_t                     idx;
	uint16_t                    flag;
	tch_dma_eventListener       listener;
};

static BOOL tch_lld_dma_beginXfer(tch_dma_instance* self,uint32_t size);
static void tch_lld_dma_setAddress(tch_dma_instance* self,uint8_t targetAddress,uint32_t addr);
static void tch_lld_dma_registerEventListener(tch_dma_instance* self,tch_dma_eventListener listener,uint16_t evType);
static void tch_lld_dma_unregisterEventListener(tch_dma_instance* self);
static void tch_lld_dma_setIncrementMode(tch_dma_instance* self,uint8_t targetAddress,BOOL enable);
static void tch_lld_dma_close(tch_dma_instance* self);

static BOOL __handle_dma_event(tch_dma_prototype* ins,dma_hw_descriptor* dhw);


__attribute__((section(".data"))) static tch_dma_prototype DMA_StaticInstances[] = {
		INIT_DMA_INSTANCE(0),
		INIT_DMA_INSTANCE(1),
		INIT_DMA_INSTANCE(2),
		INIT_DMA_INSTANCE(3),
		INIT_DMA_INSTANCE(4),
		INIT_DMA_INSTANCE(5),
		INIT_DMA_INSTANCE(6),
		INIT_DMA_INSTANCE(7),
		INIT_DMA_INSTANCE(8),
		INIT_DMA_INSTANCE(9),
		INIT_DMA_INSTANCE(10),
		INIT_DMA_INSTANCE(11),
		INIT_DMA_INSTANCE(12),
		INIT_DMA_INSTANCE(13),
		INIT_DMA_INSTANCE(14),
		INIT_DMA_INSTANCE(15)
};


/**
 *
 */
__attribute__((section(".data"))) static dma_hw_descriptor DMA_HWs[] = {
		{
				DMA1_Stream0,
				0,
				&DMA1->LISR,
				&DMA1->LIFCR,
				0,
				DMA1_Stream0_IRQn
		},
		{
				DMA1_Stream1,
				0,
				&DMA1->LISR,
				&DMA1->LIFCR,
				6,
				DMA1_Stream1_IRQn
		},
		{
				DMA1_Stream2,
				0,
				&DMA1->LISR,
				&DMA1->LIFCR,
				16,
				DMA1_Stream2_IRQn
		},
		{
				DMA1_Stream3,
				0,
				&DMA1->LISR,
				&DMA1->LIFCR,
				22,
				DMA1_Stream3_IRQn
		},
		{
				DMA1_Stream4,
				0,
				&DMA1->HISR,
				&DMA1->HIFCR,
				0,
				DMA1_Stream4_IRQn
		},
		{
				DMA1_Stream5,
				0,
				&DMA1->HISR,
				&DMA1->HIFCR,
				6,
				DMA1_Stream5_IRQn
		},
		{
				DMA1_Stream6,
				0,
				&DMA1->HISR,
				&DMA1->HIFCR,
				16,
				DMA1_Stream6_IRQn
		},
		{
				DMA1_Stream7,
				0,
				&DMA1->HISR,
				&DMA1->HIFCR,
				22,
				DMA1_Stream7_IRQn
		},
		{
				DMA2_Stream0,
				0,
				&DMA2->LISR,
				&DMA2->LIFCR,
				0,
				DMA2_Stream0_IRQn
		},
		{
				DMA2_Stream1,
				0,
				&DMA2->LISR,
				&DMA2->LIFCR,
				6,
				DMA2_Stream1_IRQn
		},
		{
				DMA2_Stream2,
				0,
				&DMA2->LISR,
				&DMA2->LIFCR,
				16,
				DMA2_Stream2_IRQn
		},
		{
				DMA2_Stream3,
				0,
				&DMA2->LISR,
				&DMA2->LIFCR,
				22,
				DMA2_Stream3_IRQn
		},
		{
				DMA2_Stream4,
				0,
				&DMA2->HISR,
				&DMA2->HIFCR,
				0,
				DMA2_Stream4_IRQn
		},
		{
				DMA2_Stream5,
				0,
				&DMA2->HISR,
				&DMA2->HIFCR,
				6,
				DMA2_Stream5_IRQn
		},
		{
				DMA2_Stream6,
				0,
				&DMA2->HISR,
				&DMA2->HIFCR,
				16,
				DMA2_Stream6_IRQn
		},
		{
				DMA2_Stream7,
				0,
				&DMA2->HISR,
				&DMA2->HIFCR,
				22,
				DMA2_Stream7_IRQn
		}
};

static __attribute__((section(".data"))) dma_com_hw_desc_t DMA_COM_HWs = {{0,0}};

void tch_lld_dma_cfginit(tch_dma_cfg* cfg){

}

tch_dma_instance* tch_lld_dma_init(dma_t dma,tch_dma_cfg* dmacfg,tch_pwrMgrCfg cfg){
	tch_dma_prototype* ins = &DMA_StaticInstances[dma];
	dma_hw_descriptor* dhw = &DMA_HWs[dma];

	if(DMA_IS_OPENED(ins)){
		return NULL;
	}

	DMA_LOCK(ins);
	DMA_SET_OPENED(ins);
	DMA_UNLOCK(ins);
	if(dma < 8){
		DMA_COM_HWs.occup_flag[0] |= (1 << dma);
		if((RCC->AHB1ENR & RCC_AHB1ENR_DMA1EN) == 0){
			RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
		}
		if(cfg == ActOnSleep){
			RCC->AHB1LPENR |= RCC_AHB1LPENR_DMA1LPEN;
			DMA_COM_HWs.lpoccup_flag[0] |= (1 << dma);
		}
	}else{
		DMA_COM_HWs.occup_flag[1] |= (1 << (dma - 8));
		if((RCC->AHB1ENR & RCC_AHB1ENR_DMA2EN) == 0){
			RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
		}
		if(cfg == ActOnSleep){
			RCC->AHB1LPENR |= RCC_AHB1LPENR_DMA2LPEN;
			DMA_COM_HWs.lpoccup_flag[1] |= (1 << (dma - 8));
		}
	}

	dhw->_hw->CR |= (dmacfg->DMA_Ch << DMA_Ch_Pos) | (dmacfg->DMA_Dir << DMA_Dir_Pos) | (dmacfg->DMA_FlowControl << DMA_FlowControl_Pos);
	dhw->_hw->CR |= ((dmacfg->DMA_MBurst << DMA_MBurst_Pos) | (dmacfg->DMA_PBurst << DMA_PBurst_Pos));
	dhw->_hw->CR |= ((dmacfg->DMA_MDataAlign << DMA_MDataAlign_Pos) | (dmacfg->DMA_PDataAlign << DMA_PDataAlign_Pos));
	dhw->_hw->CR |= ((dmacfg->DMA_Minc << DMA_Minc_Pos) | (dmacfg->DMA_Pinc << DMA_Pinc_Pos));
	dhw->_hw->CR |= dmacfg->DMA_Prior << DMA_Prior_Pos;
	dhw->_hw->CR |= DMA_SxCR_TCIE;

	NVIC_EnableIRQ(dhw->irq);
	NVIC_SetPriority(dhw->irq,HANDLER_NORMAL_PRIOR);

	return (tch_dma_instance*) ins;
}


BOOL tch_lld_dma_beginXfer(tch_dma_instance* self,uint32_t size){
	tch_dma_prototype* ins = (tch_dma_prototype*) self;
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	if(DMA_IS_BUSY(ins)){
		return FALSE;
	}
	DMA_LOCK(ins);
	DMA_SET_BUSY(ins);
	dhw->_hw->NDTR = size;
	__DMB();
	__ISB();
	dhw->_hw->CR |= DMA_SxCR_EN;
	DMA_UNLOCK(ins);
	return TRUE;

}

void tch_lld_dma_setAddress(tch_dma_instance* self,uint8_t targetAddress,uint32_t addr){
	tch_dma_prototype* ins = (tch_dma_prototype*) self;
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	switch(targetAddress){
	case DMA_TargetAddress_Mem0:
		dhw->_hw->M0AR = addr;
		return;
	case DMA_TargetAddress_Mem1:
		dhw->_hw->M1AR = addr;
		break;
	case DMA_TargetAddress_Periph:
		dhw->_hw->PAR = addr;
		break;
	}
}

void tch_lld_dma_registerEventListener(tch_dma_instance* self,tch_dma_eventListener listener,uint16_t evType){
	tch_dma_prototype* ins = (tch_dma_prototype*) self;
	dma_hw_descriptor* dhw = (dma_hw_descriptor*) &DMA_HWs[ins->idx];
	NVIC_DisableIRQ(dhw->irq);
	__DMB();
	__ISB();
	dhw->_hw->CR |= (evType >> 1);
	dhw->_hw->FCR |= ((evType & (1 << 0)) << 7);
	DMA_SET_EvFLAG(ins,evType);
	ins->listener = listener;
	__DMB();
	__ISB();
	NVIC_EnableIRQ(dhw->irq);
}

void tch_lld_dma_unregisterEventListener(tch_dma_instance* self){
	tch_dma_prototype* ins = (tch_dma_prototype*) self;
	dma_hw_descriptor* dhw = (dma_hw_descriptor*) &DMA_HWs[ins->idx];
	NVIC_DisableIRQ(dhw->irq);
	__DMB();
	__ISB();
	dhw->_hw->CR  &= ~(DMA_FLAG_EvMsk >> 1);
	dhw->_hw->FCR &=~ (1 << 7);
	DMA_CLR_EvFLAG(ins);
	ins->listener = NULL;
	__DMB();
	__ISB();
	NVIC_EnableIRQ(dhw->irq);
}

void tch_lld_dma_setIncrementMode(tch_dma_instance* self,uint8_t targetAddress,BOOL enable){
	tch_dma_prototype* ins = (tch_dma_prototype*) self;
	dma_hw_descriptor* dhw = (dma_hw_descriptor*) &DMA_HWs[ins->idx];
	switch(targetAddress){
	case DMA_TargetAddress_Mem0:
	case DMA_TargetAddress_Mem1:
		if(enable){
			dhw->_hw->CR |= DMA_SxCR_MINC;
		}else{
			dhw->_hw->CR &= ~DMA_SxCR_MINC;
		}
		break;
	case DMA_TargetAddress_Periph:
		if(enable){
			dhw->_hw->CR |= DMA_SxCR_PINC;
		}else{
			dhw->_hw->CR &= ~DMA_SxCR_PINC;
		}
		break;
	}
}


void tch_lld_dma_close(tch_dma_instance* self){
	tch_dma_prototype* ins = (tch_dma_prototype*) self;
	dma_hw_descriptor* dhw = (dma_hw_descriptor*) &DMA_HWs[ins->idx];
	while(DMA_IS_BUSY(ins)){
		tchThread_sleep(0);
	}
	dhw->_hw->CR = 0;                                                              /// set cr to reset state
	if(ins->idx > 7){
		DMA_COM_HWs.occup_flag[0] &= ~(1 << ins->idx);
		if(DMA_IS_SLEEPON(ins)){
			DMA_CLR_SLEEPON(ins);
			DMA_COM_HWs.lpoccup_flag[0] &= ~(1 << ins->idx);
			if(!DMA_COM_HWs.lpoccup_flag[0]){
				RCC->AHB1LPENR &= ~RCC_AHB1LPENR_DMA1LPEN;
			}
		}
		if(!DMA_COM_HWs.occup_flag[0]){
			RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA1EN;
		}
	}else{
		DMA_COM_HWs.occup_flag[1] &= ~(1 << (ins->idx - 8));
		if(DMA_IS_SLEEPON(ins)){
			DMA_CLR_SLEEPON(ins);
			DMA_COM_HWs.lpoccup_flag[1] &= ~(1 << (ins->idx));
			if(!DMA_COM_HWs.lpoccup_flag[1]){
				RCC->AHB1LPENR &=~ RCC_AHB1LPENR_DMA2LPEN;
			}
		}
		if(!DMA_COM_HWs.occup_flag[1]){
			RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA2EN;
		}
	}

	__DMB();
	__ISB();
	NVIC_DisableIRQ(dhw->irq);
	DMA_CLR_OPENED(ins);
}


BOOL __handle_dma_event(tch_dma_prototype* ins,dma_hw_descriptor* dhw){
	if(ins->listener == NULL){
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvTC)){
			DMA_CLR_ISR(dhw,DMA_FLAG_EvTC);
		}
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvTE)){
			DMA_CLR_ISR(dhw,DMA_FLAG_EvTE);
		}
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvFE)){
			DMA_CLR_ISR(dhw,DMA_FLAG_EvFE);
		}
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvDME)){
			DMA_CLR_ISR(dhw,DMA_FLAG_EvDME);
		}
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvHT)){
			DMA_CLR_ISR(dhw,DMA_FLAG_EvHT);
		}
		return FALSE;
	}else{
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvTC)){
			if(ins->listener((tch_dma_instance*)ins,DMA_FLAG_EvTC)){
				DMA_CLR_ISR(dhw,DMA_FLAG_EvTC);
				DMA_CLR_BUSY(ins);
				return TRUE;
			}
			return FALSE;
		}
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvTE)){
			if(ins->listener((tch_dma_instance*)ins,DMA_FLAG_EvTE)){
				DMA_CLR_ISR(dhw,DMA_FLAG_EvTE);
				return TRUE;
			}
			return FALSE;
		}
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvFE)){
			if(ins->listener((tch_dma_instance*)ins,DMA_FLAG_EvFE)){
				DMA_CLR_ISR(dhw,DMA_FLAG_EvFE);
				return TRUE;
			}
			return FALSE;
		}
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvDME)){
			if(ins->listener((tch_dma_instance*)ins,DMA_FLAG_EvDME)){
				DMA_CLR_ISR(dhw,DMA_FLAG_EvDME);
				return TRUE;
			}
			return FALSE;
		}
		if((DMA_GET_ISR(dhw) & DMA_FLAG_EvHT)){
			if(ins->listener((tch_dma_instance*)ins,DMA_FLAG_EvHT)){
				DMA_CLR_ISR(dhw,DMA_FLAG_EvHT);
				return TRUE;
			}
			return FALSE;
		}
	}
	return FALSE;
}


void DMA1_Stream0_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA1_Str0];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA1_Stream1_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA1_Str1];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA1_Stream2_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA1_Str2];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA1_Stream3_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA1_Str3];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA1_Stream4_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA1_Str4];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA1_Stream5_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA1_Str5];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA1_Stream6_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA1_Str6];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA1_Stream7_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA1_Str7];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA2_Stream0_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA2_Str0];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA2_Stream1_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA2_Str1];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA2_Stream2_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA2_Str2];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA2_Stream3_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA2_Str3];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA2_Stream4_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA2_Str4];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA2_Stream5_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA2_Str5];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA2_Stream6_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA2_Str6];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}

void DMA2_Stream7_IRQHandler(void){
	tch_dma_prototype* ins = &DMA_StaticInstances[DMA2_Str7];
	dma_hw_descriptor* dhw = &DMA_HWs[ins->idx];
	__handle_dma_event(ins,dhw);
}


