

PORT_BUILD_DIR=$(GEN_DIR)/arch
GEN_SUB_DIR+=$(PORT_BUILD_DIR)
PORT_SRCS=tch_port.c
PORT_OBJS=$(PORT_SRCS:%.c=$(PORT_BUILD_DIR)/%.o)

OBJS += $(PORT_OBJS)

$(PORT_BUILD_DIR):
	$(MKDIR) $(PORT_BUILD_DIR)

$(PORT_BUILD_DIR)/%.o:$(ARCH_SRC_DIR)/$(ARCH)/$(CPU)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
