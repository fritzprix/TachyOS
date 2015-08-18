
KERNEL_MM_NOMMU_SRCS=\
				tch_mm.c\
				tch_segment.c
	    
KERNEL_MM_NOMMU_SRC_DIR=$(KERNEL_MM_SRC_DIR)/nommu
KERNEL_MM_NOMMU_CPP_SRCS=
KERNEL_MM_NOMMU_BUILD_DIR=$(GEN_DIR)/mm/nommu
KERNEL_MM_NOMMU_OBJS=$(KERNEL_MM_NOMMU_SRCS:%.c=$(KERNEL_MM_NOMMU_BUILD_DIR)/%.o)
KERNEL_MM_NOMMU_CPPOBJS=$(KERNEL_MM_NOMMU_CPP_SRCS:%.cpp=$(KERNEL_MM_NOMMU_BUILD_DIR)/%.o)

GEN_SUB_DIR+=$(KERNEL_MM_NOMMU_BUILD_DIR) 
OBJS += $(KERNEL_MM_NOMMU_OBJS)
OBJS += $(KERNEL_MM_NOMMU_CPPOBJS)

$(KERNEL_MM_NOMMU_BUILD_DIR):
	$(MK) $(KERNEL_MM_NOMMU_BUILD_DIR)


$(KERNEL_MM_NOMMU_BUILD_DIR)/%.o:$(KERNEL_MM_NOMMU_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
	
	
$(KERNEL_MM_NOMMU_BUILD_DIR)/%.o:$(KERNEL_MM_NOMMU_SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '