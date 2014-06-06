/*
 * main.c
 *
 *  Created on: 2014. 2. 9.
 *      Author: innocentevil
 */

#include "main.h"
#include "core/port/tch_stdtypes.h"
#include "core/fmo_sched.h"
#include "core_cm4.h"
#include "lld/fmo_timer.h"
#include "core/fmo_time.h"
#include "lld/fmo_gpio.h"
#include "core/fmo_task.h"
#include "lld/fmo_adc.h"

#include "lld/fmo_usart.h"
#include "lld/fmo_spi.h"


static uint32_t child_Stack[1 << 8];
static uint32_t child_Stack1[1 << 8];
static mtx_lock tlock;

static tchThread_t* childThread;
static THREAD_ROUTINE(childRoutine);

static tchThread_t* childThread1;
static THREAD_ROUTINE(childRoutine1);

static tch_gpio_instance* ledIo;
static uint16_t led_pin = (uint16_t) (1 << 9);
static uint16_t ev_pin = (uint16_t) (1 << 10);
static tch_sysTask task;
static tch_gptimer_instance* time_evt_handler;

static void btn_eventListener(tch_gpio_instance* gpio);
static DECLARE_TASK_FN(ledTask);

static mtx_lock time_eventLock;
static GPT_TIMEOUT_LISTENER(time_eventListener);
static void postTimeEvent(void);
float a = 1.f;
float b = 2.f;
/**
 * Test Only Purpose
 */

int btn_cnt;
uint16_t ana_val[256];
void* main(void* arg){

	btn_cnt = 0;
	tch_Mtx_init(&tlock);
	task.t_arg = NULL;
	task.t_prior = 1;
	task.t_qnode.next = NULL;
	task.t_qnode.prev = NULL;
	task.t_routin = ledTask;

	tch_Mtx_init(&time_eventLock);
	time_evt_handler = lld_timer_openGPTimer(Timer4,1000,time_eventListener,DeactOnSleep);



	tch_usart_cfg ucfg;
	ucfg.USART_Parity = USART_Parity_NON;
	ucfg.USART_StopBit = USART_StopBit_1B;
	usart3->open(usart3,115200,ActOnSleep,&ucfg);
	usart3->writeCstr(usart3,"USART3 Initialized\n",NULL);

	tch_gpio_cfg io_cfg;
	tch_lld_gpio_cfgInit(&io_cfg);
	io_cfg.GPIO_Mode = GPIO_Mode_OUT;
	io_cfg.GPIO_Otype = GPIO_OType_PP;
	io_cfg.GPIO_PuPd = GPIO_PuPd_UP;
	ledIo = tch_lld_gpio_init(GPIO_F,led_pin,&io_cfg,ActOnSleep);
	usart3->writeCstr(usart3,"LED GPIO Initialized\n",NULL);

	tch_lld_gpio_cfgInit(&io_cfg);
	io_cfg.GPIO_Mode = GPIO_Mode_IN;
	io_cfg.GPIO_Otype = GPIO_OType_OD;
	io_cfg.GPIO_PuPd = GPIO_PuPd_PU;
	tch_gpio_instance* evt_io = tch_lld_gpio_init(GPIO_F,ev_pin,&io_cfg,ActOnSleep);

	tch_gpio_evt_cfg ev_cfg;
	ev_cfg.GPIO_Evt_Edge_Mode = GPIO_Evt_Edge_Fall;
	ev_cfg.GPIO_Evt_Type = GPIO_Evt_Interrupt;

	evt_io->registerIoEvent(evt_io,ev_pin,&ev_cfg,btn_eventListener);


	uint32_t inc = 3;
	uint32_t x = 0;
	childThread = tchThread_create(child_Stack,256,childRoutine,THREAD_PRIORITY_NORMAL,"child1");
	childThread1 = tchThread_create(child_Stack1,256,childRoutine1,THREAD_PRIORITY_NORMAL,"child2");
	tchThread_start(childThread);
	tchThread_sleep(200);
	if(ledIo != NULL){
			ledIo->out(ledIo,led_pin,bSet);
	}
	tchThread_start(childThread1);
	tchThread_sleep(200);
	if(ledIo != NULL){
		ledIo->out(ledIo,led_pin,bClear);
	}
	usart3->writeCstr(usart3,"Device Initailized Complete\n",NULL);

	tch_adc_cfg acfg;
	tch_lld_adc_initCfg(&acfg);
	acfg.ADC_Resolution = ADC_Resolution_12B;
	acfg.ADC_SampleHold = ADC_SampleHold_3Cycle;
	acfg.ADC_SampleFreqInHz = 100000;
	adc1->open(adc1,&acfg,ActOnSleep);
	adc1->read(adc1,ana_val,200,4);
	adc1->close(adc1);


	adc1->open(adc1,&acfg,ActOnSleep);
//	usart3->close(usart3);
//	usart3->open(usart3,115200,ActOnSleep,&ucfg);
	uint8_t c;
	uint16_t av = 0;

	tch_spi_cfg spicfg;
	tch_lld_spi_cfginit(&spicfg);
	spi1->open(spi1,&spicfg,ActOnSleep);
	while(1){
		a += 0.0001f;
		spi1->transceive(spi1,'A',NULL);
		adc1->read(adc1,ana_val,200,10);
//		usart3->putc(usart3,'A');
		usart3->writeCstr(usart3,"ADC Conversion Completed\n",NULL);
//		usart3->write(usart3,"123",3,NULL);

	}
	return 0;
}


void device_Init(void){

}

DECLARE_TASK_FN(ledTask){
	ledIo->out(ledIo,led_pin,bClear);
	usart3->writeCstr(usart3,"Btn Pressed\n",NULL);
	btn_cnt = 0;
}

GPT_TIMEOUT_LISTENER(time_eventListener){
	btn_cnt++;
}

void postTimeEvent(void){
	time_evt_handler->setTimeout(time_evt_handler,2,25);
}

void btn_eventListener(tch_gpio_instance* gpio){
//	btn_cnt++;
	tch_postSysTask(ledTask,NULL,1);
}

THREAD_ROUTINE(childRoutine){
	uint32_t cnt = 0;
	while(1){
		cnt++;
		ledIo->out(ledIo,led_pin,bClear);
		tchThread_sleep(0);
//		usart3->putc(usart3,'B');
		usart3->writeCstr(usart3,"This is Child0 Loop\n",NULL);
//		usart3->write(usart3,"456",3,NULL);

	}
	return NULL;
}

THREAD_ROUTINE(childRoutine1){
	uint32_t cnt = 0;
	while(1){
		cnt++;
//		fv += 0.001f;
		b += 0.001f;
		tchThread_sleep(0);
//		usart3->putc(usart3,'C');
		usart3->writeCstr(usart3,"This is Child1 Loop\n",NULL);
//		usart3->write(usart3,"789",3,NULL);
	}
	return NULL;
}
