/*
 * fmo_usart.h
 *
 *  Created on: 2014. 4. 17.
 *      Author: innocentevil
 */

#ifndef FMO_USART_H_
#define FMO_USART_H_

#include "hw_descriptor_types.h"


typedef struct _tch_usart_instance_t tch_usart_instance;
typedef struct _tch_usart_cfg_t tch_usart_cfg;


#define USART_Parity_Pos_CR1                (uint8_t) (9)
#define USART_Parity_NON                    (uint8_t) (0 << 1 | 0 << 0)
#define USART_Parity_ODD                    (uint8_t) (1 << 1 | 1 << 0)
#define USART_Parity_EVEN                   (uint8_t) (1 << 1 | 0 << 0)

#define USART_StopBit_Pos_CR2               (uint8_t) (12)
#define USART_StopBit_1B                    (uint8_t) (0)
#define USART_StopBit_0dot5B                (uint8_t) (1)
#define USART_StopBit_2B                    (uint8_t) (2)
#define USART_StopBit_1dot5B                (uint8_t) (3)


struct _tch_usart_cfg_t {
	uint8_t USART_StopBit;
	uint8_t USART_Parity;
};


struct _tch_usart_instance_t {
	BOOL (*open)(const tch_usart_instance* self,uint32_t brate,tch_pwrMgrCfg pmopt,tch_usart_cfg* cfg);
	BOOL (*close)(const tch_usart_instance* self);
	BOOL (*putc)(const tch_usart_instance* self,uint8_t c);
	uint8_t (*getc)(const tch_usart_instance* self);
	BOOL (*write)(const tch_usart_instance* self,const uint8_t* bp,uint32_t size,uint16_t* err);
	BOOL (*read)(const tch_usart_instance* self,uint8_t* bp,uint32_t size,uint32_t timeout,uint16_t* err);
	uint32_t (*available)(const tch_usart_instance* self);
	BOOL (*writeCstr)(const tch_usart_instance* self,const char* cstr,uint16_t* err);
	BOOL (*readCstr)(const tch_usart_instance* self,char* cstr,uint32_t timeout,uint16_t* err);
};


const tch_usart_instance* usart1;
const tch_usart_instance* usart2;
const tch_usart_instance* usart3;




#endif /* FMO_USART_H_ */
