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


#ifdef FEATURE_MTHREAD
#define USART_RXLOCK(ins_p)   {tch_Mtx_lockt(&ins_p->racc_lock,TRUE);}
#define USART_TXLOCK(ins_p)   {tch_Mtx_lockt(&ins_p->wacc_lock,TRUE);}
#define USART_RXUNLOCK(ins_p) {tch_Mtx_unlockt(&ins_p->racc_lock);}
#define USART_TXUNLOCK(ins_p) {tch_Mtx_unlockt(&ins_p->wacc_lock);}


#else
#define USART_RXLOCK(ins_p)    {}
#define USART_TXLOCK(ins_p)    {}
#define USART_RXUNLOCK(ins_p)  {}
#define USART_TXUNLOCK(ins_p)  {}
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
	                                           lld_usart_readCstr,\
	                                           lld_usart_openInputStream,\
	                                           lld_usart_openOutputStream\
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
	tch_mtx_lock                       wacc_lock;
	tch_mtx_lock                       racc_lock;
	uint16_t                       flag;
	uint8_t                        idx;
	tch_dma_instance*              tx_dma_handle;
	tch_dma_instance*              rx_dma_handle;
	tch_gpio_instance*             gpiov_handle;
	tch_usart_evQue                evQue;
	tch_istream                    istr_wrapper;
	tch_ostream                    ostr_wrapper;
}__attribute__((packed));




static BOOL lld_usart_open(const tch_usart_instance* self,uint32_t brate,tch_pwrMgrCfg pmopt,tch_usart_cfg* cfg);
static BOOL lld_usart_close(const tch_usart_instance* self);
static BOOL lld_usart_putc(const tch_usart_instance* self,uint8_t c);
static uint8_t lld_usart_getc(const tch_usart_instance* self);
static BOOL lld_usart_write(const tch_usart_instance* self,const uint8_t* bp,uint32_t size,uint16_t* err);
static BOOL lld_usart_read(const tch_usart_instance* self,uint8_t* bp,uint32_t size,uint32_t timeout,uint16_t* err);
uint32_t lld_usart_available(const tch_usart_instance* self);
static BOOL lld_usart_writeCstr(const tch_usart_instance* self,const char* cstr,uint16_t* err);
static BOOL lld_usart_readCstr(const tch_usart_instance* self,char* cstr,uint32_t timeout,uint16_t* err);
static tch_istream* lld_usart_openInputStream(const tch_usart_instance* self);
static tch_ostream* lld_usart_openOutputStream(const tch_usart_instance* self);

/***
 *  Output Stream Wrapper
 */
static BOOL lld_usart_ostr_wrapper_write(tch_ostream* stream,const uint8_t* wb,uint32_t size,uint8_t* err);
static BOOL lld_usart_ostr_wrapper_putc(tch_ostream* stream,const uint8_t c,uint8_t* err);
static uint32_t lld_usart_ostr_wrapper_available(tch_ostream* stream);
static void lld_usart_ostr_wrapper_close(tch_ostream* stream);


/***
 *  Input stream Wrapper
 */
static BOOL lld_usart_istr_wrapper_read(tch_istream* stream,uint8_t* rb,uint32_t size,uint8_t* err);
static BOOL lld_usart_istr_wrapper_getc(tch_istream* stream,uint8_t* rb,uint8_t* err);
static uint32_t lld_usart_istr_wrapper_available(tch_istream* stream);
static void lld_usart_istr_wrapper_close(tch_istream* stream);



__attribute__((always_inline)) static BOOL lld_usart_monitorEvent(const tch_usart_instance* self,uint16_t evType);

static DMA_EVENTLISTENER(usart1_txdma_listener);
static DMA_EVENTLISTENER(usart2_txdma_listener);

#ifndef STM32F401x
static DMA_EVENTLISTENER(usart3_txdma_listener);
#endif

static DMA_EVENTLISTENER(usart1_rxdma_listener);
static DMA_EVENTLISTENER(usart2_rxdma_listener);
#ifndef STM32F401x
static DMA_EVENTLISTENER(usart3_rxdma_listener);
#endif

__attribute__((always_inline)) static inline BOOL __handle_txdma_event(tch_usart_prototype* ins,uint16_t evtype);
__attribute__((always_inline)) static inline BOOL __handle_rxdma_event(tch_usart_prototype* ins,uint16_t evtype);
__attribute__((always_inline)) static inline BOOL __handle_usart_event(tch_usart_prototype* ins,usart_hw_descriptor* uhw);

__attribute__((section(".data"))) static tch_dma_eventListener USART_TXDMA_Handlers[] = {
		usart1_txdma_listener
		,usart2_txdma_listener
#ifndef STM32F401x
		,usart3_txdma_listener
#endif
};

__attribute__((section(".data"))) static tch_dma_eventListener USART_RXDMA_Handlers[] = {
		usart1_rxdma_listener
		,usart2_rxdma_listener
#ifndef STM32F401x
		,usart3_rxdma_listener
#endif
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
				GPIO_AF_USART1,
				USART1_IRQn
		}
		,{
				USART2,
				0,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_USART2EN,
				RCC_APB1LPENR_USART2LPEN,
				GPIO_AF_USART2,
				USART2_IRQn
		}
#ifndef STM32F401x
		,{
				USART3,
				0,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_USART3EN,
				RCC_APB1LPENR_USART3LPEN,
				GPIO_AF_USART3,
				USART3_IRQn
		}
#else
		,{
				USART6,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_USART6EN,
				RCC_APB1LPENR_USART6LPEN,
				GPIO_AF_USART6,
				USART6_IRQn
		}
#endif
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
				INIT_USART_EVQ,
				ISTREAM_INIT,
				OSTREAM_INIT
		}
		,{
				PUBLIC_INTERFACE,
				MTX_INIT,
				MTX_INIT,
				0,
				1,
				NULL,
				NULL,
				NULL,
				INIT_USART_EVQ,
				ISTREAM_INIT,
				OSTREAM_INIT
		}
		,{
				PUBLIC_INTERFACE,
				MTX_INIT,
				MTX_INIT,
				0,
				2,
				NULL,
				NULL,
				NULL,
				INIT_USART_EVQ,
				ISTREAM_INIT,
				OSTREAM_INIT
		}
};
/***
 * static BOOL lld_usart_ostr_wrapper_write(tch_ostream* stream,const uint8_t* wb,uint32_t size,uint8_t* err);
 * static BOOL lld_usart_ostr_wrapper_putc(tch_ostream* stream,const uint8_t c,uint8_t* err);
 * static uint32_t lld_usart_ostr_wrapper_available(tch_ostream* stream);
 * static void lld_usart_ostr_wrapper_close(tch_ostream* stream);
 * static BOOL lld_usart_istr_wrapper_read(tch_istream* stream,uint8_t* rb,uint32_t size,uint8_t* err);
 * static BOOL lld_usart_istr_wrapper_getc(tch_istream* stream,uint8_t* rb,uint8_t* err);
 * static uint32_t lld_usart_istr_wrapper_available(tch_istream* stream);
 * static void lld_usart_istr_wrapper_close(tch_istream* stream);
 * */


static tch_usart_stream USART_TX_Stream[3];
static tch_usart_stream USART_RX_Stream[3];

const tch_usart_instance* usart1 = (tch_usart_instance*)&USART_StaticInstances[0];
const tch_usart_instance* usart2 = (tch_usart_instance*)&USART_StaticInstances[1];
const tch_usart_instance* usart3 = (tch_usart_instance*)&USART_StaticInstances[2];


void tch_lld_usart_initCfg(tch_usart_cfg* cfg){
	cfg->USART_Parity = USART_Parity_No;
	cfg->USART_StopBit = USART_StopBit_0dot5B;
}


BOOL lld_usart_open(const tch_usart_instance* self,uint32_t brate,tch_pwrMgrCfg pmopt,tch_usart_cfg* cfg){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	tch_bdc_usart* uhw_cfg = &USART_BD_CFGs[ins->idx];
	if(USART_IS_OPENED(ins)){                                                                  //// if usart is opened, return
		return FALSE;
	}
	USART_TXLOCK(ins);
	USART_SET_OPENED(ins);
	USART_TXUNLOCK(ins);

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
	io_cfg.GPIO_AF = uhw->io_afv;
	io_cfg.GPIO_Mode = GPIO_Mode_AF;
	io_cfg.GPIO_OSpeed = GPIO_OSpeed_25M;
	io_cfg.GPIO_Otype = GPIO_Otype_PP;
	io_cfg.GPIO_PuPd = GPIO_PuPd_PD;

	ins->gpiov_handle = tch_lld_gpio_init(uhw_cfg->port,(1 << uhw_cfg->tx_pin) | (1 << uhw_cfg->rx_pin),&io_cfg,pmopt);

	tch_dma_cfg dma_cfg;
	if(uhw_cfg->tx_dma != DMA_NOT_USED){
		tch_lld_dma_cfginit(&dma_cfg);
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
		ins->tx_dma_handle = tch_lld_dma_init(uhw_cfg->tx_dma,&dma_cfg,pmopt);
		ins->tx_dma_handle->registerEventListener(ins->tx_dma_handle,USART_TXDMA_Handlers[ins->idx],DMA_FLAG_EvTC | DMA_FLAG_EvTE);
		uhw->_hw->CR3 |= USART_CR3_DMAT;
	}
	if(uhw_cfg->rx_dma != DMA_NOT_USED){
		dma_cfg.DMA_Ch = uhw_cfg->rx_dma_ch;
		dma_cfg.DMA_Dir = DMA_Dir_PeriphToMem;
		ins->rx_dma_handle = tch_lld_dma_init(uhw_cfg->rx_dma,&dma_cfg,pmopt);
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
	USART_TXLOCK(ins);
	USART_CLR_OPENED(ins);
	USART_TXUNLOCK(ins);
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
	USART_TXLOCK(ins);
	if(!USART_IS_OPENED(ins)){
		USART_TXUNLOCK(ins);
		return FALSE;
	}
	USART_SET_TXBUSY(ins);
	if(ins->tx_dma_handle != NULL){
		uhw->_hw->CR1 &= ~USART_EVType_TC;                                                                                            /// disable Transmit complete interrupt
		uint8_t* tbp = USART_TX_Stream[ins->idx].iobuffer;
		*tbp = c;
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Mem0,(uint32_t)tbp);
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Periph,(uint32_t)&uhw->_hw->DR);
		uhw->_hw->SR &= ~USART_SR_TC;
		while(!ins->tx_dma_handle->beginXfer(ins->tx_dma_handle,1)){
			tchThread_sleep(0);
		}
		while(USART_IS_TXBUSY(ins)){
			tchThread_sleep(0);
		}
		uhw->_hw->CR1 |= USART_EVType_TC;
		if(USART_GET_TXERR(ins)){
			USART_CLR_TXERR(ins);
			USART_TXUNLOCK(ins);
			return FALSE;
		}
		USART_TXUNLOCK(ins);
		return TRUE;
	}else{
		while(!(uhw->_hw->SR & USART_SR_TXE)){
			tchThread_sleep(0);
		}
		uhw->_hw->DR = c;
		while(!(uhw->_hw->SR & USART_SR_TC)){
			tchThread_sleep(0);
		}
		if(USART_GET_TXERR(ins)){
			USART_CLR_TXERR(ins);
			USART_TXUNLOCK(ins);
			return FALSE;
		}
		USART_TXUNLOCK(ins);
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
	}                                                                               /// check whether tx busy,if it is thread will be put on wait state and wake up by hw interrupt

	USART_TXLOCK(ins);
	if(!USART_IS_OPENED(ins)){
		USART_TXUNLOCK(ins);
		return FALSE;
	}
	USART_SET_TXBUSY(ins);

	if(ins->tx_dma_handle){
		while(!uhw->_hw->SR & USART_SR_TC){
			tchThread_sleep(0);
		}
		uhw->_hw->CR1 &= ~USART_EVType_TC;                                                                                            /// disable Transmit complete interrupt
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Mem0,(uint32_t) bp);
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Periph,(uint32_t)&uhw->_hw->DR);
		uhw->_hw->SR &= ~USART_SR_TC;
		while(!ins->tx_dma_handle->beginXfer(ins->tx_dma_handle,size)){
			tchThread_sleep(0);
		}
		if(lld_usart_monitorEvent(self,USART_EVType_TCDMA)){
			*err = USART_GET_TXERR(ins);
			USART_CLR_TXERR(ins);
			uhw->_hw->CR1 |= USART_EVType_TC;
			if(*err){
				USART_TXUNLOCK(ins);
				return FALSE;
			}
			USART_TXUNLOCK(ins);
			return TRUE;
		}
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
		USART_TXUNLOCK(ins);
		if(*err){
			return FALSE;
		}
		return TRUE;
	}
	USART_TXUNLOCK(ins);
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
	if(!USART_IS_OPENED(ins)){
		return FALSE;
	}
	uint32_t size = 0;
	uint8_t mp[256];
	while(*(cstr + size) != '\0'){
		*(mp + size) = *(cstr + size++);
	}
	*(mp + size++) = '\r';
	USART_TXLOCK(ins);
	if(!USART_IS_OPENED(ins)){
		USART_TXUNLOCK(ins);
		return FALSE;
	}
	USART_SET_TXBUSY(ins);

	if(ins->tx_dma_handle){
		while(!uhw->_hw->SR & USART_SR_TC){
			tchThread_sleep(0);
		}
		uhw->_hw->CR1 &= ~USART_EVType_TC;                                                                                            /// disable Transmit complete interrupt
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Mem0,(uint32_t)mp);
		ins->tx_dma_handle->setAddress(ins->tx_dma_handle,DMA_TargetAddress_Periph,(uint32_t)&uhw->_hw->DR);
		uhw->_hw->SR &= ~USART_SR_TC;
		while(!ins->tx_dma_handle->beginXfer(ins->tx_dma_handle,size)){
			tchThread_sleep(0);
		}
		if(lld_usart_monitorEvent(self,USART_EVType_TCDMA)){
			*err = USART_GET_TXERR(ins);
			USART_CLR_TXERR(ins);
			uhw->_hw->CR1 |= USART_EVType_TC;
			if(*err){
				USART_TXUNLOCK(ins);
				return FALSE;
			}
			USART_TXUNLOCK(ins);
			return TRUE;
		}
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
		USART_TXUNLOCK(ins);
		if(*err){
			return FALSE;
		}
		return TRUE;
	}
	USART_TXUNLOCK(ins);
	return FALSE;
}

BOOL lld_usart_readCstr(const tch_usart_instance* self,char* cstr,uint32_t timeout,uint16_t* err){
	return TRUE;
}

tch_istream* lld_usart_openInputStream(const tch_usart_instance* self){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	ins->istr_wrapper._bp = ins;
	ins->istr_wrapper.close = lld_usart_istr_wrapper_close;
	ins->istr_wrapper.getc = lld_usart_istr_wrapper_getc;
	ins->istr_wrapper.read = lld_usart_istr_wrapper_read;
	ins->istr_wrapper.available = lld_usart_istr_wrapper_available;
	return &ins->istr_wrapper;
}

tch_ostream* lld_usart_openOutputStream(const tch_usart_instance* self){
	tch_usart_prototype* ins = (tch_usart_prototype*) self;
	ins->ostr_wrapper._bp = ins;
	ins->ostr_wrapper.close = lld_usart_ostr_wrapper_close;
	ins->ostr_wrapper.available = lld_usart_ostr_wrapper_available;
	ins->ostr_wrapper.write = lld_usart_ostr_wrapper_write;
	ins->ostr_wrapper.putc = lld_usart_ostr_wrapper_putc;
	return &ins->ostr_wrapper;
}


/***
 *  Output Stream Wrapper
 */
BOOL lld_usart_ostr_wrapper_write(tch_ostream* stream,const uint8_t* wb,uint32_t size,uint8_t* err){
	tch_usart_prototype* ins = (tch_usart_prototype*)stream->_bp;
	if(ins == NULL){
		return FALSE;
	}
	return lld_usart_write((const tch_usart_instance*)ins,wb,size,(uint16_t*)err);
}

BOOL lld_usart_ostr_wrapper_putc(tch_ostream* stream,const uint8_t c,uint8_t* err){
	tch_usart_prototype* ins = (tch_usart_prototype*)stream->_bp;
	if(ins == NULL){
		return FALSE;
	}
	return lld_usart_putc((const tch_usart_instance*)ins,c);
}


uint32_t lld_usart_ostr_wrapper_available(tch_ostream* stream){
	return 0xFFFFFFFF;
}

void lld_usart_ostr_wrapper_close(tch_ostream* stream){
	stream->_bp = NULL;
	__DMB();
	__ISB();
}


/***
 *  Input stream Wrapper
 */
BOOL lld_usart_istr_wrapper_read(tch_istream* stream,uint8_t* rb,uint32_t size,uint8_t* err){
	tch_usart_prototype* ins = (tch_usart_prototype*) stream->_bp;
	tch_istream* str = USART_RX_Stream[ins->idx].istr;
	return str->read(str,rb,size,err);
}

BOOL lld_usart_istr_wrapper_getc(tch_istream* stream,uint8_t* rb,uint8_t* err){
	tch_usart_prototype* ins = (tch_usart_prototype*) stream->_bp;
	tch_istream* str = USART_RX_Stream[ins->idx].istr;
	return str->getc(str,rb,err);
}

uint32_t lld_usart_istr_wrapper_available(tch_istream* stream){
	tch_usart_prototype* ins = (tch_usart_prototype*) stream->_bp;
	tch_istream* str = USART_RX_Stream[ins->idx].istr;
	return str->available(str);

}

void lld_usart_istr_wrapper_close(tch_istream* stream){
	stream->_bp = NULL;
	__DMB();
	__ISB();
}



BOOL inline lld_usart_monitorEvent(const tch_usart_instance* self,uint16_t evType){
	tch_usart_prototype* ins = (tch_usart_prototype*)self;
	if(!USART_IS_OPENED(ins)){
		return FALSE;
	}
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
	if(!USART_IS_OPENED(ins)){
		return FALSE;
	}
	return TRUE;
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
	return FALSE;
}

DMA_EVENTLISTENER(usart2_rxdma_listener){
	return FALSE;
}

DMA_EVENTLISTENER(usart3_rxdma_listener){
	return FALSE;
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
#ifndef STM32F401x
void USART3_IRQHandler(void){
	tch_usart_prototype* ins = &USART_StaticInstances[2];
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	__handle_usart_event(ins,uhw);
}
#else
void USART6_IRQHandler(void){
	tch_usart_prototype* ins = &USART_StaticInstances[2];
	usart_hw_descriptor* uhw = &USART_HWs[ins->idx];
	__handle_usart_event(ins,uhw);
}
#endif
