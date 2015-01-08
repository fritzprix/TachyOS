# Created on: 2014. 12. 9.
#     Author: innocentevil


BOARD_BUILD_DIR=$(GEN_DIR)/board
GEN_SUB_DIR+=$(BOARD_BUILD_DIR)

BOARD_SRCS=tch_board.c
BOARD_OBJS=$(BOARD_SRCS:%.c=$(BOARD_BUILD_DIR)/%.o)


OBJS += $(BOARD_OBJS)


$(BOARD_BUILD_DIR):
	$(MK) $(BOARD_BUILD_DIR)


$(BOARD_BUILD_DIR)/%.o:$(BOARD_SRC_DIR)/%.c $(BOARD_BUILD_DIR)
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
$(BOARD_BUILD_DIR)/%.o:$(BOARD_SRC_DIR)/%.cpp $(BOARD_BUILD_DIR)
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '