
PORT_SRCS=tch_port.c
PORT_OBJS=$(PORT_SRCS:%.c=$(GEN_DIR)/port/%.o)

include $(CURDIR)/COMMON.mk

ifeq ($(CPU),cortex-m4)
	CFLAG += -mfpu=fpv4-sp-d16
endif
ifeq ($(FPU),HARD)
	CFLAG += -mfloat-abi=hard
endif
ifeq ($(FPU),SOFT)
	## Soft Float 
endif
ifeq ($(FPU),NO)
	## No Floating Point
endif

OBJS += $(PORT_OBJS)


$(GEN_DIR)/port/%.o:$(PORT_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compilere'
	$(CC) $< -c $(CFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
