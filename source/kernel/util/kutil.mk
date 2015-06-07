
KERNEL_UTIL_SRCS=\
	    cdsl_bstree.c\
	    cdsl_dlist.c\
	    cdsl_heap.c\
	    cdsl_rbtree.c\
	    cdsl_slist.c\
	    cdsl_spltree.c\
	    tch_mem.c
	    
KERNEL_UTIL_CPP_SRCS=
KERNEL_UTIL_BUILD_DIR=$(GEN_DIR)/kutil
GEN_SUB_DIR+=$(KERNEL_UTIL_BUILD_DIR) 
KERNEL_UTIL_OBJS=$(KERNEL_UTIL_SRCS:%.c=$(KERNEL_UTIL_BUILD_DIR)/%.o)
KERNEL_UTIL_CPPOBJS=$(KERNEL_UTIL_CPP_SRCS:%.cpp=$(KERNEL_UTIL_BUILD_DIR)/%.o)

           
OBJS += $(KERNEL_UTIL_OBJS)
OBJS += $(KERNEL_UTIL_CPPOBJS)

$(KERNEL_UTIL_BUILD_DIR):
	$(MK) $(KERNEL_UTIL_BUILD_DIR)


$(KERNEL_UTIL_BUILD_DIR)/%.o:$(KERNEL_UTIL_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
	
$(KERNEL_UTIL_BUILD_DIR)/%.o:$(KERNEL_UTIL_SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '
	