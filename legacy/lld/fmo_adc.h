/*
 * fmo_adc.h
 *
 *  Created on: 2014. 5. 8.
 *      Author: innocentevil
 */

#ifndef FMO_ADC_H_
#define FMO_ADC_H_
#include "../core/port/tch_stdtypes.h"
#include "hw_descriptor_types.h"


#define ADC_Resolution_Pos            ((uint8_t) 24)
#define ADC_Resolution_12B            ((uint8_t) 0)
#define ADC_Resolution_10B            ((uint8_t) 1)
#define ADC_Resolution_8B             ((uint8_t) 2)
#define ADC_Resolution_6B             ((uint8_t) 3)
#define ADC_Resolution_Msk            ((uint8_t) 3)

#define ADC_SampleHold_Offset         ((uint8_t) 3)
#define ADC_SampleHold_3Cycle         ((uint8_t) 0)
#define ADC_SampleHold_15Cycle        ((uint8_t) 1)
#define ADC_SampleHold_28Cycle        ((uint8_t) 2)
#define ADC_SampleHold_56Cycle        ((uint8_t) 3)
#define ADC_SampleHold_84Cycle        ((uint8_t) 4)
#define ADC_SampleHold_112Cycle       ((uint8_t) 5)
#define ADC_SampleHold_144Cycle       ((uint8_t) 6)
#define ADC_SampleHold_480Cycle       ((uint8_t) 7)
#define ADC_SampleHold_Msk            ((uint8_t) 7)

typedef struct _tch_adc_instance tch_adc_instance;
typedef struct _tch_adc_cfg_t tch_adc_cfg;
typedef struct _tch_adc_chcfg_t tch_adc_chcfg;

/**
 * ADC Channel Config Type
 */
struct _tch_adc_chcfg_t {
	uint8_t     ch;                 // adc channel to use
	tch_gpio_t  port;               // io port correspond to adc channel
	uint8_t     pin;                // io pin to be used as analog input
};


/**
 *  ADC Config Type
 */
struct _tch_adc_cfg_t {
	uint8_t ADC_Resolution;                   /// conversion resolution
	uint8_t ADC_SampleHold;                   /// sample hold time
	uint32_t ADC_SampleFreqInHz;              /// sampling freq. which affects multiple sample request (e.g. read() )
	tch_adc_chcfg* ADC_ChCfg_List;            /// can be null -> default Ch cfg is used (ref. bl_confg.h)
	uint8_t ADC_ChCnt;                        /// can be zero
};


/**
 *  ADC Driver Model
 */
struct _tch_adc_instance {
	BOOL (*open)(const tch_adc_instance* self,tch_adc_cfg* cfg,tch_pwrMgrCfg pcfg);
	BOOL (*close)(const tch_adc_instance* self);
	uint16_t (*convert)(const tch_adc_instance* self,uint8_t ch);
	BOOL (*read)(const tch_adc_instance* self,uint16_t* rb,uint32_t size,uint16_t ch);
	BOOL (*scan)(const tch_adc_instance* self,uint8_t* chlist,uint16_t* rb,uint32_t scancnt);    // size of rb should be cautiously chosen considering scan count and channel list
};

void tch_lld_adc_initCfg(tch_adc_cfg* cfg);
/**
 *  ADC Driver Static Object
 */
extern const tch_adc_instance* adc1;
extern const tch_adc_instance* adc2;
extern const tch_adc_instance* adc3;

#endif /* FMO_ADC_H_ */
