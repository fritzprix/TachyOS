# Created on: 2014. 12. 9.
#     Author: innocentevil


BOARD_BUILD_DIR=$(GEN_DIR)/board
GEN_SUB_DIR+=$(BOARD_BUILD_DIR)

BOARD_SRCS=tch_boardcfg.c
BOARD_OBJS=$(BOARD_SRCS:%.c=$(BOARD_BUILD_DIR)/%.o)


OBJS += $(BOARD_OBJS)


$(BOARD_BUILD_DIR):
	$(MKDIR) $(BOARD_BUILD_DIR)


$(BOARD_BUILD_DIR)/%.o:$(HAL_SRC_DIR)/board/$(BOARD_NAME)/%.c 
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
