

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################

# C Compiler preprocessor option
include COMMON.mk


TARGET=$(GEN_DIR)/tachyos_Ver$(MAJOR_VER).$(MINOR_VER).elf
TIME_STAMP=$(shell date +%s)
TIME_FLAG=__BUILD_TIME_EPOCH=$(TIME_STAMP)UL

LIBS=-lc_nano\
     -lg_nano\
     -lstdc++_nano\
    
LIB_DIR=    

CFLAG+=\
       -D$(HW_PLF)\
       -D$(TIME_FLAG)\
       -D__NEWLIB__\
       -D_REENT_SMALL\
       -D__DYNAMIC_REENT__\
       -mcpu=$(CPU)\
       -m$(INSTR)

CPFLAG+=\
       -D$(HW_PLF)\
       -mcpu=$(CPU)\
       -m$(INSTR)
       
ifneq ($(LIBS),)
	CFLAG+=--specs=nano.specs
endif

DBG_FLAG=-D__USE_MALLOC
CPFLAG+=$(DBG_FLAG)
CFLAG+=$(DBG_FLAG)


include $(PORT_SRC_DIR)/port.mk
include $(HAL_SRC_DIR)/hal.mk
include $(KERNEL_SRC_DIR)/kernel.mk
include $(USR_SRC_DIR)/usr.mk
include $(BOARD_SRC_DIR)/bd.mk
include $(TEST_SYS_SRC_DIR)/sys.mk
include $(TEST_HAL_SRC_DIR)/haltst.mk



MMAP_FLAG = -Wl,-Map,$(TARGET:%.elf=%.map)

TARGET_SIZE = $(TARGET:%.elf=%.siz)
TARGET_FLASH = $(TARGET:%.elf=%.hex) 
TARGET_BINARY = $(TARGET:%.elf=%.bin)

all : $(TARGET) $(TARGET_FLASH) $(TARGET_BINARY) $(TARGET_SIZE)

$(TARGET): $(OBJS)
	@echo "Generating ELF"
	$(CPP) -o $@ $(CFLAG) $(LDFLAG) $(MMAP_FLAG) $(LIB_DIR) $(LIBS)  $(INC) $(OBJS)
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
