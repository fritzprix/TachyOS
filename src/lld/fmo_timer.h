/*
 * fmo_timer.h
 *
 *  Created on: 2014. 4. 2.
 *      Author: innocentevil
 */

#ifndef FMO_TIMER_H_
#define FMO_TIMER_H_
#include "../core/port/cortex_v7m_port.h"
#include "hw_descriptor_types.h"



#define GPT_TIMEOUT_LISTENER(fn)                       void fn(tch_gptimer_instance* ins, uint32_t id)


/**
 * Driver Instances
 */
typedef struct _tch_pwm_instance_t tch_pwm_instance;
typedef struct _tch_gptimer_instance_t tch_gptimer_instance;
typedef struct _tch_clksrc_instance_t tch_clk_instance;
typedef struct _tch_capt_instance_t tch_pcapt_intance;
typedef enum {
	CC1 = 0,CC2 = 1,CC3 = 2,CC4 = 3,TG,UG
}tch_timer_event_type;

typedef uint16_t tch_timer;

/**
 * event listener (callback function type
 */
typedef void (*tch_gptimer_listener)(tch_gptimer_instance* dd,uint32_t id);

struct _tch_pwm_instance_t{
	BOOL (*start)(tch_pwm_instance* self);
	BOOL (*stop)(tch_pwm_instance* self);
	uint8_t (*getChannelCount)(tch_pwm_instance* self);
	void (*setDuty)(tch_pwm_instance* self,uint8_t ch,uint32_t duty);                         // set duty of pwm in range of 16 bit integer (0 ~ 66258)
	uint32_t (*getDuty)(tch_pwm_instance* self,uint8_t ch);                                   // get current duty of pwm device
	uint32_t (*getMaxDuty)(tch_pwm_instance* self);
	BOOL (*close)(tch_pwm_instance* self);                                                    // release pwm h/w and disable
};

struct _tch_gptimer_instance_t{
	uint32_t (*getCurrentTime)(tch_gptimer_instance* self);
	int (*setTimeout)(tch_gptimer_instance* self,uint32_t id,uint32_t timeIn_ms);
	int (*close)(tch_gptimer_instance* self);
};

struct _tch_clksrc_instance_t{
	int (*start)(tch_clk_instance* self);
	void (*setFrequency)(tch_clk_instance* self,uint32_t FreqInkHz);
	uint32_t (*getFrequency)(tch_clk_instance* self);
	void (*pause)(tch_clk_instance* self);
	void (*resume)(tch_clk_instance* self);
	int (*close)(tch_clk_instance* self);
};

struct _tch_capt_instance_t{
	void (*setMaxPeriod)(tch_pcapt_intance* self,uint32_t periodIn_ms);
	uint32_t (*read)(tch_pcapt_intance* self);
	int (*readBurst)(tch_pcapt_intance* self,uint32_t size);
	int (*close)(tch_pcapt_intance* self);
};


#define Timer2        ((tch_timer) 0)
#define Timer3        ((tch_timer) 1)
#define Timer4        ((tch_timer) 2)
#define Timer5        ((tch_timer) 3)
#define Timer9        ((tch_timer) 4)
#define Timer10       ((tch_timer) 5)
#define Timer11       ((tch_timer) 6)
#define Timer12       ((tch_timer) 7)
#define Timer13       ((tch_timer) 8)
#define Timer14       ((tch_timer) 9)


tch_pwm_instance* lld_timer_openTimerAsPulseOut(tch_timer timer,uint32_t periodIn_us,tch_pwrMgrCfg pcfg);
tch_gptimer_instance* lld_timer_openGPTimer(tch_timer timer,uint32_t unitTimeIn_us,tch_gptimer_listener listener,tch_pwrMgrCfg pcfg);
tch_clk_instance* lld_timer_openTimerAsClk(tch_timer timer,uint32_t FreqInKHz,tch_pwrMgrCfg pcfg);
tch_pcapt_intance* lld_timer_openTimerAsPulseIn(tch_timer timer,uint32_t periodIn_ms,tch_pwrMgrCfg pcfg);
void lld_timer_setEventGeneration(tch_timer timer, tch_timer_event_type ev, BOOL nstate);



#endif /* FMO_TIMER_H_ */
