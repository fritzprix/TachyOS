/*
 * tch_i2c.h
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#ifndef TCH_I2C_H_
#define TCH_I2C_H_

#if defined(__cplusplus)
extern "C"{
#endif


#define IIc1                  ((tch_iic) 0)
#define IIc2                  ((tch_iic) 1)
#define IIc3                  ((tch_iic) 2)


#define IIC_ROLE_MASTER                        ((uint8_t) 1)
#define IIC_ROLE_SLAVE                         ((uint8_t) 0)

#define IIC_OPMODE_STANDARD                    ((uint8_t) 0)
#define IIC_OPMODE_FAST                        ((uint8_t) 1)

#define IIC_BAUDRATE_HIGH                      ((uint8_t) 2)   // Standard Mode : 100kHz  // Fast Mode : 400kHz
#define IIC_BAUDRATE_MID                       ((uint8_t) 1)   // Standard Mode : 50kHz   // Fast Mode : 200kHz
#define IIC_BAUDRATE_LOW                       ((uint8_t) 0)   // Standard Mode : 25kHz   // Fast Mode : 100kHz

#define IIC_ADDRMODE_7B                        ((uint8_t) 0)
#define IIC_ADDRMODE_10B                       ((uint8_t) 1)



typedef struct tch_iic_handle_t tch_iicHandle;
typedef uint8_t tch_iic;
typedef struct tch_iic_cfg_t tch_iicCfg;


struct tch_iic_cfg_t {
	uint16_t    Addr;
	uint8_t     AddrMode;
	uint8_t     Role;       // Master or Slave Role
	uint8_t     OpMode;     // Fast or Standard Mode
	uint8_t     Baudrate;   // Baudrate option
	BOOL        Filter;
};

struct tch_iic_handle_t{
	tchStatus (*close)(tch_iicHandle* self);
	tchStatus (*write)(tch_iicHandle* self,uint16_t addr,const void* wb,int32_t sz);
	uint32_t (*read)(tch_iicHandle* self,uint16_t addr,void* rb,int32_t sz,uint32_t timeout);
};


typedef struct tch_lld_iic_t {
	void (*initCfg)(tch_iicCfg* cfg);
	tch_iicHandle* (*allocIIC)(const tch* env,tch_iic iic,tch_iicCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
}tch_lld_iic;




#if defined(__cplusplus)
}
#endif

#endif /* TCH_I2C_H_ */
