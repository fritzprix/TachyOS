/* tch_adc.h
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 */

#ifndef TCH_ADC_H_
#define TCH_ADC_H_

#if defined(__cplusplus)
extern "C"{
#endif


#define tch_ADC1                  ((adc_t) 0)
#define tch_ADC2                  ((adc_t) 1)
#define tch_ADC3                  ((adc_t) 2)
#define tch_ADC4                  ((adc_t) 3)

#define tch_ADC_Ch1               ((uint8_t) 0)
#define tch_ADC_Ch2               ((uint8_t) 1)
#define tch_ADC_Ch3               ((uint8_t) 2)
#define tch_ADC_Ch4               ((uint8_t) 3)
#define tch_ADC_Ch5               ((uint8_t) 4)
#define tch_ADC_Ch6               ((uint8_t) 5)
#define tch_ADC_Ch7               ((uint8_t) 6)
#define tch_ADC_Ch8               ((uint8_t) 7)
#define tch_ADC_Ch9               ((uint8_t) 8)
#define tch_ADC_Ch10              ((uint8_t) 9)
#define tch_ADC_Ch11              ((uint8_t) 10)
#define tch_ADC_Ch12              ((uint8_t) 11)



#define ADC_Precision_12B            ((uint8_t) 0)
#define ADC_Precision_10B            ((uint8_t) 1)
#define ADC_Precision_8B             ((uint8_t) 2)
#define ADC_Precision_6B             ((uint8_t) 3)

#define ADC_SampleHold_Short         ((uint8_t) 0)
#define ADC_SampleHold_Mid           ((uint8_t) 1)
#define ADC_SampleHold_Long          ((uint8_t) 2)
#define ADC_SampleHold_VeryLong      ((uint8_t) 3)

#define ADC_CONV_INFINITE            ((uint32_t) -1)
#define ADC_READ_FAIL                ((uint32_t) -1)



typedef struct tch_adc_handle_t tch_adcHandle;
typedef struct tch_adc_cfg_t tch_adcCfg;
typedef struct tch_adc_ch_def_t tch_adcChDef;
typedef uint16_t adc_t;



struct tch_adc_ch_def_t {
	uint16_t chselMsk;
	uint8_t  chcnt;
};

struct tch_adc_cfg_t {
	uint8_t      Precision;
	uint8_t      SampleHold;
	uint32_t     SampleFreq;
	tch_adcChDef chdef;
};


struct tch_adc_handle_t {
	tchStatus (*close)(tch_adcHandle* self);
	uint32_t (*read)(const tch_adcHandle* self,uint8_t ch);
	tchStatus (*readBurst)(const tch_adcHandle* self,uint8_t ch,tch_mailqId q,uint32_t convCnt);
};

typedef struct tch_lld_adc_t {
	const uint16_t ADC_COUNT;
	const uint8_t  ADC_MAX_PRECISION;
	const uint8_t  ADC_MIN_PRECISION;
	void (*initCfg)(tch_adcCfg* cfg);
	void (*addChannel)(tch_adcChDef* chdef,uint8_t ch);
	void (*removeChannel)(tch_adcChDef* chdef,uint8_t ch);
	tch_adcHandle* (*open)(const tch* env,adc_t adc,tch_adcCfg* cfg,tch_PwrOpt popt,uint32_t timeout);
	BOOL (*available)(adc_t adc);
}tch_lld_adc;

#if defined(__cplusplus)
}
#endif

#endif /* TCH_ADC_H_ */
