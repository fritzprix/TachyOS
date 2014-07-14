
USR_SRCS=main.c
USR_OBJS=$(USR_SRCS:%.c=$(GEN_DIR)/usr/%.o)

include $(CURDIR)/COMMON.mk

OBJS += $(USR_OBJS)


$(GEN_DIR)/usr/%.o:$(USR_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
$(GEN_DIR)/usr/%.o:$(USR_SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '