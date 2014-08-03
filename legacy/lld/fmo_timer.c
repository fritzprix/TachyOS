/*
 * fmo_timer.c
 *
 *  Created on: 2014. 4. 2.
 *      Author: innocentevil
 */

#include "../core/port/cortex_v7m_port.h"
#include "fmo_timer.h"
#include "../core/fmo_sched.h"
#include "stm32f4xx_tim.h"





/**
 * Macro Variable
 */

#define _PWM_PINTERFACE {\
	lld_timer_pwm_start,\
	lld_timer_pwm_stop,\
	lld_timer_pwm_getChannelCount,\
	lld_timer_pwm_setDuty,\
	lld_timer_pwm_getDuty,\
	lld_timer_pwm_getMaxDuty,\
    lld_timer_pwm_close}

#define _GPT_PINTERFACE {\
	tch_GptgetCurrentTime,\
	tch_GptSetTimeout,\
	tch_GptClose}

#define _CLK_PINTERFACE {\
	tch_ClkStart,\
	tch_ClkSetFrequency,\
	tch_ClkGetFrequency,\
	tch_ClkPause,\
	tch_ClkResume,\
	tch_ClkClose}

#define _PCapt_INTERFACE {\
	tch_pCaptSetMaxPeriod,\
	tch_pCaptRead,\
	tch_pCaptReadBurst,\
	tch_pCaptClose}

#define _GPT_REQ_INIT(ccp) {MTX_INIT,0,ccp}
/**
 * Macro Function
 */

/**
 * 	tch_pwm_instance                   p_ins;
	mtx_lock                           p_lock;
	tch_timer                          p_timer;
	uint16_t                           p_status;
	uint16_t                           p_periodIn_us;
	volatile uint32_t*                 p_pulse[4];
 */
#define PWM_INSTANCE_INIT(x) {_PWM_PINTERFACE,MTX_INIT,x,0,0,{NULL,NULL,NULL,NULL}}
#define GPT_INSTANCE_INIT(x,TIMx) {_GPT_PINTERFACE,MTX_INIT,x,0,NULL,{_GPT_REQ_INIT(&TIMx->CCR1),_GPT_REQ_INIT(&TIMx->CCR2),_GPT_REQ_INIT(&TIMx->CCR3),_GPT_REQ_INIT(&TIMx->CCR4)}}

#define HANDLE_GPT_INTERRUPT(ins,thw){\
	if((thw->_hw->SR & TIM_SR_CC1IF) != 0){\
			if(ins->p_listener != NULL){\
				ins->p_listener((GPTimer_Driver*)ins,ins->p_req[0].r_id);\
			}\
			ins->p_req[0].r_id = 0;\
			tch_Mtx_unlockt(&ins->p_req[0].r_lock);\
			thw->_hw->SR &= ~TIM_SR_CC1IF;\
			thw->_hw->DIER &= ~TIM_DIER_CC1IE;\
			return;\
		}\
		if((thw->_hw->SR & TIM_SR_CC2IF) != 0){\
			if(ins->p_listener != NULL){\
				ins->p_listener((GPTimer_Driver*)ins,ins->p_req[1].r_id);\
			}\
			ins->p_req[1].r_id = 0;\
			tch_Mtx_unlockt(&ins->p_req[1].r_lock);\
			thw->_hw->SR &= ~TIM_SR_CC2IF;\
			thw->_hw->DIER &= ~TIM_DIER_CC2IE;\
			return;\
		}\
		if((thw->_hw->SR & TIM_SR_CC3IF) != 0){\
			if(ins->p_listener != NULL){\
				ins->p_listener((GPTimer_Driver*)ins,ins->p_req[2].r_id);\
			}\
			ins->p_req[2].r_id = 0;\
			tch_Mtx_unlockt(&ins->p_req[2].r_lock);\
			thw->_hw->SR &= ~TIM_SR_CC3IF;\
			thw->_hw->DIER &= ~TIM_DIER_CC3IE;\
			return;\
		}\
		if((thw->_hw->SR & TIM_SR_CC4IF) != 0){\
			if(ins->p_listener != NULL){\
				ins->p_listener((GPTimer_Driver*)ins,ins->p_req[3].r_id);\
			}\
			ins->p_req[3].r_id = 0;\
			tch_Mtx_unlockt(&ins->p_req[3].r_lock);\
			thw->_hw->SR &= ~TIM_SR_CC4IF;\
			thw->_hw->DIER &= ~TIM_DIER_CC4IE;\
			return;\
		}\
}

typedef struct _tch_pwm_prototype_t tch_pwm_prototype;
typedef struct _tch_gpt_prototype_t tch_gptimer_prototype;
typedef struct _tch_clk_prototype_t tch_clk_prototype;
typedef struct _tch_pcapt_prototype_t tch_pcapt_prototype;

/**
 * private function
 * 	BOOL (*start)(tch_pwm_instance* self);
	BOOL (*stop)(tch_pwm_instance* self);
	uint8_t (*getChannelCount)(tch_pwm_instance* self);
	BOOL (*isChannelEnabled)(tch_pwm_instance* self,uint8_t ch);
	void (*setChannelEnable)(tch_pwm_instance* self,uint8_t ch,BOOL enable);
	void (*setDuty)(tch_pwm_instance* self,uint8_t ch,uint32_t duty);                         // set duty of pwm in range of 16 bit integer (0 ~ 66258)
	uint32_t (*getDuty)(tch_pwm_instance* self,uint8_t ch);                                   // get current duty of pwm device
	uint32_t (*getMaxDuty)(tch_pwm_instance* self,uint8_t ch);
	BOOL (*close)(tch_pwm_instance* self);
 */
static BOOL lld_timer_pwm_start(tch_pwm_instance* self);
static BOOL lld_timer_pwm_stop(tch_pwm_instance* self);
static uint8_t lld_timer_pwm_getChannelCount(tch_pwm_instance* self);
static void lld_timer_pwm_setDuty(tch_pwm_instance* self,uint8_t ch,uint32_t duty);                         // set duty of pwm in range of 16 bit integer (0 ~ 66258)
static uint32_t lld_timer_pwm_getDuty(tch_pwm_instance* self,uint8_t ch);                                   // get current duty of pwm device
static uint32_t lld_timer_pwm_getMaxDuty(tch_pwm_instance* self);
static BOOL lld_timer_pwm_close(tch_pwm_instance* self);                                          // release pwm h/w and disable


static uint32_t tch_GptgetCurrentTime(tch_gptimer_instance* self);
static int tch_GptSetTimeout(tch_gptimer_instance* self,uint32_t id,uint32_t timeIn_ms);
static int tch_GptClose(tch_gptimer_instance* self);


static int tch_ClkStart(tch_clk_instance* self);
static void tch_ClkSetFrequency(tch_clk_instance* self,uint32_t FreqInkHz);
static uint32_t tch_ClkGetFrequency(tch_clk_instance* self);
static void tch_ClkPause(tch_clk_instance* self);
static void tch_ClkResume(tch_clk_instance* self);
static int tch_ClkClose(tch_clk_instance* self);


static void tch_pCaptSetMaxPeriod(tch_pcapt_intance* self,uint32_t periodIn_ms);
static uint32_t tch_pCaptRead(tch_pcapt_intance* self);
static int tch_pCaptReadBurst(tch_pcapt_intance* self,uint32_t size);
static int tch_pCaptClose(tch_pcapt_intance* self);

/**
 * private variable
 */


static inline void __handle_gpt_interrupt(tch_gptimer_prototype* ins,timer_hw_descriptor* thw)__attribute__((always_inline));


#define PWM_STATUS_INIT_Pos           ((uint8_t) 0)
#define PWM_STATUS_INIT_Msk           ((uint16_t) 1 << PWM_STATUS_INIT_Pos)
#define PWM_STATUS_CHCNT_Pos          ((uint8_t) 1)
#define PWM_STATUS_CHCNT_Msk          (7 << PWM_STATUS_CHCNT_Pos)

#define PWM_SET_INIT(pwm_p)           (pwm_p->p_status |= PWM_STATUS_INIT_Msk)
#define PWM_CLR_INIT(pwm_p)           (pwm_p->p_status &= ~PWM_STATUS_INIT_Msk)
#define PWM_IS_INIT(pwm_p)            (pwm_p->p_status & PWM_STATUS_INIT_Msk)

#define PWM_SET_CHCNT(pwm_p,cnt)      (pwm_p->p_status |= (cnt << PWM_STATUS_CHCNT_Pos))
#define PWM_CLR_CHCNT(pwm_p)          (pwm_p->p_status &= ~PWM_STATUS_CHCNT_Msk)
#define PWM_GET_CHCNT(pwm_p)          (pwm_p->p_status & PWM_STATUS_CHCNT_Msk)

struct _tch_pwm_prototype_t {
	tch_pwm_instance                   p_ins;
	mtx_lock                           p_lock;
	tch_timer                          p_timer;
	uint16_t                           p_status;
	uint16_t                           p_periodIn_us;
	volatile uint32_t*                 p_pulse[4];
};

struct _tch_gpt_request_t {
	mtx_lock               r_lock;
	uint32_t               r_id;
	volatile uint32_t*    _r_time;
};


#define GPT_STATUS_INIT                     (uint16_t) (1 << 0)
#define GPT_STATUS_MAXREQ_MASK                (uint16_t) (7 << 1)

#define GPT_SET_MAXREQ(gpt_p,max)  {\
	gpt_p->p_status &= ~(GPT_STATUS_MAXREQ_MASK);\
	gpt_p->p_status |= ((max << 1) & GPT_STATUS_MAXREQ_MASK);\
}

#define GPT_GET_MAXREQ(gpt_p)   (uint8_t) ((gpt_p->p_status & GPT_STATUS_MAXREQ_MASK) >> 1)
#define GPT_SET_INIT(gpt_p)  (gpt_p->p_status |= GPT_STATUS_INIT)
#define GPT_CLR_INIT(gpt_p) (gpt_p->p_status &= ~GPT_STATUS_INIT)
#define GPT_IS_INIT(gpt_p)  (gpt_p->p_status & GPT_STATUS_INIT)

struct _tch_gpt_prototype_t {
	tch_gptimer_instance                P_ins;
	mtx_lock                            p_lock;
	tch_timer                           p_timer;
	uint16_t                            p_status;
	tch_gptimer_listener                p_listener;
	struct _tch_gpt_request_t           p_req[4];
};

struct _tch_clk_prototype_t {

};

struct _tch_pcapt_prototype_t {

};


#define TIMER_STATUS_USED_AS_UNDEFINED                  (uint32_t) (0)
#define TIMER_STATUS_USED_AS_PWM                        (uint32_t) (1 << 0)
#define TIMER_STATUS_USED_AS_GPT                        (uint32_t) (1 << 1)
#define TIMER_STATUS_USED_AS_CLK                        (uint32_t) (1 << 2)
#define TIMER_STATUS_USED_AS_PCAPT                      (uint32_t) (1 << 3)
#define TIMER_STATUS_USED_MASK                          (TIMER_STATUS_USED_AS_CLK|\
		                                                 TIMER_STATUS_USED_AS_GPT|\
		                                                 TIMER_STATUS_USED_AS_PCAPT|\
		                                                 TIMER_STATUS_USED_AS_PWM)


#define TIMER_SET_USE_MODE(thw_p,mode) {\
	thw_p->status_flags &= ~TIMER_STATUS_USED_MASK;\
	thw_p->status_flags |= mode;\
}
#define TIMER_GET_USE_MODE(thw_p) (thw_p->status_flags & TIMER_STATUS_USED_MASK)

typedef struct _timer_hw_desc_t timer_hw_descriptor;


struct _timer_hw_desc_t TIMER_Hw_Desc[] = {
		{
				TIM2,
				0,
				TIMER_FEATURE_RES32B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1|TIMER_FEATURE_CH_2|TIMER_FEATURE_CH_3|TIMER_FEATURE_CH_4,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_TIM2EN,
				RCC_APB1LPENR_TIM12LPEN,
				TIM2_IRQn
		},
		{
				TIM3,
				0,
				TIMER_FEATURE_RES16B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1|TIMER_FEATURE_CH_2|TIMER_FEATURE_CH_3|TIMER_FEATURE_CH_4,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_TIM3EN,
				RCC_APB1LPENR_TIM3LPEN,
				TIM3_IRQn
		},
		{
				TIM4,
				0,
				TIMER_FEATURE_RES16B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1|TIMER_FEATURE_CH_2|TIMER_FEATURE_CH_3|TIMER_FEATURE_CH_4,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_TIM4EN,
				RCC_APB1LPENR_TIM14LPEN,
				TIM4_IRQn
		},
		{
				TIM5,
				0,
				TIMER_FEATURE_RES32B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1|TIMER_FEATURE_CH_2|TIMER_FEATURE_CH_3|TIMER_FEATURE_CH_4,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_TIM5EN,
				RCC_APB1LPENR_TIM5LPEN,
				TIM5_IRQn
		},
		{
				TIM9,
				0,
				TIMER_FEATURE_RES16B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1|TIMER_FEATURE_CH_2,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_TIM9EN,
				RCC_APB2LPENR_TIM9LPEN,
				TIM1_BRK_TIM9_IRQn
		},
		{
				TIM10,
				0,
				TIMER_FEATURE_RES16B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_TIM10EN,
				RCC_APB2LPENR_TIM10LPEN,
				TIM1_UP_TIM10_IRQn
		},
		{
				TIM11,
				0,
				TIMER_FEATURE_RES16B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1,
				&RCC->APB2ENR,
				&RCC->APB2LPENR,
				RCC_APB2ENR_TIM11EN,
				RCC_APB2LPENR_TIM11LPEN,
				TIM1_TRG_COM_TIM11_IRQn
		},
		{
				TIM12,
				0,
				TIMER_FEATURE_RES16B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1|TIMER_FEATURE_CH_2,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_TIM12EN,
				RCC_APB1LPENR_TIM12LPEN,
				TIM8_BRK_TIM12_IRQn
		},
		{
				TIM13,
				0,
				TIMER_FEATURE_RES16B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_TIM13EN,
				RCC_APB1LPENR_TIM13LPEN,
				TIM8_UP_TIM13_IRQn
		},
		{
				TIM14,
				0,
				TIMER_FEATURE_RES16B|TIMER_FEATURE_COMPARE|TIMER_FEATURE_PULSE_INPUT|TIMER_FEATURE_PWM|TIMER_FEATURE_CH_1,
				&RCC->APB1ENR,
				&RCC->APB1LPENR,
				RCC_APB1ENR_TIM14EN,
				RCC_APB1LPENR_TIM14LPEN,
				TIM8_TRG_COM_TIM14_IRQn
		}
};

static struct _tch_pwm_prototype_t PWM_Instances[] = {
		PWM_INSTANCE_INIT(0),
		PWM_INSTANCE_INIT(1),
		PWM_INSTANCE_INIT(2),
		PWM_INSTANCE_INIT(3),
		PWM_INSTANCE_INIT(4),
		PWM_INSTANCE_INIT(5),
		PWM_INSTANCE_INIT(6),
		PWM_INSTANCE_INIT(7),
		PWM_INSTANCE_INIT(8),
		PWM_INSTANCE_INIT(9)
};

static struct _tch_gpt_prototype_t GpTimer_Instances[] = {
		GPT_INSTANCE_INIT(0,TIM2),
		GPT_INSTANCE_INIT(1,TIM3),
		GPT_INSTANCE_INIT(2,TIM4),
		GPT_INSTANCE_INIT(3,TIM5),
		GPT_INSTANCE_INIT(4,TIM9),
		GPT_INSTANCE_INIT(5,TIM10),
		GPT_INSTANCE_INIT(6,TIM11),
		GPT_INSTANCE_INIT(7,TIM12),
		GPT_INSTANCE_INIT(8,TIM13),
		GPT_INSTANCE_INIT(9,TIM14)
};

static struct _tch_clk_prototype_t Clk_Instances[] = {

};

static struct _tch_pcapt_prototype_t PCapt_Instances[] = {

};




/**
 * public function
 */
tch_pwm_instance* lld_timer_openTimerAsPulseOut(tch_timer timer,uint32_t periodIn_us,tch_pwrMgrCfg pcfg){

	timer_hw_descriptor* thw = &TIMER_Hw_Desc[timer];
	tch_pwm_prototype* ins = &PWM_Instances[timer];
	if(PWM_IS_INIT(ins)){
		return NULL;
	}
	tch_Mtx_lockt(&ins->p_lock,MTX_TRYMODE_WAIT);
	PWM_SET_INIT(ins);


	if((*thw->clken & thw->clkmsk) == 0){
		*thw->clken |= thw->clkmsk;
	}

	TIMER_SET_USE_MODE(thw,TIMER_STATUS_USED_AS_PWM);


	if(pcfg == ActOnSleep){
		*thw->_lpCklenr |= thw->lpClkmsk;
	}else{
		*thw->_lpCklenr &= ~thw->lpClkmsk;
	}

	uint8_t cnt = 1;
	ins->p_pulse[0] = &thw->_hw->CCR1;
	thw->_hw->CCMR1 |= (6 << 4);
	thw->_hw->CCER |= TIM_CCER_CC1E;
	if(thw->feature_flags & TIMER_FEATURE_CH_2){
		cnt++;
		ins->p_pulse[1] = &thw->_hw->CCR2;
		thw->_hw->CCMR1 |= (6 << 12);
		thw->_hw->CCER |= TIM_CCER_CC2E;
	}
	if(thw->feature_flags & TIMER_FEATURE_CH_3){
		cnt++;
		ins->p_pulse[2] = &thw->_hw->CCR3;
		thw->_hw->CCMR2 |= (6 << 4);
		thw->_hw->CCER |= TIM_CCER_CC3E;
	}
	if(thw->feature_flags & TIMER_FEATURE_CH_4){
		cnt++;
		ins->p_pulse[3] = &thw->_hw->CCR4;
		thw->_hw->CCMR2 |= (6 << 12);
		thw->_hw->CCER |= TIM_CCER_CC4E;
	}
	PWM_SET_CHCNT(ins,cnt);

	thw->_hw->CR1 |= (TIM_CR1_URS);
	uint8_t tclk_div = thw->clken == &RCC->APB1ENR? 2 : 1;
	uint64_t tmp_arr = 0;
	uint32_t tmp_psc  = 1;
	if(thw->feature_flags & TIMER_FEATURE_RES32B){
		while((tmp_arr > 0xFFFFFFFF) || (tmp_arr < 1)){
			tmp_arr = ((SYS_CLK / 1000000) / (tclk_div * tmp_psc++)) * periodIn_us;
		}
		thw->_hw->PSC = tmp_psc - 1;
		thw->_hw->ARR = tmp_arr;
	}else{
		while((tmp_arr > 0xFFFF) || (tmp_arr < 1)){
			tmp_arr = ((SYS_CLK / 1000000) / (tclk_div * tmp_psc++)) * periodIn_us;
		}
		thw->_hw->PSC = tmp_psc - 1;
		thw->_hw->ARR = tmp_arr;
	}
	thw->_hw->CNT = 0;
	tch_Mtx_unlockt(&ins->p_lock);
	return (tch_pwm_instance*) ins;

}

tch_gptimer_instance* lld_timer_openGPTimer(tch_timer timer,uint32_t unitTimeInus,tch_gptimer_listener listener,tch_pwrMgrCfg pcfg){

	timer_hw_descriptor* thw = &TIMER_Hw_Desc[timer];
	tch_gptimer_prototype* sp = &GpTimer_Instances[timer];
	if(!GPT_IS_INIT(sp)){
		tch_Mtx_lockt(&sp->p_lock,MTX_TRYMODE_WAIT);
		GPT_SET_INIT(sp);
		tch_Mtx_unlockt(&sp->p_lock);
	}else{
		return NULL;                                                                            /// fail to initiate because this device is already initiated by another thread
	}

	if((*thw->clken & thw->clkmsk) == 0){
		*thw->clken |= thw->clkmsk;                                            // check clock enabled
	}

	if(pcfg == ActOnSleep){
		*thw->_lpCklenr |= thw->lpClkmsk;
	}else{
		*thw->_lpCklenr &= ~thw->lpClkmsk;
	}
	uint8_t maxCh = 1;                                                         // minimum channel number : 1

	TIM_DeInit(thw->_hw);                                                      // clear hw register
	TIM_OCInitTypeDef t_init;
	TIM_OCStructInit(&t_init);                                                 // бщ timer configuration
	t_init.TIM_OCMode = TIM_OCMode_Timing;
	t_init.TIM_OCPolarity = TIM_OCPolarity_High;
	t_init.TIM_OutputState = TIM_OutputState_Disable;
	t_init.TIM_Pulse = 0;
	TIM_OC1Init(thw->_hw,&t_init);

	if((thw->feature_flags & TIMER_FEATURE_CH_2) != 0){                        // check hw feature and enable
		TIM_OC2Init(thw->_hw,&t_init);
		maxCh++;
	}
	if((thw->feature_flags & TIMER_FEATURE_CH_3) != 0){
		TIM_OC3Init(thw->_hw,&t_init);
		maxCh++;
	}
	if((thw->feature_flags & TIMER_FEATURE_CH_4) != 0){
		TIM_OC4Init(thw->_hw,&t_init);
		maxCh++;
	}


	GPT_SET_MAXREQ(sp,maxCh);                                                  // save number of channel

	uint32_t psc = 0;
	if(thw->clken == &RCC->APB1ENR){                                           // check clk source and calculate prescaler value for us
		//apb1 PDIV = 4 , TIMxCLK = 2
		psc = (SYS_CLK / 4 / 1000000) * unitTimeInus;
	}else{
		//apb1 PDIV = 2 , TIMxCLK = 1
		psc = (SYS_CLK / 2 / 1000000) * unitTimeInus;
	}
	TIM_PrescalerConfig(thw->_hw,psc,TIM_PSCReloadMode_Immediate);
	TIM_SetAutoreload(thw->_hw,0xFFFF);                                        // set Timer top value (ovf)

	sp->p_listener = listener;                                                 // set timer event listener
	TIMER_SET_USE_MODE(thw,TIMER_STATUS_USED_AS_GPT);                          // mark hw resource as gpt use
	NVIC_SetPriority(thw->irq,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(thw->irq);                                                  // enable timer interrupt
	TIM_Cmd(thw->_hw,ENABLE);

	return (tch_gptimer_instance*) sp;
}

tch_clk_instance* lld_timer_openTimerAsClk(tch_timer timer,uint32_t FreqInKHz,tch_pwrMgrCfg pcfg){
	return NULL;
}

tch_pcapt_intance* lld_timer_openTimerAsPulseIn(tch_timer timer,uint32_t periodIn_ms,tch_pwrMgrCfg pcfg){
	return NULL;
}

void lld_timer_setEventGeneration(tch_timer timer, tch_timer_event_type ev, BOOL nstate){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[timer];
	switch(ev){
	case CC1:
		thw->_hw->EGR |= TIM_EGR_CC1G;
		break;
	case CC2:
		thw->_hw->EGR |= TIM_EGR_CC2G;
		break;
	case CC3:
		thw->_hw->EGR |= TIM_EGR_CC3G;
		break;
	case CC4:
		thw->_hw->EGR |= TIM_EGR_CC4G;
		break;
	}
}




BOOL lld_timer_pwm_start(tch_pwm_instance* self){
	tch_pwm_prototype* ins = (tch_pwm_prototype*) self;
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[ins->p_timer];
	thw->_hw->CNT = 0;
	thw->_hw->CR1 |= TIM_CR1_CEN;
	return TRUE;
}

BOOL lld_timer_pwm_stop(tch_pwm_instance* self){
	tch_pwm_prototype* ins = (tch_pwm_prototype*) self;
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[ins->p_timer];
	thw->_hw->CR1 &= ~TIM_CR1_CEN;
	return TRUE;
}


uint8_t lld_timer_pwm_getChannelCount(tch_pwm_instance* self){
	tch_pwm_prototype* ins = (tch_pwm_prototype*) self;
	return PWM_GET_CHCNT(ins);
}



void lld_timer_pwm_setDuty(tch_pwm_instance* self,uint8_t ch,uint32_t duty){                         // set duty of pwm in range of 16 bit integer (0 ~ 66258)
	tch_pwm_prototype* ins =(tch_pwm_prototype*) self;
	if(ch > PWM_GET_CHCNT(ins)){
		return;
	}
	*ins->p_pulse[ch] = duty;
}

uint32_t lld_timer_pwm_getDuty(tch_pwm_instance* self,uint8_t ch){                                   // get current duty of pwm device
	tch_pwm_prototype* ins = (tch_pwm_prototype*) self;
	if(ch > PWM_GET_CHCNT(ins)){
		return 0;
	}
	return *ins->p_pulse[ch];
}

uint32_t lld_timer_pwm_getMaxDuty(tch_pwm_instance* self){
	tch_pwm_prototype* ins =(tch_pwm_prototype*) self;
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[ins->p_timer];
	return thw->_hw->ARR;
}


BOOL lld_timer_pwm_close(tch_pwm_instance* self){
	tch_pwm_prototype* ins = (tch_pwm_prototype*) self;
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[ins->p_timer];
	if(!PWM_IS_INIT(ins)){
		return FALSE;
	}
	tch_Mtx_lockt(&ins->p_lock,MTX_TRYMODE_WAIT);
	PWM_CLR_INIT(ins);
	tch_Mtx_unlockt(&ins->p_lock);
	PWM_CLR_CHCNT(ins);
	thw->_hw->CR1 = 0;
	thw->_hw->CR2 = 0;
	thw->_hw->CCER = 0;
	thw->_hw->CCMR1 = 0;
	thw->_hw->CCMR2 = 0;
	*thw->clken &= ~thw->clkmsk;
	*thw->_lpCklenr &= ~thw->lpClkmsk;
	return TRUE;
}




uint32_t tch_GptgetCurrentTime(tch_gptimer_instance* self){
	tch_gptimer_prototype* sp = (tch_gptimer_prototype*) self;
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[sp->p_timer];
	return thw->_hw->CNT;
}

int tch_GptSetTimeout(tch_gptimer_instance* self,uint32_t id,uint32_t timeIn_tu){
	tch_gptimer_prototype* sp = (tch_gptimer_prototype*) self;
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[sp->p_timer];
	uint8_t cc_ch = 0;
	while(cc_ch < GPT_GET_MAXREQ(sp)){                                            // iterate channels
		if(!sp->p_req[cc_ch].r_id){
			if(!__get_IPSR()){
				tch_Mtx_lockt(&sp->p_req[cc_ch].r_lock,MTX_TRYMODE_WAIT);
			}
			sp->p_req[cc_ch].r_id = id;
			if(!__get_IPSR()){
				tch_Mtx_unlockt(&sp->p_req[cc_ch].r_lock);
			}
			*sp->p_req[cc_ch]._r_time = thw->_hw->CNT + timeIn_tu;                // set its timer value and return
			thw->_hw->DIER |= (1 << (cc_ch + 1));
			return SUCCESS;
		}
		cc_ch++;
	}
	return ERROR;                                                                  // there is no available timer
}

int tch_GptClose(tch_gptimer_instance* self){
	tch_gptimer_prototype* sp = (tch_gptimer_prototype*) self;
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[sp->p_timer];
	TIM_Cmd(thw->_hw,DISABLE);                                                    // stop timer operation
	TIM_DeInit(thw->_hw);                                                          // clear hw register
	*thw->clken &= ~thw->clkmsk;                                                   // clock disable for saving power
	NVIC_DisableIRQ(thw->irq);                                                     // disable timer interrupt
	GPT_CLR_INIT(sp);
	return SUCCESS;
}

int tch_ClkStart(tch_clk_instance* self){
	return ERROR;
}

void tch_ClkSetFrequency(tch_clk_instance* self,uint32_t FreqInkHz){

}

uint32_t tch_ClkGetFrequency(tch_clk_instance* self){
	return 0;
}

void tch_ClkPause(tch_clk_instance* self){

}

void tch_ClkResume(tch_clk_instance* self){

}

int tch_ClkClose(tch_clk_instance* self){

}


void tch_pCaptSetMaxPeriod(tch_pcapt_intance* self,uint32_t periodIn_ms){

}

uint32_t tch_pCaptRead(tch_pcapt_intance* self){

}

int tch_pCaptReadBurst(tch_pcapt_intance* self,uint32_t size){

}

int tch_pCaptClose(tch_pcapt_intance* self){

}



/**
 * Interrupt Handler
 */

inline void __handle_gpt_interrupt(tch_gptimer_prototype* ins,timer_hw_descriptor* thw){
	if((thw->_hw->SR & TIM_SR_CC1IF) != 0){
			if(ins->p_listener != NULL){
				ins->p_listener((tch_gptimer_instance*)ins,ins->p_req[0].r_id);
			}
			ins->p_req[0].r_id = 0;
			thw->_hw->SR &= ~TIM_SR_CC1IF;
			thw->_hw->DIER &= ~TIM_DIER_CC1IE;
			return;
		}
		if((thw->_hw->SR & TIM_SR_CC2IF) != 0){
			if(ins->p_listener != NULL){
				ins->p_listener((tch_gptimer_instance*)ins,ins->p_req[1].r_id);
			}
			ins->p_req[1].r_id = 0;
			thw->_hw->SR &= ~TIM_SR_CC2IF;
			thw->_hw->DIER &= ~TIM_DIER_CC2IE;
			return;
		}
		if((thw->_hw->SR & TIM_SR_CC3IF) != 0){
			if(ins->p_listener != NULL){
				ins->p_listener((tch_gptimer_instance*)ins,ins->p_req[2].r_id);
			}
			ins->p_req[2].r_id = 0;
			thw->_hw->SR &= ~TIM_SR_CC3IF;
			thw->_hw->DIER &= ~TIM_DIER_CC3IE;
			return;
		}
		if((thw->_hw->SR & TIM_SR_CC4IF) != 0){
			if(ins->p_listener != NULL){
				ins->p_listener((tch_gptimer_instance*)ins,ins->p_req[3].r_id);
			}
			ins->p_req[3].r_id = 0;
			thw->_hw->SR &= ~TIM_SR_CC4IF;
			thw->_hw->DIER &= ~TIM_DIER_CC4IE;
			return;
		}
}


void TIM2_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer2];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer2];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}

void TIM3_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer3];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer3];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}

void TIM4_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer4];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer4];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}


void TIM5_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer5];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer5];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}


void TIM1_BRK_TIM9_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer9];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer9];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}

void TIM1_UP_TIM10_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer10];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer10];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}

void TIM1_TRG_COM_TIM11_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer11];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer11];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}

void TIM8_BRK_TIM12_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer12];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer12];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}

void TIM8_UP_TIM13_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer13];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer13];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}

void TIM8_TRG_COM_TIM14_IRQHandler(void){
	timer_hw_descriptor* thw = &TIMER_Hw_Desc[Timer14];
	tch_gptimer_prototype* ins = &GpTimer_Instances[Timer14];
	switch(TIMER_GET_USE_MODE(thw)){
	case TIMER_STATUS_USED_AS_PWM:
		break;
	case TIMER_STATUS_USED_AS_GPT:
		__handle_gpt_interrupt(ins,thw);
		break;
	case TIMER_STATUS_USED_AS_CLK:
		break;
	case TIMER_STATUS_USED_AS_PCAPT:
		break;
	}
}
