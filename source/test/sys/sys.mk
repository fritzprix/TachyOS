SYS_TEST_SRC = mailq_test.c\
               mpool_test.c\
               msgq_test.c\
               mtx_test.c\
               semaphore_test.c\
               bar_test.c\
               monitor_test.c
              
SYS_TEST_BUILD_DIR=$(GEN_DIR)/test/sys
GEN_SUB_DIR+=$(SYS_TEST_BUILD_DIR) 
SYS_TEST_OBJS=$(SYS_TEST_SRC:%.c=$(SYS_TEST_BUILD_DIR)/%.o)
SYS_TEST_CPPOBJS=

INC += -I$(TEST_HDR_BASE_DIR)/sys
OBJS += $(SYS_TEST_OBJS)
OBJS += $(SYS_TEST_CPPOBJS)


$(SYS_TEST_BUILD_DIR):$(TEST_BUILD_BASE_DIR)
	$(MK) $(SYS_TEST_BUILD_DIR)



$(SYS_TEST_BUILD_DIR)/%.o:$(TEST_SRC_BASE)/sys/%.c 
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
	
$(SYS_TEST_BUILD_DIR)/%.o:$(TEST_SRC_BASE)/sys/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '
	