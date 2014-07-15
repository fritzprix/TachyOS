

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################

# C Compiler preprocessor option
include COMMON.mk


ifeq ($(CPU),cortex-m4)
	CFLAG += -mfpu=fpv4-sp-d16
	CPFLAG += -mfpu=fpv4-sp-d16
endif
ifeq ($(FPU),HARD)
	CFLAG += -mfloat-abi=hard
	CPFLAG += -mfloat-abi=hard
endif
ifeq ($(FPU),SOFT)
	## Soft Float 
endif
ifeq ($(FPU),NO)
	## No Floating Point
endif

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
