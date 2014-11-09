SYS_TEST_SRC = mailq_test.c\
               mpool_test.c\
               msgq_test.c\
               mtx_test.c\
               semaphore_test.c\
               bar_test.c\
               monitor_test.c
               
SYS_TEST_OBJS=$(SYS_TEST_SRC:%.c=$(GEN_DIR)/test/sys/%.o)
SYS_TEST_CPPOBJS=

OBJS += $(SYS_TEST_OBJS)
OBJS += $(SYS_TEST_CPPOBJS)



$(GEN_DIR)/test/sys/%.o:$(TEST_SYS_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
	
$(GEN_DIR)/test/sys/%.o:$(TEST_SYS_SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '
	