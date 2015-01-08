

APP_BUILD_DIR=$(GEN_DIR)/app
GEN_SUB_DIR+=$(APP_BUILD_DIR)
APP_SRCS=app.c
APP_OBJS=$(APP_SRCS:%.c=$(APP_BUILD_DIR)/%.o)
OBJS += $(APP_OBJS)


$(APP_BUILD_DIR):
	$(MK) $(APP_BUILD_DIR)

$(APP_BUILD_DIR)/%.o:$(APP_SRC_DIR)/%.c $(APP_BUILD_DIR)
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
$(APP_BUILD_DIR)/%.o:$(APP_SRC_DIR)/%.cpp $(APP_BUILD_DIR)
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '