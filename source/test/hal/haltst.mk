HAL_TEST_SRC = gpio_test.c\
               uart_test.c\
               timer_test.c\
               spi_test.c
               
HAL_TEST_BUILD_DIR=$(GEN_DIR)/test/hal
GEN_SUB_DIR+=$(HAL_TEST_BUILD_DIR)          
HAL_TEST_OBJS=$(HAL_TEST_SRC:%.c=$(HAL_TEST_BUILD_DIR)/%.o)
HAL_TEST_CPPOBJS=



INC += -I$(TEST_HDR_BASE_DIR)/hal
OBJS += $(HAL_TEST_OBJS)
OBJS += $(HAL_TEST_CPPOBJS)


$(HAL_TEST_BUILD_DIR):$(TEST_BUILD_BASE_DIR)
	$(MK) $(HAL_TEST_BUILD_DIR)

$(HAL_TEST_BUILD_DIR)/%.o:$(TEST_SRC_BASE)/hal/%.c 
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
	
$(HAL_TEST_BUILD_DIR)/%.o:$(TEST_SRC_BASE)/hal/%.cpp 
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '
	