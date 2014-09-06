##include $(CURDIR)/COMMON.mk

KERNEL_SRCS=\
	    tch_mpool.c\
	    tch_mtx.c\
	    tch_sched.c\
	    tch_sig.c\
	    tch_sys.c\
	    tch_sem.c\
	    tch_thread.c\
	    tch_vtimer.c\
	    tch_list.c\
	    tch_mem.c\
	    tch_clibsysc.c\
	    tch_nclib.c\
	    tch_async.c\
	    tch_bar.c\
	    tch_condv.c\
	    tch_msgq.c\
	    tch_mailq.c
	    
KERNEL_CPP_SRCS=tch_crtb.cpp

KERNEL_OBJS=$(KERNEL_SRCS:%.c=$(GEN_DIR)/sys/%.o)
KERNEL_CPPOBJS=$(KERNEL_CPP_SRCS:%.cpp=$(GEN_DIR)/sys/%.o)
           
OBJS += $(KERNEL_OBJS)
OBJS += $(KERNEL_CPPOBJS)


$(GEN_DIR)/sys/%.o:$(KERNEL_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
	
$(GEN_DIR)/sys/%.o:$(KERNEL_SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '
	