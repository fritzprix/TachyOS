include $(CURDIR)/COMMON.mk

KERNEL_SRCS=\
	    tch_ipc.c\
	    tch_mpool.c\
	    tch_mtx.c\
	    tch_sched.c\
	    tch_sig.c\
	    tch_sys.c\
	    tch_thread.c\
	    tch_vtimer.c

KERNEL_OBJS=$(KERNEL_SRCS:%.c=$(GEN_DIR)/sys/%.o)
           
OBJS += $(KERNEL_OBJS)


$(GEN_DIR)/sys/%.o:$(KERNEL_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '