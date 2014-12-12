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



typedef struct tch_iic_handle_t tch_iicHandle;
typedef uint8_t tch_iic;
typedef struct tch_iic_cfg_t tch_iicCfg;

struct tch_iic_str_t{
	uint8_t iic1;
	uint8_t iic2;
	uint8_t iic3;
};

struct tch_iic_role_t {
	uint8_t Master;
	uint8_t Slave;
};


struct tch_iic_opmode_t {
	uint8_t Standard;
	uint8_t Fast;
};

struct tch_iic_brate_t {
	uint8_t High;
	uint8_t Mid;
	uint8_t Low;
};

struct tch_iic_cfg_t {
	uint8_t Role;       // Master or Slave Role
	uint8_t OpMode;     // Fast or Standard Mode
	uint8_t Baudrate;   // Baudrate option
	BOOL    Filter;
};

struct tch_iic_handle_t{
	tchStatus (*close)(tch_iicHandle* self);
	tchStatus (*write)(tch_iicHandle* self,uint8_t addr,const void* wb,size_t sz);
	tchStatus (*read)(tch_iicHandle* self,uint8_t addr,void* rb,size_t sz,uint32_t timeout);
};


typedef struct tch_lld_iic_t {
	struct tch_iic_str_t    iic;
	struct tch_iic_role_t   Role;
	struct tch_iic_opmode_t OpMode;
	struct tch_iic_brate_t  Baudrate;
	tch_iicHandle* (*allocIIC)(const tch* env,tch_iic iic,tch_iicCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
}tch_lld_iic;


extern const tch_lld_iic* tch_i2c_instance;


#endif /* TCH_I2C_H_ */
