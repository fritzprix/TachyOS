

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################

# C Compiler preprocessor option
include tchConfig.mk


# Tachyos Src Tree Structure
ROOT_DIR= $(CURDIR)
ifeq ($(MK),)
	MK=mkdir
endif

ifeq ($(GEN_DIR),)
	GEN_DIR=$(ROOT_DIR)/$(BUILD)
endif

KERNEL_SRC_DIR=$(ROOT_DIR)/source/kernel
PORT_SRC_DIR=$(ROOT_DIR)/source/port/$(ARCH)/$(CPU)
HAL_SRC_DIR=$(ROOT_DIR)/source/hal/$(HW_VENDOR)/$(HW_PLF)
BOARD_SRC_DIR=$(ROOT_DIR)/source/board/$(BOARD_NAME)
TEST_SRC_BASE=$(ROOT_DIR)/source/test
TEST_SYS_SRC_DIR=$(ROOT_DIR)/source/test/sys
TEST_HAL_SRC_DIR=$(ROOT_DIR)/source/test/hal
USR_SRC_DIR=$(ROOT_DIR)/source/usr
APP_SRC_DIR=$(USR_SRC_DIR)/app/$(APP_NAME)
MODULE_BASE_SRC_DIR=$(ROOT_DIR)/source/usr/module

TCH_API_HDR_DIR=$(ROOT_DIR)/include
KERNEL_HDR_DIR=$(ROOT_DIR)/include/kernel
PORT_COMMON_HDR_DIR=$(ROOT_DIR)/include/port/$(ARCH)
PORT_ARCH_HDR_DIR=$(PORT_COMMON_HDR_DIR)/$(CPU)
HAL_VND_HDR_DIR=$(ROOT_DIR)/include/hal/$(HW_VENDOR)/$(HW_PLF)
BOARD_HDR_DIR=$(ROOT_DIR)/include/board/$(BOARD_NAME)
HAL_COMMON_HDR_DIR=$(ROOT_DIR)/include/hal
TEST_SYS_HDR_DIR=$(ROOT_DIR)/include/test/sys
TEST_HAL_HDR_DIR=$(ROOT_DIR)/include/test/hal
USR_HDR_DIR=$(ROOT_DIR)/include/usr
APP_HDR_DIR=$(USR_HDR_DIR)/app/$(APP_NAME)
MODULE_HDR_DIR=$(USR_HDR_DIR)/module

ifeq ($(GEN_SUB_DIR),)
GEN_SUB_DIR=
endif



LDSCRIPT=$(HAL_VND_HDR_DIR)/ld/flash.ld
TOOL_CHAIN=arm-none-eabi-
CC=$(TOOL_CHAIN)gcc
CPP=$(TOOL_CHAIN)g++
OBJCP=$(TOOL_CHAIN)objcopy
SIZEPrt=$(TOOL_CHAIN)size

ifeq ($(INC),)
	INC = -I$(PORT_ARCH_HDR_DIR)\
	      -I$(PORT_COMMON_HDR_DIR)\
	      -I$(HAL_VND_HDR_DIR)\
	      -I$(HAL_COMMON_HDR_DIR)\
	      -I$(KERNEL_HDR_DIR)\
	      -I$(TCH_API_HDR_DIR)\
	      -I$(BOARD_HDR_DIR)\
	      -I$(USR_HDR_DIR)\
	      -I$(TEST_SYS_HDR_DIR)\
	      -I$(TEST_HAL_HDR_DIR)
endif




ifeq ($(LDFLAG),)
	LDFLAG=
endif

ifeq ($(CFLAG),)
	CFLAG =-fsigned-char\
		   -ffunction-sections\
		   -fdata-sections\
		   -ffreestanding\
		   -nostartfiles\
		   -Xlinker\
		   --gc-sections\
		   -T$(LDSCRIPT)\
		   $(OPT_FLAG)
endif

ifeq ($(CPFLAG),)
	CPFLAG = -mlong-calls\
	         -ffunction-sections\
	         -ffreestanding\
	         -fno-rtti\
	         -fno-exceptions\
	         -Wall\
	         -fpermissive\
	         -T$(LDSCRIPT)\
	          $(OPT_FLAG)
endif

ifeq ($(OPT_FLAG),)
	OPT_FLAG=
endif

ifeq ($(BUILD),Release)
	OPT_FLAG=-O2 -g3
else
	OPT_FLAG=-O0 -g3
	CFLAG+= -DDBG
	CPFLAG+= -DDBG
endif

ifeq ($(FLOAT_FLAG),)
	FLOAT_FLAG= 
endif

ifeq ($(CPU),cortex-m4)
	FLOAT_FLAG += -mfpu=fpv4-sp-d16
endif
ifeq ($(FPU),HARD)
	FLOAT_FLAG += -mfloat-abi=hard\
	              -DMFEATURE_HFLOAT=1
endif
ifeq ($(FPU),HALFSOFT)
	FLOAT_FLAG += -mfloat-abi=softfp\
	              -DMFEATURE_HFLOAT=1
endif
ifeq ($(FPU),SOFT)
	FLOAT_FLAG += -mfloat-abi=soft
endif


ifneq ($(FLOAT_FLAG),)
	CPFLAG += $(FLOAT_FLAG)
	CFLAG += $(FLOAT_FLAG)
endif



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
include $(TEST_SRC_BASE)/tst.mk


MMAP_FLAG = -Wl,-Map,$(TARGET:%.elf=%.map)

TARGET_SIZE = $(TARGET:%.elf=%.siz)
TARGET_FLASH = $(TARGET:%.elf=%.hex) 
TARGET_BINARY = $(TARGET:%.elf=%.bin)

all : clean $(GEN_DIR) $(TARGET) $(TARGET_FLASH) $(TARGET_BINARY) $(TARGET_SIZE) 



$(GEN_DIR):
	$(MK) $(GEN_DIR)

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
