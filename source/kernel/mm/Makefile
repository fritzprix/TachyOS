
KERNEL_MM_SRCS=\
	    wtree.c\
	    wtmalloc.c\
	    tch_kmalloc.c\
	    tch_malloc.c\
	    tch_shm.c
	    
	    
KERNEL_MM_SRC_DIR=$(KERNEL_SRC_DIR)/mm
KERNEL_MM_INC_DIR=$(ROOT_DIR)/include/kernel/mm
KERNEL_MM_BUILD_DIR=$(GEN_DIR)/mm
KERNEL_MM_OBJS=$(KERNEL_MM_SRCS:%.c=$(KERNEL_MM_BUILD_DIR)/%.o)
KERNEL_MM_CPPOBJS=$(KERNEL_MM_CPP_SRCS:%.cpp=$(KERNEL_MM_BUILD_DIR)/%.o)
GEN_SUB_DIR +=$(KERNEL_MM_BUILD_DIR) 
INC += -I$(KERNEL_MM_INC_DIR)
OBJS += $(KERNEL_MM_OBJS)
OBJS += $(KERNEL_MM_CPPOBJS)

include $(KERNEL_MM_SRC_DIR)/$(CONFIG_MMU)/Makefile


$(KERNEL_MM_BUILD_DIR):
	$(MKDIR) $(KERNEL_MM_BUILD_DIR)


$(KERNEL_MM_BUILD_DIR)/%.o:$(KERNEL_MM_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
	
$(KERNEL_MM_BUILD_DIR)/%.o:$(KERNEL_MM_SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '
	