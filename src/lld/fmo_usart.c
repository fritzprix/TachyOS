/*
 * fmo_usart.c
 *
 *  Created on: 2014. 4. 17.
 *      Author: innocentevil
 */


#include "fmo_usart.h"
#include "fmo_dma.h"
#include "bl_config.h"
#include "stm32f4xx.h"
#include "../util/generic_data_types.h"
#include "../core/fmo_lldabs.h"


#ifndef USART_BUFFER_SIZE
#define USART_BUFFER_SIZE                    (uint16_t) (1 << 9)
#endif

#define PUBLIC_INTERFACE                     {\
	                                           lld_usart_open,\
	                                           lld_usart_close,\
	                                           lld_usart_putc,\
	                                           lld_usart_getc,\
	                                           lld_usart_write,\
	                                           lld_usart_read,\
	                                           lld_usart_available,\
	                                           lld_usart_writeCstr,\
	                                           lld_usart_readCstr\
                                              }

#define USART_EVType_RXNE                     (uint16_t) (1 << 5)
#define USART_EVType_TC                       (uint16_t) (1 << 6)
#define USART_EVType_TCDMA                    (uint16_t) (1 << 7)

#define USART_FLAG_TXERR_Pos                  (uint16_t) (4)
#define USART_FLAG_RXERR_Pos                  (uint16_t) (8)

#define USART_FLAG_ERR_OVR                    (uint16_t) (1 << 0)
#define USART_FLAG_ERR_PARITY                 (uint16_t) (1 << 1)
#define USART_FLAG_ERR_FRAME                  (uint16_t) (1 << 2)
#define USART_FLAG_ERR_DMA                    (uint16_t) (1 << 3)

#define USART_FLAG_ERR_Msk                    (USART_FLAG_ERR_OVR |\
		                                       USART_FLAG_ERR_DMA |\
		                                       USART_FLAG_ERR_FRAME|\
		                                       USART_FLAG_ERR_PARITY)

#define INIT_USART_EVQ                        {GENERIC_LIST_QUEUE_INIT,GENERIC_LIST_QUEUE_INIT}

#define USART_FLAG_OPENED                     (uint16_t) (1 << 0)
#define USART_FLAG_ACTIVE_INSLEEP             (uint16_t) (1 << 1)
#define USART_FLAG_RXBUSY                     (uint16_t) (1 << 2)
#define USART_FLAG_TXBUSY                     (uint16_t) (1 << 3)

#define USART_SET_OPENED(ins)                 (ins->flag |= USART_FLAG_OPENED)
#define USART_CLR_OPENED(ins)                 (ins->flag &= ~USART_FLAG_OPENED)
#define USART_IS_OPENED(ins)                  (ins->flag & USART_FLAG_OPENED)

#define USART_SET_ACTIVE_INSLEEP(ins)         (ins->flag |= USART_FLAG_ACTIVE_INSLEEP)
#define USART_CLR_ACTIVE_INSLEEP(ins)         (ins->flag &= ~USART_FLAG_ACTIVE_INSLEEP)
#define USART_IS_ACTIVE_INSLEEP(ins)          (ins->flag & USART_FLAG_ACTIVE_INSLEEP)

#define USART_SET_RXBUSY(ins)                 (ins->flag |= USART_FLAG_RXBUSY)
#define USART_CLR_RXBUSY(ins)                 (ins->flag &= ~USART_FLAG_RXBUSY)
#define USART_IS_RXBUSY(ins)                  (ins->flag & USART_FLAG_RXBUSY)

#define USART_SET_TXBUSY(ins)                 (ins->flag |= USART_FLAG_TXBUSY)
#define USART_CLR_TXBUSY(ins)                 (ins->flag &= ~USART_FLAG_TXBUSY)
#define USART_IS_TXBUSY(ins)                  (ins->flag & USART_FLAG_TXBUSY)

#define USART_SET_TXERR(ins,err)              (ins->flag |= (err << USART_FLAG_TXERR_Pos))
#define USART_CLR_TXERR(ins)                  (ins->flag &= ~(USART_FLAG_ERR_Msk << USART_FLAG_TXERR_Pos))
#define USART_GET_TXERR(ins)                  ((ins->flag & (USART_FLAG_ERR_Msk << USART_FLAG_TXERR_Pos)) >> USART_FLAG_TXERR_Pos)

#define USART_SET_RXERR(ins,err)              (ins->flag |= (err << USART_FLAG_RXERR_Pos))
#define USART_CLR_RXERR(ins)                  (ins->flag &= ~(USART_FLAG_ERR_Msk << USART_FLAG_RXERR_Pos))
#define USART_GET_RXERR(ins)                  ((ins->flag & (USART_FLAG_ERR_Msk << USART_FLAG_RXERR_Pos)) >> USART_FLAG_RXERR_Pos)


typedef struct _tch_usart_prototype_t tch_usart_prototype;
typedef struct _tch_usart_eventque_t tch_usart_evQue;
typedef struct _tch_usart_stream_t tch_usart_stream;

struct _tch_usart_eventque_t {
	tch_genericList_queue_t         rxneque;
	tch_genericList_queue_t         tcque;
	tch_genericList_queue_t         txdmaque;
	tch_genericList_queue_t         rxdmaque;
};

struct _tch_usart_stream_t {
	tch_iostream_buffer             iostr;
	tch_istream*                    istr;
	tch_ostream*                    ostr;
	uint8_t                        iobuffer[USART_BUFFER_SIZE];
};

struct _tch_usart_prototype_t{
	tch_usart_instance            _public_;
	mtx_lock                       wacc_lock;
	mtx_lock                       racc_lock;
	uint16_t                       flag;
	uint8_t                        idx;
	tch_dma_instance*              tx_dma_handle;
	tch_dma_instance*              rx_dma_handle;
	tch_gpio_instance*              gpiov_handle;
	tch_usart_evQue                evQue;
};




static BOOL lld_usart_open(const tch_usart_instance* self,uint32_t brate,tch_pwrMgrCfg pmopt,tch_usart_cfg* cfg);
static BOOL lld_usart_close(const tch_usart_instance* self);
static BOOL lld_usart_putc(const tch_usart_instance* self,uint8_t c);
static uint8_t lld_usart_getc(const tch_usart_instance* self);
static BOOL lld_usart_write(const tch_usart_instance* self,const uint8_t* bp,uint32_t size,uint16_t* err);
static BOOL lld_usart_read(const tch_usart_instance* self,uint8_t* bp,uint32_t size,uint32_t timeout,uint16_t* err);
uint32_t lld_usart_available(const tch_usart_instance* self);
static BOOL lld_usart_writeCstr(const tch_usart_instance* self,const char* cstr,uint16_t* err);
static BOOL lld_usart_readCstr(const tch_usart_instance* self,char* cstr,uint32_t timeout,uint16_t* err);

__attribute__((always_inline)) static void lld_usart_monitorEvent(const tch_usart_instance* self,uint16_t evType);

static DMA_EVENTLISTENER(usart1_txdma_listener);
static DMA_EVENTLISTENER(usart2_txdma_listener);
static DMA_EVENTLISTENER(usart3_txdma_listener);

static DMA_EVENTLISTENER(usart1_rxdma_listener);
static DMA_EVENTLISTENER(usart2_rxdma_listener);
static DMA_EVENTLISTENER(usart3_rxdma_listener);

__attribute__((always_inline)) static inline BOOL __handle_txdma_event(tch_usart_prototype* ins,uint16_t evtype);
__attribute__((always_inline)) static inline BOOL __handle_rxdma_event(tch_usart_prototype* ins,uint16_t evtype);
__attribute__((always_inline)) static inline BOOL __handle_usart_event(tch_usart_prototype* ins,usart_hw_descriptor* uhw);

__attribute__((section(".data"))) static tch_dma_eventListener USART_TXDMA_Handlers[] = {
		usart1_txdma_listener,
		usart2_txdma_listener,
		usart3_txdma_listener
};

__attribute__((section(".data"))) static tch_dma_eventListener USART_RXDMA_Handlers[] = {
		usart1_rxdma_listener,
		usart2_rxdma_listener,
		usart3_rxdma_listener
};

/**
 *
 */
__attribute__((section(".data"))) static usart_hw_descriptor USART_HWs[] = {
		{
				USART1,
				0,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_USART1EN,
				RCC_APB2LPENR_USART1LPEN,
				USART1_IRQn
		},
		{
				USART2,
				0,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_USART2EN,
				RCC_APB1LPENR_USART2LPEN,
				USART2_IRQn
		},
		{
				USART3,
				0,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_USART3EN,
				RCC_APB1LPENR_USART3LPEN,
				USART3_IRQn
		}
};


/**
 */

__attribute__((section(".data"))) static tch_usart_prototype USART_StaticInstances[] = {
		{
				PUBLIC_INTERFACE,
				MTX_INIT,
				MTX_INIT,
				0,
				0,
				NULL,
				NULL,
				NULL,
				INIT_USART_EVQ
		},
		{
				PUBLIC_INTERFACE,
				MTX_INIT,
				MTX_INIT,
				0,
				1,
				NULL,
				NULL,
				NULL,
				INIT_USART_EVQ
		},
		{
				PUBLIC_INTERFACE,
				MTX_INIT,
				MTX_INIT,
				0,
				2,
				NULL,
				NULL,
				NULL,
				INIT_USART_EVQ
		}
};

static tch_usart_stream USART_TX_Stream[3];
static tch_usart_stream USART_RX_Stream[3];

const tch_usart_instance* usart1 = (tch_usart_instance*)&USART_StaticInstances[0];
const tch_usart_instance* usart2 = (tch_usart_instance*)&USART_StaticInstances[1];
const tch_usart_instance* usart3 = (tch_usart_instance*)&USART_StaticInstances[2];




BOOL lld_usart_open(const tch_usart_instance* self,uint32_t brate,tch_pwrMgrCfg pmopt,tch_usart_cfg* cfg){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	tch_bdc_usart* uhw_cfg = &USART_BD_CFGs[ins->idx];
	if(USART_IS_OPENED(ins)){                                                                  //// if usart is opened, return
		return FALSE;
	}
	tch_Mtx_lockt(&ins->wacc_lock,MTX_TRYMODE_WAIT);                                             //// otherwise try open it
	USART_SET_OPENED(ins);
	tch_Mtx_unlockt(&ins->wacc_lock);

	*uhw->_clkenr |= uhw->clkmsk;                                                              //// enable clock source

	if(pmopt == ActOnSleep){                                                                   //// active on sleep
		USART_SET_ACTIVE_INSLEEP(ins);                                                         //// enable lp clock and set relevant flag
		*uhw->_lpclkenr |= uhw->lpclkmsk;
	}

	USART_InitTypeDef uinit;
	USART_DeInit(uhw->_hw);
	USART_StructInit(&uinit);
	uinit.USART_BaudRate = brate;
	uinit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uinit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	uinit.USART_WordLength = USART_WordLength_8b;

	USART_Init(uhw->_hw,&uinit);
	uhw->_hw->CR1 |= (cfg->USART_Parity << USART_Parity_Pos_CR1) | USART_EVType_RXNE;
	uhw->_hw->CR2 |= (cfg->USART_StopBit << USART_StopBit_Pos_CR2);

	uhw->_hw->CR1 |= USART_CR1_UE;

	tch_gpio_cfg io_cfg;
	io_cfg.GPIO_AF = uhw_cfg->io_afv;
	io_cfg.GPIO_LP_Mode = pmopt == ActOnSleep? GPIO_LP_Mode_ON : GPIO_LP_Mode_OFF;
	io_cfg.GPIO_Mode = GPIO_Mode_AF;
	io_cfg.GPIO_OSpeed = GPIO_OSpeed_25M;
	io_cfg.GPIO_Otype = GPIO_Otype_PP;
	io_cfg.GPIO_PuPd = GPIO_PuPd_PD;

	ins->gpiov_handle = tch_lld_getGpio(uhw_cfg->port,(1 << uhw_cfg->tx_pin) | (1 << uhw_cfg->rx_pin),&io_cfg);

	tch_dma_cfg dma_cfg;
	if(uhw_cfg->tx_dma != DMA_NOT_USED){
		tch_lld_DMA_initCfg(&dma_cfg);
		dma_cfg.DMA_BufferMode = DMA_BufferMode_Normal;
		dma_cfg.DMA_Ch = uhw_cfg->tx_dma_ch;
		dma_cfg.DMA_Dir = DMA_Dir_MemToPeriph;
		dma_cfg.DMA_FlowControl = DMA_FlowControl_DMA;
		dma_cfg.DMA_MBurst = DMA_Burst_Single;
		dma_cfg.DMA_MDataAlign = DMA_DataAlign_Byte;
		dma_cfg.DMA_Minc = DMA_Minc_Enable;
		dma_cfg.DMA_PBurst = DMA_Burst_Single;
		dma_cfg.DMA_PDataAlign = DMA_DataAlign_Byte;
		dma_cfg.DMA_Pinc = DMA_Pinc_Disable;
		dma_cfg.DMA_Prior = DMA_Prior_Med;
		ins->tx_dma_handle = tch_lld_DMA_getInstance(uhw_cfg->tx_dma,&dma_cfg,pmopt);
		ins->tx_dma_handle->registerEventListener(ins->tx_dma_handle,USART_TXDMA_Handlers[ins->idx],DMA_FLAG_EvTC | DMA_FLAG_EvTE);
		uhw->_hw->CR3 |= USART_CR3_DMAT;
	}
	if(uhw_cfg->rx_dma != DMA_NOT_USED){
		dma_cfg.DMA_Ch = uhw_cfg->rx_dma_ch;
		dma_cfg.DMA_Dir = DMA_Dir_PeriphToMem;
		ins->rx_dma_handle = tch_lld_DMA_getInstance(uhw_cfg->rx_dma,&dma_cfg,pmopt);
		ins->rx_dma_handle->registerEventListener(ins->rx_dma_handle,USART_RXDMA_Handlers[ins->idx],DMA_FLAG_EvTC | DMA_FLAG_EvTE);
		uhw->_hw->CR3 |= USART_CR3_DMAR;
	}

	tch_iostreamBuffer_init(&USART_TX_Stream[ins->idx].iostr,USART_TX_Stream[ins->idx].iobuffer,USART_BUFFER_SIZE);
	USART_TX_Stream[ins->idx].ostr = tch_iostreamBuffer_openOutputStream(&USART_TX_Stream[ins->idx].iostr,ThreadUnsafe);
	USART_TX_Stream[ins->idx].istr = tch_iostreamBuffer_openInputStream(&USART_TX_Stream[ins->idx].iostr,ThreadUnsafe);

	tch_iostreamBuffer_init(&USART_RX_Stream[ins->idx].iostr,USART_RX_Stream[ins->idx].iobuffer,USART_BUFFER_SIZE);
	USART_RX_Stream[ins->idx].ostr = tch_iostreamBuffer_openOutputStream(&USART_RX_Stream[ins->idx].iostr,ThreadUnsafe);
	USART_RX_Stream[ins->idx].istr = tch_iostreamBuffer_openInputStream(&USART_RX_Stream[ins->idx].iostr,ThreadUnsafe);

	tch_genericQue_Init(&ins->evQue.rxneque);
	tch_genericQue_Init(&ins->evQue.tcque);
	tch_genericQue_Init(&ins->evQue.txdmaque);
	tch_genericQue_Init(&ins->evQue.rxdmaque);

	NVIC_EnableIRQ(uhw->irq);
	NVIC_SetPriority(uhw->irq,HANDLER_NORMAL_PRIOR);
	__DMB();
	__ISB();
	return TRUE;
}

BOOL lld_usart_close(const tch_usart_instance* self){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	tch_usart_stream* rx_str = &USART_RX_Stream[ins->idx];
	tch_bdc_usart* ucf = &USART_BD_CFGs[ins->idx];
	while(USART_IS_RXBUSY(ins) || USART_IS_TXBUSY(ins));                /// wait for complete on going communication
	tch_Mtx_lockt(&ins->wacc_lock,MTX_TRYMODE_WAIT);
	USART_CLR_OPENED(ins);
	tch_Mtx_unlockt(&ins->wacc_lock);
	NVIC_DisableIRQ(uhw->irq);
	__DMB();
	__ISB();
	if(ins->rx_dma_handle != NULL){
		ins->rx_dma_handle->close(ins->rx_dma_handle);
	}
	if(ins->tx_dma_handle != NULL){
		ins->tx_dma_handle->close(ins->tx_dma_handle);
	}
	tch_iostreamBuffer_close(&rx_str->iostr);
	ins->gpiov_handle->free(ins->gpiov_handle,(1 << ucf->rx_pin | 1 << ucf->tx_pin));
	while(ins->evQue.rxdmaque.entry){
		tchThread_wake(&ins->evQue.rxdmaque);
	}
	while(ins->evQue.rxneque.entry){
		tchThread_wake(&ins->evQue.rxneque);
	}
	while(ins->evQue.tcque.entry){
		tchThread_wake(&ins->evQue.tcque);
	}
	while(ins->evQue.txdmaque.entry){
		tchThread_wake(&ins->evQue.txdmaque);
	}
	return TRUE;
}


BOOL lld_usart_putc(const tch_usart_instance* self,uint8_t c){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	if(!USART_IS_OPENED(ins)){
		return FALSE;
	}
	if(USART_IS_TXBUSY(ins)){
		lld_usart_monitorEvent(self,USART_EVType_TCDMA);
		if(!USART_IS_OPENED(ins)){
			return FALSE;
		}
	}
	tch_Mtx_lockt(&ins->wacc_lock,MTX_TRYMODE_WAIT);
	if(!USART_IS_OPENED(ins)){
		tch_Mtx_unlockt(&ins->wacc_lock);
		return FALSE;
	}
	USART_SET_TXBUSY(ins);
	tch_Mtx_unlockt(&ins->wacc_lock);
	if(ins->tx_dma_handle != NULL){
		uhw->_hw->CR1 &= ~USART_EVType_TC;                                                                                            /// disable Transmit complete interrupt
		uint8_t* tbp = USART_TX_Stream[ins->idx].iobuffer;
		*tbp = c;
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Mem0,(uint32_t)tbp);
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Periph,(uint32_t)&uhw->_hw->DR);
		uhw->_hw->SR &= ~USART_SR_TC;
		while(!ins->tx_dma_handle->beginXfer(ins->tx_dma_handle,1)){
			lld_usart_monitorEvent(self,USART_EVType_TCDMA);
			if(!USART_IS_OPENED(ins)){
				USART_CLR_TXBUSY(ins);
				return FALSE;
			}
		}
		while(USART_IS_TXBUSY(ins)){
			tchThread_sleep(1);
			if(!USART_IS_OPENED(ins)){
				USART_CLR_TXBUSY(ins);
				return FALSE;
			}
		}
		uhw->_hw->CR1 |= USART_EVType_TC;
		if(USART_GET_TXERR(ins)){
			USART_CLR_TXERR(ins);
			return FALSE;
		}
		return TRUE;
	}else{
		while(!(uhw->_hw->SR & USART_SR_TXE));
		uhw->_hw->DR = c;
		if(!(uhw->_hw->SR & USART_SR_TC)){
			lld_usart_monitorEvent(self,USART_EVType_TC);
			if(!USART_IS_OPENED(ins)){
				USART_CLR_TXBUSY(ins);
				return FALSE;
			}
		}
		if(USART_GET_TXERR(ins)){
			USART_CLR_TXERR(ins);
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;

}

uint8_t lld_usart_getc(const tch_usart_instance* self){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	tch_istream* istr = USART_RX_Stream[ins->idx].istr;
	if(!USART_IS_OPENED(ins)){
		return 0;
	}
	uint8_t rb = 0;                                                                          // clr rx busy*/
	istr->getc(istr,&rb,NULL);
	return rb;
}

BOOL lld_usart_write(const tch_usart_instance* self,const uint8_t* bp,uint32_t size,uint16_t* err){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	if(!USART_IS_OPENED(ins) || !size){
		return FALSE;
	}
	if(USART_IS_TXBUSY(ins)){
		lld_usart_monitorEvent(self,USART_EVType_TCDMA);
		if(!USART_IS_OPENED(ins)){
			return FALSE;
		}
	}                                                                                     /// check whether tx busy,if it is thread will be put on wait state and wake up by hw interrupt
	tch_Mtx_lockt(&ins->wacc_lock,MTX_TRYMODE_WAIT);                                       /// otherwise tx busy flag is set
	if(!USART_IS_OPENED(ins)){
		tch_Mtx_unlockt(&ins->wacc_lock);
		return FALSE;
	}
	USART_SET_TXBUSY(ins);
	tch_Mtx_unlockt(&ins->wacc_lock);
	if(ins->tx_dma_handle != NULL){                                                                                                   /// tx dma handle is available
		uhw->_hw->CR1 &= ~USART_EVType_TC;                                                                                            /// disable Transmit complete interrupt
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Mem0,(uint32_t)bp);                                       /// set address of memory source
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Periph,(uint32_t)&uhw->_hw->DR);                          /// set address of peripheral target
		uhw->_hw->SR &= ~USART_SR_TC;                                                                                                 /// clear tc bit in the Status register
		while(!ins->tx_dma_handle->beginXfer(ins->tx_dma_handle,size)){
			lld_usart_monitorEvent(self,USART_EVType_TCDMA);
			if(!USART_IS_OPENED(ins)){
				USART_CLR_TXBUSY(ins);
				return FALSE;
			}
		}              /// begin dma transfer
		while(USART_IS_TXBUSY(ins)){
			tchThread_sleep(0);
			if(!USART_IS_OPENED(ins)){
				USART_CLR_TXBUSY(ins);
				return FALSE;
			}
		}
		uhw->_hw->CR1 |= USART_EVType_TC;
		*err = USART_GET_TXERR(ins);
		USART_CLR_TXERR(ins);
		if(*err){
			return FALSE;
		}
		return TRUE;
	}else{
		uint32_t idx = 0;
		while(idx < size){
			while(!(uhw->_hw->SR & USART_SR_TXE));
			uhw->_hw->DR = bp[idx++];
			if(!(uhw->_hw->SR & USART_SR_TC)){
				lld_usart_monitorEvent(self,USART_EVType_TC);
				if(!USART_IS_OPENED(ins)){
					return FALSE;
				}
			}
		}
		*err = USART_GET_TXERR(ins);
		USART_CLR_TXERR(ins);
		USART_CLR_TXBUSY(ins);
		if(*err){
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}


BOOL lld_usart_read(const tch_usart_instance* self,uint8_t* bp,uint32_t size,uint32_t timeout,uint16_t* err){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	tch_istream* istr = USART_RX_Stream[ins->idx].istr;
	return istr->read(istr,bp,size,NULL);
}

uint32_t lld_usart_available(const tch_usart_instance* self){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	tch_istream* istr = USART_RX_Stream[ins->idx].istr;
	return istr->available(istr);
}


BOOL lld_usart_writeCstr(const tch_usart_instance* self,const char* cstr,uint16_t* err){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	uint32_t size = 0;
	uint8_t* mp = USART_TX_Stream[ins->idx].iobuffer;
	if(USART_IS_TXBUSY(ins)){
		lld_usart_monitorEvent(self,USART_EVType_TCDMA);
		if(!USART_IS_OPENED(ins)){
			return FALSE;
		}
	}
	tch_Mtx_lockt(&ins->wacc_lock,MTX_TRYMODE_WAIT);
	if(!USART_IS_OPENED(ins)){
		tch_Mtx_unlockt(&ins->wacc_lock);
		return FALSE;
	}
	USART_SET_TXBUSY(ins);
	tch_Mtx_unlockt(&ins->wacc_lock);
	while(*(cstr + size) != '\0'){*(mp + size) = *(cstr + size++);}
	*(mp + size++) = '\r';
	if(ins->tx_dma_handle){
		uhw->_hw->CR1 &= ~USART_EVType_TC;
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Mem0,(uint32_t)mp);
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Periph,(uint32_t)&uhw->_hw->DR);
		uhw->_hw->SR &= ~USART_SR_TC;
		while(!ins->tx_dma_handle->beginXfer(ins->tx_dma_handle,size)){
			lld_usart_monitorEvent(self,USART_EVType_TCDMA);
			if(!USART_IS_OPENED(ins)){
				USART_CLR_TXBUSY(ins);
				return FALSE;
			}
		}
		while(USART_IS_TXBUSY(ins)){
			tchThread_sleep(1);
			if(!USART_IS_OPENED(ins)){
				USART_CLR_TXBUSY(ins);
				return FALSE;
			}
		}
		uhw->_hw->CR1 |= USART_EVType_TC;
		*err = USART_GET_TXERR(ins);
		USART_CLR_TXERR(ins);
		if(*err){
			return FALSE;
		}
		return TRUE;
	}else{
		uint32_t idx = 0;
		while(idx < size){
			while(!(uhw->_hw->SR & USART_SR_TXE));
			uhw->_hw->DR = mp[idx++];
			if(!(uhw->_hw->SR & USART_SR_TC)){
				lld_usart_monitorEvent(self,USART_EVType_TC);
				if(!USART_IS_OPENED(ins)){
					return FALSE;
				}
			}
		}
		*err = USART_GET_TXERR(ins);
		USART_CLR_TXERR(ins);
		USART_CLR_TXBUSY(ins);
		if(*err){
			return FALSE;
		}
		return TRUE;
	}
}

BOOL lld_usart_readCstr(const tch_usart_instance* self,char* cstr,uint32_t timeout,uint16_t* err){

}

void inline lld_usart_monitorEvent(const tch_usart_instance* self,uint16_t evType){
	tch_usart_prototype* ins = (tch_usart_prototype*)self;
	switch(evType){
	case USART_EVType_RXNE:
		tchThread_wait(&ins->evQue.rxneque);
		break;
	case USART_EVType_TC:
		tchThread_wait(&ins->evQue.tcque);
		break;
	case USART_EVType_TCDMA:
		tchThread_wait(&ins->evQue.txdmaque);
		break;
	}
	/// check usart is closed.
}

inline BOOL __handle_txdma_event(tch_usart_prototype* ins,uint16_t evtype){
	switch(evtype){
	case DMA_FLAG_EvTC:
		if(ins->evQue.txdmaque.entry){
			tchThread_wake(&ins->evQue.txdmaque);
		}
		USART_CLR_TXBUSY(ins);
		return TRUE;
	case DMA_FLAG_EvTE:
		USART_SET_TXERR(ins,USART_FLAG_ERR_DMA);
		return TRUE;
	case DMA_FLAG_EvFE:
		USART_SET_TXERR(ins,USART_FLAG_ERR_DMA);
		return TRUE;
	case DMA_FLAG_EvDME:
		USART_SET_TXERR(ins,USART_FLAG_ERR_DMA);
		return TRUE;
	}
	return FALSE;
}


inline BOOL __handle_rxdma_event(tch_usart_prototype* ins,uint16_t evtype){
	switch(evtype){
	case DMA_FLAG_EvTC:
		if(ins->evQue.rxdmaque.entry){
			tchThread_wake(&ins->evQue.rxdmaque);
		}
		return TRUE;
	case DMA_FLAG_EvTE:
		return TRUE;
	case DMA_FLAG_EvFE:
		return TRUE;
	case DMA_FLAG_EvDME:
		return TRUE;
	}
}

DMA_EVENTLISTENER(usart1_txdma_listener){
	tch_usart_prototype* uins = &USART_StaticInstances[0];
	return __handle_txdma_event(uins,evtype);
}

DMA_EVENTLISTENER(usart2_txdma_listener){
	tch_usart_prototype* uins = &USART_StaticInstances[1];
	return __handle_txdma_event(uins,evtype);
}

DMA_EVENTLISTENER(usart3_txdma_listener){
	tch_usart_prototype* uins = &USART_StaticInstances[2];
 	return __handle_txdma_event(uins,evtype);
}



DMA_EVENTLISTENER(usart1_rxdma_listener){

}

DMA_EVENTLISTENER(usart2_rxdma_listener){

}

DMA_EVENTLISTENER(usart3_rxdma_listener){

}

static inline BOOL __handle_usart_event(tch_usart_prototype* ins,usart_hw_descriptor* uhw){
	if(uhw->_hw->SR & USART_SR_RXNE){
		tch_ostream* ostr = USART_RX_Stream[ins->idx].ostr;
		ostr->putc(ostr,uhw->_hw->DR,NULL);
		if(ins->evQue.rxneque.entry){
			tchThread_wake(&ins->evQue.rxneque);
		}
		uhw->_hw->SR &= ~USART_SR_RXNE;
		return TRUE;
	}
	if(uhw->_hw->SR & USART_SR_TC){
		if(ins->evQue.tcque.entry){
			tchThread_wake(&ins->evQue.tcque);
		}
		uhw->_hw->SR &= ~USART_SR_TC;
		return TRUE;
	}
	if(uhw->_hw->SR & USART_SR_FE){
		uhw->_hw->SR &= ~USART_SR_FE;
		return TRUE;
	}
	if(uhw->_hw->SR & USART_SR_ORE){
		uhw->_hw->SR &= ~USART_SR_ORE;
		return TRUE;
	}
	return  FALSE;
}

void USART1_IRQHandler(void){
	tch_usart_prototype* ins = &USART_StaticInstances[0];
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	__handle_usart_event(ins,uhw);
}

void USART2_IRQHandler(void){
	tch_usart_prototype* ins = &USART_StaticInstances[1];
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	__handle_usart_event(ins,uhw);
}

void USART3_IRQHandler(void){
	tch_usart_prototype* ins = &USART_StaticInstances[2];
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	__handle_usart_event(ins,uhw);
}
