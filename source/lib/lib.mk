


##include $(CURDIR)/COMMON.mk
LIB_SRCS= tch_clibsysc.c
           
LIB_OBJS=$(LIB_SRCS:%.c=$(GEN_DIR)/lib/%.o)

OBJS += $(LIB_OBJS)


$(GEN_DIR)/lib/%.o:$(LIB_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '