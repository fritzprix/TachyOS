

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################

# C Compiler preprocessor option

include tchConfig.mk

# Tachyos Src Tree Structure
ROOT_DIR=$(shell pwd)
KERNEL_SRC_DIR=$(ROOT_DIR)/source/kernel
PORT_SRC_DIR=$(ROOT_DIR)/source/port/$(ARCH)/$(CPU)
HAL_SRC_DIR=$(ROOT_DIR)/source/hal/$(HW_PLF)

KERNEL_HDR_DIR=$(ROOT_DIR)/include/kernel
PORT_HDR_DIR=$(ROOT_DIR)/include/port/$(ARCH)/$(CPU)
PORT_COMMON_HDR_DIR=$(ROOT_DIR)/include/port/$(ARCH)
HAL_VND_HDR_DIR=$(ROOT_DIR)/include/hal/$(HW_PLF)
HAL_COMMON_HDR_DIR=$(ROOT_DIR)/include/hal


TARGET=tachyos.elf
SRCS=
OBJS=





LIBS=
LIB_DIR=




LDSCRIPT=$(HAL_VND_HDR_DIR)/ld/flash.ld
TOOL_CHAIN=arm-none-eabi-
CC=$(TOOL_CHAIN)gcc


ifeq ($(INC),)
	INC = -I$(PORT_HDR_DIR)\
	      -I$(PORT_COMMON_DIR)\
	      -I$(HAL_VND_HDR_DIR)\
	      -I$(HAL_COMMON_HDR_DIR)
endif


ifeq ($(CFLAG),)
	CFLAG = -fsigned-char\
		-ffunction-sections\
		-fdata-sections\
		-ffreestanding\
		-T$(LDSCRIPT)\
endif



. SUFFIX: .o.c

include $(PORT_SRC_DIR)/port.mk
include $(KERNEL_SRC_DIR)/kernel.mk
include $(HAL_SRC_DIR)/hal.m


all : $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAG) $(INC) -o $@

clean:
	rm -rf $(OBJS) $(TARGET)


