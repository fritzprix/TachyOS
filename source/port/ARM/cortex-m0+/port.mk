SRCS=tch_port.c
OBJS=$(PORT_SRCS:%.c=%.o)



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

%.o:%.c
	$(CC) $(CFLAG) $(INC) $< -o $@

OBJS += $(PORT_OBJS)
