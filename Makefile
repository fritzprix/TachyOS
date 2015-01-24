

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

ifeq ($(DEPS),)
	DEPS=
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

# kernel source directory
KERNEL_SRC_DIR=$(ROOT_DIR)/source/kernel
# port source dircetory
PORT_SRC_DIR=$(ROOT_DIR)/source/port
# hal source directory
HAL_SRC_DIR=$(ROOT_DIR)/source/hal/$(HW_VENDOR)/$(HW_PLF)
# board source directory
BOARD_SRC_DIR=$(ROOT_DIR)/source/board/$(BOARD_NAME)
# application source directory
USR_SRC_DIR=$(ROOT_DIR)/source/usr
# unit test source directory
UTEST_SRC_BASE=$(ROOT_DIR)/source/test


BASE_HEADER_DIR=$(ROOT_DIR)/include
KERNEL_HEADER_DIR=$(ROOT_DIR)/include/kernel
PORT_ARCH_COMMON_HEADER_DIR=$(ROOT_DIR)/include/port/$(ARCH)
PORT_ARCH_HEADER_DIR=$(PORT_ARCH_COMMON_HEADER_DIR)/$(CPU)
HAL_COMMON_HEADER_DIR=$(ROOT_DIR)/include/hal
HAL_VENDOR_HEADER_DIR=$(ROOT_DIR)/include/hal/$(HW_VENDOR)/$(HW_PLF)
BOARD_HEADER_DIR=$(ROOT_DIR)/include/board/$(BOARD_NAME)
UTEST_HEADER_DIR=$(ROOT_DIR)/include/test
USR_HEADER_DIR=$(ROOT_DIR)/include/usr

#######################################################################################
##################           default build option        #############################
#######################################################################################

ifeq ($(LDSCRIPT),)
LDSCRIPT=$(HAL_VENDOR_HEADER_DIR)/ld/flash.ld
endif

###################      TachyOS default header inclusion  ############################
ifeq ($(INC),)
	INC = -I$(PORT_ARCH_HEADER_DIR)\
	      -I$(PORT_ARCH_COMMON_HEADER_DIR)\
	      -I$(HAL_VENDOR_HEADER_DIR)\
	      -I$(HAL_COMMON_HEADER_DIR)\
	      -I$(KERNEL_HEADER_DIR)\
	      -I$(BASE_HEADER_DIR)\
	      -I$(BOARD_HEADER_DIR)
endif

###################      build option initialization       ############################

ifeq ($(BUILD),Release)
	OPT_FLAG=-O2 -g0
	DBG_OPTION=
else
	OPT_FLAG=-O0 -g3
	DBG_OPTION=-D__DBG
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
		   --specs=nano.specs\
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

ifeq ($(FLOAT_OPTION),)
	FLOAT_OPTION= 
endif

ifeq ($(LIBS),)
	LIBS=
endif

ifeq ($(LIB_DIR),)
	LIB_DIR=
endif


#######################################################################################
###############          Tool-chain configuration inclusion       #####################
#######################################################################################
include $(ROOT_DIR)/source/port/$(ARCH)/toolchain/$(TOOLCHAIN_NAME)/tool.mk


#######################################################################################
###############           default tool-chain configuration        #####################
#######################################################################################
ifeq ($(TOOL_PREFIX),)
TOOL_PREFIX=arm-none-eabi-
endif

CC=$(TOOL_PREFIX)gcc
CPP=$(TOOL_PREFIX)g++
OBJCP=$(TOOL_PREFIX)objcopy
SIZEPRINT=$(TOOL_PREFIX)size



TARGET=$(GEN_DIR)/tachyos_Ver$(MAJOR_VER).$(MINOR_VER).elf
TIME_STAMP=$(shell date +%s)
TIME_FLAG=__BUILD_TIME_EPOCH=$(TIME_STAMP)UL

CFLAG+= $(FLOAT_OPTION)	$(DBG_OPTION) -D$(HW_PLF)\
       -D$(TIME_FLAG)\
       -D__NEWLIB__\
       -D_REENT_SMALL\
       -D__DYNAMIC_REENT__\
       -mcpu=$(CPU)\
       -m$(INSTR)

CPFLAG+=$(FLOAT_OPTION)	$(DBG_OPTION) -D$(HW_PLF)\
       -mcpu=$(CPU)\
       -m$(INSTR)
       


include $(PORT_SRC_DIR)/$(ARCH)/$(CPU)/port.mk
include $(HAL_SRC_DIR)/hal.mk
include $(KERNEL_SRC_DIR)/kernel.mk
include $(USR_SRC_DIR)/usr.mk
include $(BOARD_SRC_DIR)/bd.mk
include $(UTEST_SRC_BASE)/tst.mk


MMAP_FLAG = -Wl,-Map,$(TARGET:%.elf=%.map)

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
	@echo 

clean:
	rm -rf $(OBJS) $(TARGET) $(TARGET_FLASH) $(TARGET_SIZE) $(TARGET_BINARY)
	rmdir $(GEN_SUB_DIR) 
