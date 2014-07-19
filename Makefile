

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################

# C Compiler preprocessor option
include COMMON.mk

include $(PORT_SRC_DIR)/port.mk
include $(HAL_SRC_DIR)/hal.mk
include $(LIB_SRC_DIR)/lib.mk
include $(KERNEL_SRC_DIR)/sys.mk
include $(USR_SRC_DIR)/usr.mk

TARGET=$(GEN_DIR)/tachyos.elf


LIBS=
LIB_DIR=
CFLAG+=\
       -D$(HW_PLF)\
       -mcpu=$(CPU)\
       -m$(INSTR)

CPFLAG+=\
       -D$(HW_PLF)\
       -mcpu=$(CPU)\
       -m$(INSTR)

MMAP_FLAG = -Wl,-Map,$(TARGET:%.elf=%.map)

all : $(TARGET)

$(TARGET): $(OBJS)
	@echo "Generating ELF"
	$(CC) -o $@ $(CFLAG) $(MMAP_FLAG) $(INC) $(OBJS)
	


clean:
	rm -rf $(OBJS) $(TARGET)
