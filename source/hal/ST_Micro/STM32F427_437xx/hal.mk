
HAL_SRCS=\
	tch_adc.c\
	tch_dma.c\
	tch_gpio.c\
	tch_hal.c\
	tch_i2c.c\
	tch_rtc.c\
	tch_spi.c\
	tch_timer.c\
	tch_usart.c\
	tch_boot.c
	
HAL_ASM_SRCS=\
     startup_stm32f40xx.S
     
ASM_OPT=-x assembler-with-cpp\
        -nostdinc

HAL_BUILD_DIR=$(GEN_DIR)/platform
GEN_SUB_DIR+=$(HAL_BUILD_DIR)

HAL_ASM_OBJS=$(HAL_ASM_SRCS:%.S=$(HAL_BUILD_DIR)/%.o)
HAL_OBJS=$(HAL_SRCS:%.c=$(HAL_BUILD_DIR)/%.o)


OBJS += $(HAL_OBJS) 
OBJS += $(HAL_ASM_OBJS)


$(HAL_BUILD_DIR): 
	$(MK) $(HAL_BUILD_DIR)

$(HAL_BUILD_DIR)/%.o: $(HAL_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '

$(HAL_BUILD_DIR)/%.o: $(HAL_SRC_DIR)/%.S 
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	$(CC) $< -c $(CFLAG) $(INC) $(ASM_OPT) -o $@
	@echo 'Finished building: $<'
	@echo ' '






