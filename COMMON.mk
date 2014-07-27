# Shared Makefile in entire tachyos project


include tchConfig.mk




#OBJS=
#SRCS=

# Tachyos Src Tree Structure
ROOT_DIR= $(CURDIR)
ifeq ($(PUBLISH_TYPE),)
#	PUBLISH_TYPE=Release
	PUBLISH_TYPE=Debug
endif
ifeq ($(GEN_DIR),)
	GEN_DIR=$(ROOT_DIR)/Debug
endif

ifeq ($(PUBLISH_TYPE),Release)
	GEN_DIR=$(ROOT_DIR)/Release
	PUBLISH_TYPE=Release
endif
KERNEL_SRC_DIR=$(ROOT_DIR)/source/sys
PORT_SRC_DIR=$(ROOT_DIR)/source/port/$(ARCH)/$(CPU)
HAL_SRC_DIR=$(ROOT_DIR)/source/hal/$(HW_PLF)


USR_SRC_DIR=$(ROOT_DIR)/source/usr

TCH_API_HDR_DIR=$(ROOT_DIR)/include
KERNEL_HDR_DIR=$(ROOT_DIR)/include/sys
PORT_HDR_DIR=$(ROOT_DIR)/include/port/$(ARCH)/$(CPU)
PORT_COMMON_HDR_DIR=$(ROOT_DIR)/include/port/$(ARCH)
HAL_VND_HDR_DIR=$(ROOT_DIR)/include/hal/$(HW_PLF)
HAL_COMMON_HDR_DIR=$(ROOT_DIR)/include/hal
LIB_HDR_DIR=$(ROOT_DIR)/include/lib
LIB_SRC_DIR=$(ROOT_DIR)/source/lib

USR_HDR_DIR=$(ROOT_DIR)/include/usr



LDSCRIPT=$(HAL_VND_HDR_DIR)/ld/flash.ld
TOOL_CHAIN=arm-none-eabi-
CC=$(TOOL_CHAIN)gcc
CPP=$(TOOL_CHAIN)g++
OBJCP=$(TOOL_CHAIN)objcopy
SIZEPrt=$(TOOL_CHAIN)size

ifeq ($(INC),)
	INC = -I$(PORT_HDR_DIR)\
	      -I$(PORT_COMMON_HDR_DIR)\
	      -I$(HAL_VND_HDR_DIR)\
	      -I$(HAL_COMMON_HDR_DIR)\
	      -I$(KERNEL_HDR_DIR)\
	      -I$(LIB_HDR_DIR)\
	      -I$(TCH_API_HDR_DIR)\
	      -I$(USR_HDR_DIR)	      
endif


ifeq ($(OPT_FLAG),)
	OPT_FLAG=-O0 -g3
ifeq ($(PUBLISH_TYPE),Release)
	OPT_FLAG=-O2 -g
endif
endif

ifeq ($(LDFLAG),)
	LDFLAG=
endif

ifeq ($(CFLAG),)
	CFLAG = -fsigned-char\
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
	CPFLAG = -g -gdwarf-2 -c\
	         -mlong-calls\
	         -ffunction-sections\
	         -ffreestanding\
	         -O\
	         -fno-rtti\
	         -fno-exceptions\
	         -Wall\
	         -fpermissive\
	         -T$(LDSCRIPT)\
	         $(OPT_FLAG)
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


ifneq (&(FLOAT_FLAG),)
	CPFLAG += $(FLOAT_FLAG)
	CFLAG += $(FLOAT_FLAG)
endif


