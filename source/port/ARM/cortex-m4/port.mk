
PORT_SRCS=tch_port.c
PORT_OBJS=$(PORT_SRCS:%.c=$(GEN_DIR)/port/%.o)


OBJS += $(PORT_OBJS)


$(GEN_DIR)/port/%.o:$(PORT_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
