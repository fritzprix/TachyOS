##include $(CURDIR)/COMMON.mk

KERNEL_SRCS=\
		tch_lock.c\
	    tch_mpool.c\
	    tch_sched.c\
	    tch_sys.c\
	    tch_sem.c\
	    tch_thread.c\
	    tch_bar.c\
	    tch_msgq.c\
	    tch_mailq.c\
	    tch_time.c\
	    tch_event.c\
	    tch_kdesc.c\
	    tch_crt0.c\
	    tch_idle.c\
	    tch_lwtask.c\
	    tch_err.c\
	    tch_loader.c\
	    tch_init.c\
	    tch_interrupt.c\
	    tch_kobj.c
	    
	    
KERNEL_CPP_SRCS=tch_crtb.cpp


KERNEL_BUILD_DIR=$(GEN_DIR)/kernel
GEN_SUB_DIR+=$(KERNEL_BUILD_DIR) 
KERNEL_OBJS=$(KERNEL_SRCS:%.c=$(KERNEL_BUILD_DIR)/%.o)
KERNEL_CPPOBJS=$(KERNEL_CPP_SRCS:%.cpp=$(KERNEL_BUILD_DIR)/%.o)

           
OBJS += $(KERNEL_OBJS)
OBJS += $(KERNEL_CPPOBJS)

$(KERNEL_BUILD_DIR):
	$(MK) $(KERNEL_BUILD_DIR)


$(KERNEL_BUILD_DIR)/%.o:$(KERNEL_SRC_DIR)/%.c# $(KERNEL_BUILD_DIR)
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
	
$(KERNEL_BUILD_DIR)/%.o:$(KERNEL_SRC_DIR)/%.cpp# $(KERNEL_BUILD_DIR)
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '
	