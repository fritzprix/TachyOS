

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


LIBS=-L'C:\Program Files\GNU Tools ARM Embedded\4.8 2014q2\arm-none-eabi\lib\armv7-m\'
LIB_DIR=-lc_s\
        -lg_s

CFLAG+=\
       -D$(HW_PLF)\
       -D__NEWLIB__\
       -mcpu=$(CPU)\
       -m$(INSTR)

CPFLAG+=\
       -D$(HW_PLF)\
       -mcpu=$(CPU)\
       -m$(INSTR)
       
ifneq ($(LIBS),)
	CFLAG+=--specs=nano.specs
endif

MMAP_FLAG = -Wl,-Map,$(TARGET:%.elf=%.map)

TARGET_SIZE = $(TARGET:%.elf=%.siz)
TARGET_FLASH = $(TARGET:%.elf=%.hex) 
TARGET_BINARY = $(TARGET:%.elf=%.bin)

all : $(TARGET) $(TARGET_FLASH) $(TARGET_SIZE) $(TARGET_BINARY)

$(TARGET): $(OBJS)
	@echo "Generating ELF"
	$(CC) -o $@ $(CFLAG) $(LDFLAG) $(MMAP_FLAG) $(LIB_DIR) $(LIBS)  $(INC) $(OBJS)
	@echo ' '

$(TARGET_FLASH): $(TARGET)
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	$(OBJCP) -O ihex $<  $@
	@echo 'Finished building: $@'
	@echo ' '
	
$(TARGET_BINARY): $(TARGET)
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	$(OBJCP) -O binary -S $<  $@
	@echo 'Finished building: $@'
	@echo ' '

$(TARGET_SIZE): $(TARGET)
	@echo 'Invoking: Cross ARM GNU Print Size'
	$(SIZEPrt) --format=berkeley $<
	@echo 'Finished building: $@'
	@echo 

clean:
	rm -rf $(OBJS) $(TARGET) $(TARGET_FLASH) $(TARGET_SIZE) $(TARGET_BINARY) 
