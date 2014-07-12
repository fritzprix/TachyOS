
include $(CURDIR)/

HAL_SRCS=\
	 system_stm32f4xx.c\
	 tch_adc.c\
	 tch_dma.c\
	 tch_gpio.c\
	 tch_hal.c\
	 tch_i2c.c\
	 tch_rtc.c\
	 tch_spi.c\
	 tch_timer.c\
	 tch_usart.c
	
HAL_ASM_SRCS=\
     startup_stm32f40xx.S

HAL_ASM_OBJS=$(HAL_ASM_SRC:%.S=%.o)
HAL_OBJS=$(HAL_SRCS:%.c=%.o)


OBJS += $(HAL_ASM_OBJS)
OBJS += $(HAL_OBJS)




