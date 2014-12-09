# Created on: 2014. 12. 9.
#     Author: innocentevil



BOARD_SRCS=tch_board.c
BOARD_OBJS=$(BOARD_SRCS:%.c=$(GEN_DIR)/board/%.o)


OBJS += $(BOARD_OBJS)


$(GEN_DIR)/board/%.o:$(BOARD_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	$(CC) $< -c $(CFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '
           
$(GEN_DIR)/board/%.o:$(BOARD_SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM g++'
	$(CPP) $< -c $(CPFLAG) $(LDFLAG) $(INC) -o $@
	@echo 'Finishing building: $<'
	@echo ' '