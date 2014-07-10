PORT_SRCS=tch_port.c
PORT_OBJS=$(PORT_SRC.%c=.%o)



ifeq($(CPU),cortex-m4)
	CFLAG += -mfpu=fpv4-sp-d16
ifeq ($(FPU),HARD)
	CFLAG += -mfloat-abi=hard
endif
ifeq ($(FPU),SOFT)
	## Soft Float 
endif
ifeq ($(FPU),NO)
	## No Floating Point
endif
endif

$(PORT_OBJS):$(PORT_SRCS)
	$(CC) $(CFLAG) $(INC) -o $@

OBJS += $(PORT_OBJS)

clean:
	rm -rf $(PORT_OBJS)

