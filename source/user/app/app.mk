

APP_BUILD_DIR=$(GEN_DIR)/app
GEN_SUB_DIR+=$(APP_BUILD_DIR)
APP_SRCS=app.c
APP_OBJS=$(APP_SRCS:%.c=$(APP_BUILD_DIR)/%.o)
OBJS += $(APP_OBJS)


$(APP_BUILD_DIR):
	$(MKDIR) $(APP_BUILD_DIR)

$(APP_BUILD_DIR)/%.o:$(USR_SRC_DIR)/app/$(APP_NAME)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
$(APP_BUILD_DIR)/%.o:$(USR_SRC_DIR)/app/$(APP_NAME)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '