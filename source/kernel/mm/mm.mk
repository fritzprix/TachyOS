
KERNEL_MM_SRCS=\
	    tch_kmem.c\
	    tch_palloc.c
	    
KERNEL_MM_CPP_SRCS=
KERNEL_MM_BUILD_DIR=$(GEN_DIR)/mm
GEN_SUB_DIR+=$(KERNEL_MM_BUILD_DIR) 
KERNEL_MM_OBJS=$(KERNEL_MM_SRCS:%.c=$(KERNEL_MM_BUILD_DIR)/%.o)
KERNEL_MM_CPPOBJS=$(KERNEL_MM_CPP_SRCS:%.cpp=$(KERNEL_MM_BUILD_DIR)/%.o)

           
OBJS += $(KERNEL_MM_OBJS)
OBJS += $(KERNEL_MM_CPPOBJS)

$(KERNEL_MM_BUILD_DIR):
	$(MK) $(KERNEL_MM_BUILD_DIR)


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
	