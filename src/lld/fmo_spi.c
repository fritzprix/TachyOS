/*
 * fmo_spi.c
 *
 *  Created on: 2014. 5. 8.
 *      Author: innocentevil
 */

#include "fmo_spi.h"
#include "fmo_dma.h"
#include "fmo_gpio.h"

#include "../core/fmo_lldabs.h"
#include "hw_descriptor_types.h"
#include "bl_config.h"


#ifndef SPI_RXBUFFER_SIZE
#define SPI_RXBUFFER_SIZE 256
#endif

#define _PUBLIC_IX_LIST                   {\
	                                        tch_lld_spi_open,\
	                                        tch_lld_spi_transceive,\
	                                        tch_lld_spi_transceiveBurst,\
	                                        tch_lld_spi_read,\
	                                        tch_lld_spi_write,\
	                                        tch_lld_spi_registerSlaveEventListener,\
	                                        tch_lld_spi_unregisterSlaveEventListener,\
	                                        tch_lld_spi_close}


#define SPI_Baudrate_Pos_CR1              ((uint8_t) 3)
#define SPI_Baudrate_Msk                  (7 << SPI_Baudrate_Pos_CR1)

#define SPI_ClockMode_Pos_CR1             ((uint8_t) 0)
#define SPI_ClockMode_Msk                 (3 << SPI_ClockMode_Pos_CR1)

#define SPI_DataFormat_Pos_CR1            ((uint8_t) 11)
#define SPI_DataFormat_Msk                (1 << SPI_DataFormat_Pos_CR1)

#define SPI_DataOrientation_Pos_CR1       ((uint8_t) 7)
#define SPI_DataOrientation_Msk           (1 << SPI_DataOrientation_Pos_CR1)

#define SPI_OPMode_Pos_CR1                ((uint8_t) 2)
#define SPI_OPMode_Msk                    (1 << SPI_OPMode_Pos_CR1)

#ifdef FEATURE_MTHREAD
#define SPI_LOCK(ins_p){\
	tch_Mtx_lockt(&ins_p->lock,MTX_TRYMODE_WAIT);\
}
#define SPI_UNLOCK(ins_p){\
	tch_Mtx_unlockt(&ins_p->lock);\
}
#else
#define SPI_LOCK(ins_p){}
#define SPI_UNLOCK(ins_p){}
#endif


#define SPI_STATUS_OPEN                     ((uint16_t) 1 << 0)
#define SPI_STATUS_BUSY                     ((uint16_t) 1 << 1)
#define SPI_STATUS_MASTER                   ((uint16_t) 1 << 2)

#define SPI_SET_OPEN(ins_p)                 (ins_p->status |= SPI_STATUS_OPEN)
#define SPI_CLR_OPEN(ins_p)                 (ins_p->status &= ~SPI_STATUS_OPEN)
#define SPI_IS_OPEN(ins_p)                  (ins_p->status & SPI_STATUS_OPEN)

#define SPI_SET_BUSY(ins_p)                 (ins_p->status |= SPI_STATUS_BUSY)
#define SPI_CLR_BUSY(ins_p)                 (ins_p->status &= ~SPI_STATUS_BUSY)
#define SPI_IS_BUSY(ins_p)                  (ins_p->status & SPI_STATUS_BUSY)

#define SPI_SET_MASTER(ins_p)               (ins_p->status |= SPI_STATUS_MASTER)
#define SPI_CLR_MASTER(ins_p)               (ins_p->status &= ~SPI_STATUS_MASTER)
#define SPI_IS_MASTER(ins_p)                (ins_p->status & SPI_STATUS_MASTER)



typedef struct _tch_spi_prototype_t tch_spi_prototype;
typedef struct _tch_spi_evque_t tch_spi_evque;
typedef struct _tch_spi_rxbuffer_t tch_spi_rxbuffer;

struct _tch_spi_rxbuffer_t {
	tch_iostream_buffer    rx_streambuffer;
	uint8_t                rx_streambuffer_mem[SPI_RXBUFFER_SIZE];
};

typedef enum {
	TXC,RXC
} tch_spi_evtype;


struct _tch_spi_evque_t {
	tch_genericList_queue_t  txcqueue;
	tch_genericList_queue_t  rxcqueue;
};

struct _tch_spi_prototype_t {
	tch_spi_instance       pix;
	uint16_t               status;
	uint8_t                idx;
	tch_dma_instance*     _txdma_handle;
	tch_dma_instance*     _rxdma_handle;
	tch_gpio_instance*    _io_handle;
	tch_istream*           rx_buffer_istream;
	tch_ostream*           rx_buffer_ostream;
	tch_spi_eventlistener _listener;
#ifdef FEATURE_MTHREAD
	mtx_lock               lock;
#endif
	tch_spi_evque          evque;
} __attribute__((packed));



/**
 * public function
 */
static BOOL tch_lld_spi_open(const tch_spi_instance* self,tch_spi_cfg* cfg,tch_pwrMgrCfg pcfg);
static uint16_t tch_lld_spi_transceive(const tch_spi_instance* self,uint16_t data,uint16_t* err);
static BOOL tch_lld_spi_transceiveBurst(const tch_spi_instance* self,const void* wb,void* rb,uint32_t size,uint16_t* err);
static BOOL tch_lld_spi_read(const tch_spi_instance* self,void* rb,uint32_t size,uint16_t* err);
static BOOL tch_lld_spi_write(const tch_spi_instance* self,const void* wb,uint32_t size,uint16_t* err);
static BOOL tch_lld_spi_registerSlaveEventListener(const tch_spi_instance* self,tch_spi_eventlistener listener);
static BOOL tch_lld_spi_unregisterSlaveEventListener(const tch_spi_instance* self);
static BOOL tch_lld_spi_close(const tch_spi_instance* self);

/**
 * private function
 */

static inline BOOL tch_lld_spi_monitorEvent(const tch_spi_prototype* ins,tch_spi_evtype ev) __attribute__((always_inline));
static inline BOOL tch_lld_spi_handle_rxdma_event(tch_spi_prototype* ins) __attribute__((always_inline));

static DMA_EVENTLISTENER(tch_lld_spi1_rxdma_eventlistener);
static DMA_EVENTLISTENER(tch_lld_spi2_rxdma_eventlistener);
static DMA_EVENTLISTENER(tch_lld_spi3_rxdma_eventlistener);

static DMA_EVENTLISTENER(tch_lld_spi1_txdma_eventlistener);
static DMA_EVENTLISTENER(tch_lld_spi2_txdma_eventlistener);
static DMA_EVENTLISTENER(tch_lld_spi3_txdma_eventlistener);
/**
 */
__attribute__((section(".data"))) static spi_hw_descriptor SPI_HWs[] = {
		{
				SPI1,
				0,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_SPI1EN,
				RCC_APB2LPENR_SPI1LPEN,
				GPIO_AF_SPI1,
				SPI1_IRQn
		},
		{
				SPI2,
				0,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_SPI2EN,
				RCC_APB1LPENR_SPI2LPEN,
				GPIO_AF_SPI2,
				SPI2_IRQn
		},
		{
				SPI3,
				0,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_SPI3EN,
				RCC_APB1LPENR_SPI3LPEN,
				GPIO_AF_SPI3,
				SPI3_IRQn
		}
};
/**
 */
__attribute__((section(".data"))) static tch_spi_prototype SPI_StaticInstances[] = {
		{
				_PUBLIC_IX_LIST,
				0,
				0,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
#ifdef FEATURE_MTHREAD
				MTX_INIT,
#endif
				{GENERIC_LIST_QUEUE_INIT,GENERIC_LIST_QUEUE_INIT}
		},
		{
				_PUBLIC_IX_LIST,
				0,
				1,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
#ifdef FEATURE_MTHREAD
				MTX_INIT,
#endif
				{GENERIC_LIST_QUEUE_INIT,GENERIC_LIST_QUEUE_INIT}
		},
		{
				_PUBLIC_IX_LIST,
				0,
				2,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
#ifdef FEATURE_MTHREAD
				MTX_INIT,
#endif
				{GENERIC_LIST_QUEUE_INIT,GENERIC_LIST_QUEUE_INIT}
		}
};



__attribute__((section(".data"))) static tch_dma_eventListener SPI_TXDMA_Listeners[] = {
		tch_lld_spi1_txdma_eventlistener,
		tch_lld_spi2_txdma_eventlistener,
		tch_lld_spi3_txdma_eventlistener
};

__attribute__((section(".data"))) static tch_dma_eventListener SPI_RXDMA_Listeners[] = {
		tch_lld_spi1_rxdma_eventlistener,
		tch_lld_spi2_rxdma_eventlistener,
		tch_lld_spi3_rxdma_eventlistener
};

static tch_spi_rxbuffer SPI_RX_BUFFs[3];

const tch_spi_instance* spi1 = (const tch_spi_instance*) &SPI_StaticInstances[0];
const tch_spi_instance* spi2 = (const tch_spi_instance*) &SPI_StaticInstances[1];
const tch_spi_instance* spi3 = (const tch_spi_instance*) &SPI_StaticInstances[2];


void tch_lld_spi_cfginit(tch_spi_cfg* cfg){
	cfg->SPI_Baudrate = SPI_Baudrate_Mid;
	cfg->SPI_ClockMode = SPI_ClockMode_0;
	cfg->SPI_DataFormat = SPI_DataFormat_8B;
	cfg->SPI_DataOrientation = SPI_DataOrientation_MSBFIRST;
	cfg->SPI_OPMode = SPI_OPMode_Master;
}



BOOL tch_lld_spi_open(const tch_spi_instance* self,tch_spi_cfg* cfg,tch_pwrMgrCfg pcfg){
	tch_spi_prototype* ins = (tch_spi_prototype*) self;
	spi_hw_descriptor* shw = &SPI_HWs[ins->idx];
	tch_bdc_spi* spicfg = &SPI_BD_CFGs[ins->idx];

	if(SPI_IS_OPEN(ins)){
		return FALSE;
	}
	SPI_LOCK(ins);
	SPI_SET_OPEN(ins);
	SPI_UNLOCK(ins);

	*shw->_clkenr |= shw->clkmsk;
	if(pcfg == ActOnSleep){
		*shw->_lpclkenr |= shw->lpclkmsk;
	}

	/**
	 * gpio configuration
	 */

	tch_gpio_cfg iocfg;
	tch_lld_gpio_cfgInit(&iocfg);
	iocfg.GPIO_AF = shw->io_afv;
	iocfg.GPIO_Mode = GPIO_Mode_AF;
	iocfg.GPIO_OSpeed = GPIO_OSpeed_25M;
	iocfg.GPIO_Otype = GPIO_OType_PP;
	iocfg.GPIO_PuPd = GPIO_PuPd_NOPULL;

	if(cfg->SPI_OPMode == SPI_OPMode_Master){
		SPI_SET_MASTER(ins);
		ins->_io_handle = tch_lld_gpio_init(spicfg->spi_ioport,(1 << spicfg->mosi_pin) | (1 << spicfg->miso_pin) | (1 << spicfg->sclk_pin),&iocfg,pcfg);
	}else{
		SPI_CLR_MASTER(ins);
		ins->_io_handle = tch_lld_gpio_init(spicfg->spi_ioport,(1 << spicfg->mosi_pin) | (1 << spicfg->miso_pin) | (1 << spicfg->sclk_pin) | (1 << spicfg->cs_pin),&iocfg,pcfg);
	}
	if(!ins->_io_handle){
		return FALSE;
	}

	/**
	 * tx dma initialize
	 */
	tch_dma_cfg dmacfg;
	tch_lld_dma_cfginit(&dmacfg);
	dmacfg.DMA_BufferMode = DMA_BufferMode_Normal;
	dmacfg.DMA_Ch = spicfg->tx_dma_ch;
	dmacfg.DMA_Dir = DMA_Dir_MemToPeriph;
	dmacfg.DMA_MBurst = DMA_Burst_Single;
	dmacfg.DMA_Minc = DMA_Minc_Enable;
	dmacfg.DMA_PBurst = DMA_Burst_Single;
	dmacfg.DMA_Pinc = DMA_Pinc_Disable;
	if(cfg->SPI_DataFormat == SPI_DataFormat_8B){
		dmacfg.DMA_MDataAlign = DMA_DataAlign_Byte;
		dmacfg.DMA_PDataAlign = DMA_DataAlign_Byte;
	}else{
		dmacfg.DMA_MDataAlign = DMA_DataAlign_Hword;
		dmacfg.DMA_PDataAlign = DMA_DataAlign_Hword;
	}

	dmacfg.DMA_Prior = DMA_Prior_Med;
	ins->_txdma_handle = tch_lld_dma_init(spicfg->tx_dma,&dmacfg,pcfg);
	ins->_txdma_handle->registerEventListener(ins->_txdma_handle,SPI_TXDMA_Listeners[ins->idx],DMA_FLAG_EvTC);


	/**
	 * rx dma initialize
	 * - RX DMA is used only when spi acts as master
	 *   otherwise, everytime data recevied, event listener is invoked (if it has been registered)
	 *   and received data stored in receive buffer
	 *
	 */
	if(cfg->SPI_OPMode == SPI_OPMode_Master){
		dmacfg.DMA_Ch = spicfg->rx_dma_ch;
		dmacfg.DMA_Dir = DMA_Dir_PeriphToMem;
		dmacfg.DMA_Prior = DMA_Prior_Med;
		ins->_rxdma_handle = tch_lld_dma_init(spicfg->rx_dma,&dmacfg,pcfg);
		ins->_rxdma_handle->registerEventListener(ins->_rxdma_handle,SPI_RXDMA_Listeners[ins->idx],DMA_FLAG_EvTC);
		ins->_listener = NULL;
	}else{
		tch_iostreamBuffer_init(&SPI_RX_BUFFs[ins->idx].rx_streambuffer,SPI_RX_BUFFs[ins->idx].rx_streambuffer_mem,SPI_RXBUFFER_SIZE);
		ins->rx_buffer_istream = tch_iostreamBuffer_openInputStream(&SPI_RX_BUFFs[ins->idx].rx_streambuffer,ThreadSafe);
		ins->rx_buffer_ostream = tch_iostreamBuffer_openOutputStream(&SPI_RX_BUFFs[ins->idx].rx_streambuffer,ThreadSafe);
		NVIC_EnableIRQ(shw->irq);
		ins->_listener = NULL;
	}

	if(cfg->SPI_OPMode == SPI_OPMode_Master){
		shw->_hw->CR1 |= (SPI_CR1_SSM | SPI_CR1_SSI);
		shw->_hw->CR2 |= (SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
	}else{
		shw->_hw->CR2 |= (SPI_CR2_RXNEIE | SPI_CR2_TXDMAEN);
	}
	shw->_hw->CR1 |= ((cfg->SPI_Baudrate << SPI_Baudrate_Pos_CR1) | (cfg->SPI_ClockMode << SPI_ClockMode_Pos_CR1) | (cfg->SPI_DataFormat << SPI_DataFormat_Pos_CR1));
	shw->_hw->CR1 |= ((cfg->SPI_DataOrientation << SPI_DataOrientation_Pos_CR1) | (cfg->SPI_OPMode << SPI_OPMode_Pos_CR1));
	shw->_hw->CR1 |= SPI_CR1_SPE;

	return TRUE;

}

uint16_t tch_lld_spi_transceive(const tch_spi_instance* self,uint16_t data,uint16_t* err){
	tch_spi_prototype* ins = (tch_spi_prototype*) self;
	spi_hw_descriptor* shw = &SPI_HWs[ins->idx];
	uint16_t rtb = 0;
	if(!SPI_IS_OPEN(ins)){
		return 0;
	}
	if(SPI_IS_MASTER(ins)){
		while(SPI_IS_BUSY(ins)){
			tchThread_sleep(0);
		}
		SPI_LOCK(ins);
		SPI_SET_BUSY(ins);
		while(shw->_hw->SR & SPI_SR_BSY){
			tchThread_sleep(0);
		}

		/**
		 * configure rxdma
		 */
		ins->_rxdma_handle->setIncrementMode(ins->_rxdma_handle,DMA_TargetAddress_Mem0,TRUE);
		ins->_rxdma_handle->setAddress(ins->_rxdma_handle,DMA_TargetAddress_Mem0,(uint32_t) &rtb);
		ins->_rxdma_handle->setAddress(ins->_rxdma_handle,DMA_TargetAddress_Periph,(uint32_t) &shw->_hw->DR);
		ins->_rxdma_handle->beginXfer(ins->_rxdma_handle,1);
		/**
		 * configure txdma
		 */
		ins->_txdma_handle->setIncrementMode(ins->_txdma_handle,DMA_TargetAddress_Mem0,TRUE);
		ins->_txdma_handle->setAddress(ins->_txdma_handle,DMA_TargetAddress_Mem0,(uint32_t) &data);
		ins->_txdma_handle->setAddress(ins->_txdma_handle,DMA_TargetAddress_Periph,(uint32_t) &shw->_hw->DR);
		ins->_txdma_handle->beginXfer(ins->_txdma_handle,1);
		while(shw->_hw->SR & SPI_SR_BSY){
			tchThread_sleep(0);
		}
		SPI_CLR_BUSY(ins);
		SPI_UNLOCK(ins);
		return rtb;
	}else{

	}
	return 0;
}

BOOL tch_lld_spi_transceiveBurst(const tch_spi_instance* self,const void* wb,void* rb,uint32_t size,uint16_t* err){
	tch_spi_prototype* ins = (tch_spi_prototype*) self;
	spi_hw_descriptor* shw = &SPI_HWs[ins->idx];
	if(!SPI_IS_OPEN(ins)){
		return FALSE;
	}
	if(SPI_IS_MASTER(ins)){
		while(SPI_IS_BUSY(ins)){
			tchThread_sleep(0);
		}
		SPI_LOCK(ins);
		SPI_SET_BUSY(ins);
		while(shw->_hw->SR & SPI_SR_BSY){
			tchThread_sleep(0);
		}
		ins->_rxdma_handle->setIncrementMode(ins->_rxdma_handle,DMA_TargetAddress_Mem0,TRUE);
		ins->_rxdma_handle->setAddress(ins->_rxdma_handle,DMA_TargetAddress_Mem0,(uint32_t) rb);
		ins->_rxdma_handle->setAddress(ins->_rxdma_handle,DMA_TargetAddress_Periph,(uint32_t) &shw->_hw->DR);
		ins->_rxdma_handle->beginXfer(ins->_rxdma_handle,size);

		ins->_txdma_handle->setIncrementMode(ins->_txdma_handle,DMA_TargetAddress_Mem0,TRUE);
		ins->_txdma_handle->setAddress(ins->_txdma_handle,DMA_TargetAddress_Mem0,(uint32_t) wb);
		ins->_txdma_handle->setAddress(ins->_txdma_handle,DMA_TargetAddress_Periph,(uint32_t) &shw->_hw->DR);
		ins->_txdma_handle->beginXfer(ins->_txdma_handle,size);
		while(shw->_hw->SR & SPI_SR_BSY){
			if(!tch_lld_spi_monitorEvent(ins,RXC)){
				SPI_CLR_BUSY(ins);
				SPI_UNLOCK(ins);
				return FALSE;
			}
		}
		SPI_CLR_BUSY(ins);
		SPI_UNLOCK(ins);
		return TRUE;
	}else{

	}
	return FALSE;
}

BOOL tch_lld_spi_read(const tch_spi_instance* self,void* rb,uint32_t size,uint16_t* err){
	tch_spi_prototype* ins = (tch_spi_prototype*) self;
	spi_hw_descriptor* shw = &SPI_HWs[ins->idx];
	if(!SPI_IS_OPEN(ins)){
		return FALSE;
	}
	if(SPI_IS_MASTER(ins)){
		uint16_t txz = 0;
		while(SPI_IS_BUSY(ins)){
			tchThread_sleep(0);
		}
		SPI_LOCK(ins);
		SPI_SET_BUSY(ins);
		while(shw->_hw->SR & SPI_SR_BSY){
			tchThread_sleep(0);
		}
		ins->_rxdma_handle->setIncrementMode(ins->_rxdma_handle,DMA_TargetAddress_Mem0,TRUE);
		ins->_rxdma_handle->setAddress(ins->_rxdma_handle,DMA_TargetAddress_Mem0,(uint32_t) rb);
		ins->_rxdma_handle->setAddress(ins->_rxdma_handle,DMA_TargetAddress_Periph,(uint32_t) &shw->_hw->DR);
		ins->_rxdma_handle->beginXfer(ins->_rxdma_handle,size);

		ins->_txdma_handle->setIncrementMode(ins->_txdma_handle,DMA_TargetAddress_Mem0,FALSE);
		ins->_txdma_handle->setAddress(ins->_txdma_handle,DMA_TargetAddress_Mem0,(uint32_t) &txz);
		ins->_txdma_handle->setAddress(ins->_txdma_handle,DMA_TargetAddress_Periph,(uint32_t) &shw->_hw->DR);
		ins->_txdma_handle->beginXfer(ins->_txdma_handle,size);
		while(shw->_hw->SR & SPI_SR_BSY){
			if(!tch_lld_spi_monitorEvent(ins,RXC)){
				SPI_CLR_BUSY(ins);
				SPI_UNLOCK(ins);
				return FALSE;
			}
		}
		SPI_CLR_BUSY(ins);
		SPI_UNLOCK(ins);
		return TRUE;
	}else{

	}
	return FALSE;
}

BOOL tch_lld_spi_write(const tch_spi_instance* self,const void* wb,uint32_t size,uint16_t* err){
	tch_spi_prototype* ins = (tch_spi_prototype*) self;
	spi_hw_descriptor* shw = &SPI_HWs[ins->idx];
	if(!SPI_IS_OPEN(ins)){
		return FALSE;
	}
	if(SPI_IS_MASTER(ins)){
		while(SPI_IS_BUSY(ins)){
			tchThread_sleep(0);
		}
		SPI_LOCK(ins);
		SPI_SET_BUSY(ins);
		while(shw->_hw->SR & SPI_SR_BSY){
			tchThread_sleep(0);
		}
		ins->_rxdma_handle->setIncrementMode(ins->_rxdma_handle,DMA_TargetAddress_Mem0,FALSE);
		ins->_rxdma_handle->setAddress(ins->_rxdma_handle,DMA_TargetAddress_Mem0,(uint32_t) NULL);
		ins->_rxdma_handle->setAddress(ins->_rxdma_handle,DMA_TargetAddress_Periph,(uint32_t) &shw->_hw->DR);
		ins->_rxdma_handle->beginXfer(ins->_rxdma_handle,size);

		ins->_txdma_handle->setIncrementMode(ins->_txdma_handle,DMA_TargetAddress_Mem0,TRUE);
		ins->_txdma_handle->setAddress(ins->_txdma_handle,DMA_TargetAddress_Mem0,(uint32_t) wb);
		ins->_txdma_handle->setAddress(ins->_txdma_handle,DMA_TargetAddress_Periph,(uint32_t) &shw->_hw->DR);
		ins->_txdma_handle->beginXfer(ins->_txdma_handle,size);
		while(shw->_hw->SR & SPI_SR_BSY){
			if(!tch_lld_spi_monitorEvent(ins,RXC)){
				SPI_CLR_BUSY(ins);
				SPI_UNLOCK(ins);
				return FALSE;
			}
		}
		SPI_CLR_BUSY(ins);
		SPI_UNLOCK(ins);
		return TRUE;
	}else{

	}
	return FALSE;
}

BOOL tch_lld_spi_registerSlaveEventListener(const tch_spi_instance* self,tch_spi_eventlistener listener){
	tch_spi_prototype* ins = (tch_spi_prototype*) self;
	spi_hw_descriptor* shw = &SPI_HWs[ins->idx];
	if((!SPI_IS_OPEN(ins)) || SPI_IS_MASTER(ins)){
		return FALSE;
	}
	ins->_listener = listener;
}

BOOL tch_lld_spi_unregisterSlaveEventListener(const tch_spi_instance* self){
	tch_spi_prototype* ins = (tch_spi_prototype*) self;
	spi_hw_descriptor* shw = &SPI_HWs[ins->idx];
	if((!SPI_IS_OPEN(ins)) || SPI_IS_MASTER(ins)){
		return FALSE;

	}
	ins->_listener = NULL;
}

BOOL tch_lld_spi_close(const tch_spi_instance* self){
	tch_spi_prototype* ins = (tch_spi_prototype*) self;
	spi_hw_descriptor* shw = &SPI_HWs[ins->idx];
	tch_bdc_spi* cfg = &SPI_BD_CFGs[ins->idx];
	while(SPI_IS_BUSY(ins)){
		tchThread_sleep(0);
	}
	SPI_LOCK(ins);
	SPI_CLR_OPEN(ins);
	SPI_UNLOCK(ins);

	shw->_hw->CR1 = 0;
	shw->_hw->CR2 = 0;
	*shw->_clkenr &= ~shw->clkmsk;
	*shw->_lpclkenr &= ~shw->lpclkmsk;

	if(ins->_rxdma_handle){
		ins->_rxdma_handle->close(ins->_rxdma_handle);
	}
	if(ins->_txdma_handle){
		ins->_txdma_handle->close(ins->_txdma_handle);
	}

	if(ins->rx_buffer_istream){
		ins->rx_buffer_istream->close(ins->rx_buffer_istream);
	}
	if(ins->rx_buffer_ostream){
		ins->rx_buffer_ostream->close(ins->rx_buffer_ostream);
	}

	if(SPI_IS_MASTER(ins)){
		ins->_io_handle->free(ins->_io_handle,(1 <<  cfg->miso_pin) | (1 << cfg->mosi_pin) | (1 << cfg->sclk_pin));
	}else{
		ins->_io_handle->free(ins->_io_handle,(1 <<  cfg->miso_pin) | (1 << cfg->mosi_pin) | (1 << cfg->sclk_pin) | (1 << cfg->cs_pin));
	}

	if(ins->evque.rxcqueue.entry){
		tchThread_wake(&ins->evque.rxcqueue);
	}
	if(ins->evque.txcqueue.entry){
		tchThread_wake(&ins->evque.txcqueue);
	}

	SPI_CLR_MASTER(ins);
	SPI_CLR_OPEN(ins);
}


static inline BOOL tch_lld_spi_monitorEvent(const tch_spi_prototype* ins,tch_spi_evtype ev){
	if(!SPI_IS_OPEN(ins)){
		return FALSE;
	}
	switch(ev){
	case TXC:
		tchThread_wait(&ins->evque.txcqueue);
		break;
	case RXC:
		tchThread_wait(&ins->evque.rxcqueue);
		break;
	}
	if(!SPI_IS_OPEN(ins)){
		return FALSE;
	}
	return TRUE;
}



static inline BOOL tch_lld_spi_handle_rxdma_event(tch_spi_prototype* ins){
	if(SPI_IS_MASTER(ins)){
		if(ins->evque.rxcqueue.entry){
			tchThread_wake(&ins->evque.rxcqueue);
		}
		return TRUE;
	}else{

	}
	return FALSE;
}



void SPI1_IRQHandler(void){

}
void SPI2_IRQHandler(void){

}
void SPI3_IRQHandler(void){

}


DMA_EVENTLISTENER(tch_lld_spi1_rxdma_eventlistener){
	tch_spi_prototype* sins = &SPI_StaticInstances[0];
	return tch_lld_spi_handle_rxdma_event(sins);
}

DMA_EVENTLISTENER(tch_lld_spi2_rxdma_eventlistener){
	tch_spi_prototype* sins = &SPI_StaticInstances[1];
	return tch_lld_spi_handle_rxdma_event(sins);
}

DMA_EVENTLISTENER(tch_lld_spi3_rxdma_eventlistener){
	tch_spi_prototype* sins = &SPI_StaticInstances[2];
	return tch_lld_spi_handle_rxdma_event(sins);
}

DMA_EVENTLISTENER(tch_lld_spi1_txdma_eventlistener){

}

DMA_EVENTLISTENER(tch_lld_spi2_txdma_eventlistener){

}

DMA_EVENTLISTENER(tch_lld_spi3_txdma_eventlistener){

}
