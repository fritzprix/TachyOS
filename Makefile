

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################


FLOAT_OPTION= 
LIBS=
LIB_DIR=
CFLAG =
CPFLAG =  
LDFLAG =
TOOL_PREFIX=

# C Compiler preprocessor option
include tchConfig.mk


# Tachyos Src Tree Structure
ROOT_DIR= $(CURDIR)
ifeq ($(MK),)
	MK=mkdir
endif


# target directory to put binary file 
ifeq ($(GEN_DIR),)
	GEN_DIR=$(ROOT_DIR)/$(BUILD)
endif

ifeq ($(GEN_SUB_DIR),)
	GEN_SUB_DIR=
endif


#######################################################################################
###################     TachyOS source tree declaration    ############################
#######################################################################################

#source code directory 
KERNEL_UTIL_SRC_DIR=$(ROOT_DIR)/source/kernel/util
KERNEL_MM_SRC_DIR=$(ROOT_DIR)/source/kernel/mm
KERNEL_SRC_DIR=$(ROOT_DIR)/source/kernel
PORT_SRC_DIR=$(ROOT_DIR)/source/arch
HAL_SRC_DIR=$(ROOT_DIR)/source/hal/$(HW_VENDOR)/$(HW_PLF)
BOARD_SRC_DIR=$(ROOT_DIR)/source/board/$(BOARD_NAME)
USR_SRC_DIR=$(ROOT_DIR)/source/usr
UTEST_SRC_BASE=$(ROOT_DIR)/source/test

#header file directory
BASE_HEADER_DIR=$(ROOT_DIR)/include
KERNEL_UTIL_HEADER_DIR=$(ROOT_DIR)/include/kernel/util
KERNEL_HEADER_DIR=$(ROOT_DIR)/include/kernel
PORT_ARCH_COMMON_HEADER_DIR=$(ROOT_DIR)/include/arch/$(ARCH)
PORT_ARCH_HEADER_DIR=$(PORT_ARCH_COMMON_HEADER_DIR)/$(CPU)
HAL_COMMON_HEADER_DIR=$(ROOT_DIR)/include/hal
HAL_VENDOR_HEADER_DIR=$(ROOT_DIR)/include/hal/$(HW_VENDOR)/$(HW_PLF)
BOARD_HEADER_DIR=$(ROOT_DIR)/include/board
UTEST_HEADER_DIR=$(ROOT_DIR)/include/test
USR_HEADER_DIR=$(ROOT_DIR)/include/usr

#######################################################################################
##################          initialize build option       #############################
#######################################################################################
TARGET=$(GEN_DIR)/tachyos_Ver$(MAJOR_VER).$(MINOR_VER).elf
TIME_STAMP=$(shell date +%s)
TIME_FLAG=__BUILD_TIME_EPOCH=$(TIME_STAMP)UL


ifeq ($(LDSCRIPT),)
LDSCRIPT=$(HAL_SRC_DIR)/ld/flash.ld
endif

###################      TachyOS default header inclusion  ############################
ifeq ($(INC),)
	INC = -I$(PORT_ARCH_HEADER_DIR)\
	      -I$(PORT_ARCH_COMMON_HEADER_DIR)\
	      -I$(HAL_VENDOR_HEADER_DIR)\
	      -I$(HAL_COMMON_HEADER_DIR)\
	      -I$(KERNEL_HEADER_DIR)\
	      -I$(BASE_HEADER_DIR)\
	      -I$(BOARD_HEADER_DIR)\
	      -I$(KERNEL_UTIL_HEADER_DIR)
endif

###################      toolchain & architecture specific makefile   #################
include $(PORT_SRC_DIR)/$(ARCH)/toolchain/$(TOOLCHAIN_NAME)/tool.mk


#######################################################################################
####################  Toolchain Independent section of Makefile  ######################
#######################################################################################

CFLAG+= $(FLOAT_OPTION)	$(DBG_OPTION) -D$(HW_PLF) -D$(TIME_FLAG) -DSIMPLIFIED_SYSCALL
CPFLAG+=$(FLOAT_OPTION)	$(DBG_OPTION) -D$(HW_PLF)

include $(PORT_SRC_DIR)/$(ARCH)/$(CPU)/port.mk
include $(KERNEL_UTIL_SRC_DIR)/kutil.mk
include $(KERNEL_MM_SRC_DIR)/mm.mk
include $(HAL_SRC_DIR)/hal.mk
include $(KERNEL_SRC_DIR)/kernel.mk
include $(USR_SRC_DIR)/usr.mk
include $(BOARD_SRC_DIR)/bd.mk
include $(UTEST_SRC_BASE)/tst.mk

TARGET_SIZE = $(TARGET:%.elf=%.siz)
TARGET_FLASH = $(TARGET:%.elf=%.hex) 
TARGET_BINARY = $(TARGET:%.elf=%.bin)

all : $(GEN_DIR) $(GEN_SUB_DIR) $(TARGET) $(TARGET_FLASH) $(TARGET_BINARY) $(TARGET_SIZE)



$(GEN_DIR): 
	$(MK) $(GEN_DIR)

$(TARGET): $(OBJS) $(DEPS)
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
	$(SIZEPRINT) --format=berkeley $<
	@echo 'Finished building: $@'
	@echo ' '
	
	
clean:
	rm -rf $(OBJS) $(TARGET) $(TARGET_FLASH) $(TARGET_SIZE) $(TARGET_BINARY) $(GEN_SUB_DIR)
