HAL_TEST_SRC = gpio_test.c\
               uart_test.c\
               timer_test.c\
               spi_test.c
               
HAL_TEST_OBJS=$(HAL_TEST_SRC:%.c=$(GEN_DIR)/test/hal/%.o)
HAL_TEST_CPPOBJS=

OBJS += $(HAL_TEST_OBJS)
OBJS += $(HAL_TEST_CPPOBJS)



$(GEN_DIR)/test/hal/%.o:$(TEST_HAL_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
	
$(GEN_DIR)/test/hal/%.o:$(TEST_HAL_SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '
	