
KERNEL_UTIL_SRCS=\
	    cdsl_bstree.c\
	    cdsl_dlist.c\
	    cdsl_heap.c\
	    cdsl_rbtree.c\
	    cdsl_slist.c\
	    cdsl_spltree.c\
	    string.c
	    
	    
KERNEL_UTIL_BUILD_DIR=$(GEN_DIR)/kutil
GEN_SUB_DIR+=$(KERNEL_UTIL_BUILD_DIR) 
KERNEL_UTIL_OBJS=$(KERNEL_UTIL_SRCS:%.c=$(KERNEL_UTIL_BUILD_DIR)/%.o)


INC += -I$(KERNEL_HEADER_DIR)/util
OBJS += $(KERNEL_UTIL_OBJS)
OBJS += $(KERNEL_UTIL_CPPOBJS)

$(KERNEL_UTIL_BUILD_DIR):
	$(MKDIR) $(KERNEL_UTIL_BUILD_DIR)


$(KERNEL_UTIL_BUILD_DIR)/%.o:$(KERNEL_SRC_DIR)/util/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
