
include $(CURDIR)/COMMON.mk
.SUFFIXES : .c .o
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
     
ASM_OPT=-x assembler-with-cpp\
        -nostdinc

HAL_ASM_OBJS=$(HAL_ASM_SRCS:%.S=$(GEN_DIR)/hal/%.o)
HAL_OBJS=$(HAL_SRCS:%.c=$(GEN_DIR)/hal/%.o)


OBJS += $(HAL_ASM_OBJS)
OBJS += $(HAL_OBJS) 



$(GEN_DIR)/hal/%.o: $(HAL_SRC_DIR)/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	$(CC) $< -c $(CFLAG) $(INC) $(ASM_OPT) -o $@
	@echo 'Finished building: $<'
	@echo ' '



$(GEN_DIR)/hal/%.o: $(HAL_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '







