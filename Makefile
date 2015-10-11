

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

# Configuration make file
include tchConfig.mk


ROOT_DIR= $(CURDIR)
ifeq ($(MKDIR),)
	MKDIR=mkdir
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
KERNEL_SRC_DIR=$(ROOT_DIR)/source/kernel
ARCH_SRC_DIR=$(ROOT_DIR)/source/arch
HAL_SRC_DIR=$(ROOT_DIR)/source/hal/$(HW_VENDOR)/$(HW_PLF)
USR_SRC_DIR=$(ROOT_DIR)/source/user

#header file directory
HEADER_ROOT_DIR=$(ROOT_DIR)/include
KERNEL_HEADER_DIR=$(HEADER_ROOT_DIR)/kernel
ARCH_BASE_HEADER_DIR=$(HEADER_ROOT_DIR)/arch/$(ARCH)
ARCH_CPU_HEADER_DIR=$(ARCH_BASE_HEADER_DIR)/$(CPU)
HAL_BASE_HEADER_DIR=$(HEADER_ROOT_DIR)/hal
HAL_VENDOR_HEADER_DIR=$(HAL_BASE_HEADER_DIR)/$(HW_VENDOR)
HAL_CHIPSET_HEADER_DIR=$(HAL_VENDOR_HEADER_DIR)/$(HW_PLF)

#######################################################################################
##################          initialize build option       #############################
#######################################################################################
TARGET=$(GEN_DIR)/tachyos_Ver$(MAJOR_VER).$(MINOR_VER).elf
#TIME_STAMP=$(shell date +%s)
TIME_FLAG=__BUILD_TIME_EPOCH=$(shell date +%s)UL


ifeq ($(LDSCRIPT),)
LDSCRIPT=$(HAL_SRC_DIR)/ld/flash.ld
endif

###################      TachyOS default header inclusion  ############################
ifeq ($(INC),)
	INC = -I$(ARCH_CPU_HEADER_DIR)\
	      -I$(ARCH_BASE_HEADER_DIR)\
	      -I$(HAL_BASE_HEADER_DIR)\
	      -I$(HAL_VENDOR_HEADER_DIR)\
	      -I$(HAL_CHIPSET_HEADER_DIR)\
	      -I$(KERNEL_HEADER_DIR)\
	      -I$(HEADER_ROOT_DIR)
endif

###################      toolchain & architecture specific makefile   #################
include $(ARCH_SRC_DIR)/$(ARCH)/toolchain/$(TOOLCHAIN_NAME)/tool.mk


#######################################################################################
####################  Toolchain Independent section of Makefile  ######################
#######################################################################################

CFLAG+= $(FLOAT_OPTION)	$(DBG_OPTION) -D$(HW_PLF) -D$(TIME_FLAG)
CPFLAG+=$(FLOAT_OPTION)	$(DBG_OPTION) -D$(HW_PLF) -D$(TIME_FLAG)


##
#KERNEL_SRC_DIR=$(ROOT_DIR)/source/kernel
#ARCH_SRC_DIR=$(ROOT_DIR)/source/arch
#HAL_SRC_DIR=$(ROOT_DIR)/source/hal/$(HW_VENDOR)/$(HW_PLF)
#USR_SRC_DIR=$(ROOT_DIR)/source/usr
##

include $(ARCH_SRC_DIR)/$(ARCH)/$(CPU)/port.mk
include $(KERNEL_SRC_DIR)/kernel.mk
include $(HAL_SRC_DIR)/hal.mk
include $(USR_SRC_DIR)/usr.mk

TARGET_SIZE = $(TARGET:%.elf=%.siz)
TARGET_FLASH = $(TARGET:%.elf=%.hex) 
TARGET_BINARY = $(TARGET:%.elf=%.bin)

all : $(GEN_DIR) $(GEN_SUB_DIR) $(TARGET) $(TARGET_FLASH) $(TARGET_BINARY) $(TARGET_SIZE)



$(GEN_DIR): 
	$(MKDIR) $(GEN_DIR)

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
	rm -rf $(OBJS) $(TARGET) $(TARGET_FLASH) $(TARGET_SIZE) $(TARGET_BINARY) $(GEN_SUB_DIR) $(GEN_DIR)
