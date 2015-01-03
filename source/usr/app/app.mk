
APP_SRCS=app.c
APP_OBJS=$(APP_SRCS:%.c=$(GEN_DIR)/usr/app/%.o)


OBJS += $(APP_OBJS)


$(GEN_DIR)/usr/app/%.o:$(APP_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
$(GEN_DIR)/usr/app/%.o:$(APP_SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '