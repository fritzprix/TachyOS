/*
 * tch_i2c.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include "hal/tch_hal.h"
#include "tch_i2c.h"
#include "tch_halInit.h"
#include "tch_halcfg.h"


typedef struct tch_iic_handle_prototype_t tch_iic_handle_prototype;

#define TCH_IIC_CLASS_KEY                      ((uint16_t) 0x62D1)
#define TCH_IIC_OCCP_MSK                       ((uint32_t) 0x0010)


#define IIC_ROLE_MASTER                        ((uint8_t) 1)
#define IIC_ROLE_SLAVE                         ((uint8_t) 0)

#define IIC_OPMODE_STANDARD                    ((uint8_t) 0)
#define IIC_OPMODE_FAST                        ((uint8_t) 1)

#define IIC_BAUDRATE_HIGH                      ((uint8_t) 2)
#define IIC_BAUDRATE_MID                       ((uint8_t) 1)
#define IIC_BAUDRATE_LOW                       ((uint8_t) 0)


#define INIT_IIC_STR                           {0,1,2}
#define INIT_IIC_ROLE                          {\
	                                             IIC_ROLE_MASTER,\
	                                             IIC_ROLE_SLAVE\
                                               }

#define INIT_IIC_OPMODE                        {\
	                                             IIC_OPMODE_STANDARD,\
	                                             IIC_OPMODE_FAST\
                                               }

#define INIT_IIC_BAUDRATE                      {\
	                                             IIC_BAUDRATE_HIGH,\
	                                             IIC_BAUDRATE_MID,\
	                                             IIC_BAUDRATE_LOW\
                                               }


struct tch_iic_handle_prototype_t {
	tch_iicHandle                        pix;
	tch_iic                              iic;
	uint32_t                             status;
	union rxch {
		tch_DmaHandle*                   dma;
		tch_msgqId                       mq;
	};
	union txch {
		tch_DmaHandle*                   dma;
		tch_msgqId                       mq;
	};
	const tch*                           env;
	tch_mtxId                            mtx;
	tch_condvId                          condv;
};


static tch_iicHandle* tch_IIC_alloc(const tch* env,tch_iic i2c,tch_iicCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
static tchStatus tch_IIC_free(tch_iicHandle* self);


static tchStatus tch_IIC_writeDma(tch_iicHandle* self,uint8_t addr,const void* wb,size_t sz);
static tchStatus tch_IIC_readDma(tch_iicHandle* self,uint8_t addr,void* rb,size_t sz,uint32_t timeout);

static tchStatus tch_IIC_write(tch_iicHandle* self,uint8_t addr,const void* wb,size_t sz);
static tchStatus tch_IIC_read(tch_iicHandle* self,uint8_t addr,void* rb,size_t sz,uint32_t timeout);

static void tch_IICValidate(tch_iic_handle_prototype* hnd);
static BOOL tch_IICisValid(tch_iic_handle_prototype* hnd);
static void tch_IICInvalidate(tch_iic_handle_prototype* hnd);

typedef struct tch_lld_i2c_prototype {
	tch_lld_iic                           pix;
	tch_mtxId                             mtx;
	tch_condvId                           condv;
} tch_lld_i2c_prototype;


/**
 */
__attribute__((section(".data"))) static tch_lld_i2c_prototype IIC_StaticInstance = {
		{
				INIT_IIC_STR,
				INIT_IIC_ROLE,
				INIT_IIC_OPMODE,
				INIT_IIC_BAUDRATE,
				tch_IIC_alloc,
				tch_IIC_free
		},
		NULL,
		NULL

};


const tch_lld_iic* tch_i2c_instance = (const tch_lld_iic*) &IIC_StaticInstance;


static tch_iicHandle* tch_IIC_alloc(const tch* env,tch_iic i2c,tch_iicCfg* cfg,uint32_t timeout,tch_PwrOpt popt){
	if(!(i2c < MFEATURE_IIC))
		return NULL;
	if(!IIC_StaticInstance.mtx)
		IIC_StaticInstance.mtx = env->Mtx->create();
	if(!IIC_StaticInstance.condv)
		IIC_StaticInstance.condv = env->Condv->create();

	tch_iic_descriptor* iicHw = &IIC_HWs[i2c];
	tch_iic_bs* iicCfgs = &IIC_BD_CFGs[i2c];

	tchStatus result = osOK;
	if((result = env->Mtx->lock(IIC_StaticInstance.mtx,timeout)) != osOK)
		return NULL;
	while(iicHw->_handle){
		if((result = env->Condv->wait(IIC_StaticInstance.condv,IIC_StaticInstance.mtx,timeout)) != osOK)
			return NULL;
	}
	iicHw->_handle = TCH_IIC_OCCP_MSK;  // mark as occupied
	if((result = env->Mtx->unlock(IIC_StaticInstance.mtx)) != osOK)
		return NULL;

	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) env->Mem->alloc(sizeof(tch_iic_handle_prototype));
	if(!ins){

	}
	env->uStdLib->string->memset(ins,0,sizeof(tch_iic_handle_prototype));
	ins->env = env;





}

static tchStatus tch_IIC_free(tch_iicHandle* self){

}


static tchStatus tch_IIC_writeDma(tch_iicHandle* self,uint8_t addr,const void* wb,size_t sz){

}

static tchStatus tch_IIC_readDma(tch_iicHandle* self,uint8_t addr,void* rb,size_t sz,uint32_t timeout){

}

static tchStatus tch_IIC_write(tch_iicHandle* self,uint8_t addr,const void* wb,size_t sz){

}

static tchStatus tch_IIC_read(tch_iicHandle* self,uint8_t addr,void* rb,size_t sz,uint32_t timeout){

}

static void tch_IICValidate(tch_iic_handle_prototype* hnd){

}

static BOOL tch_IICisValid(tch_iic_handle_prototype* hnd){

}

static void tch_IICInvalidate(tch_iic_handle_prototype* hnd){

}

