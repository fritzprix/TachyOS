
#################################################################
##################   ARM GCC Specific Makefile ##################
##################   Author : Doowoong,Lee     ##################
#################################################################

# Optimization options (BUILD = Release / Debug)
ifeq ($(BUILD),Release)
	OPT_FLAG=-O2 -g3
	DBG_OPTION=
else
	OPT_FLAG=-O0 -g3
	DBG_OPTION=-D__DBG
endif

# Linker Option
TOOL_LINKER_OPT=-Map,$(TARGET:%.elf=%.map)\
				--gc-sections\



# C build option
TOOL_CFLAG =-fsigned-char\
		   -ffunction-sections\
		   -fdata-sections\
		   -ffreestanding\
		   -nostartfiles\
		   -T$(LDSCRIPT)\
		   $(OPT_FLAG)\
		   -mcpu=$(ARCH_NAME)\
		   -mthumb

# Cpp build option
TOOL_CPFLAG = -mlong-calls\
	         -ffunction-sections\
	         -ffreestanding\
	         -fno-rtti\
	         -fno-exceptions\
	         -Wall\
	         -fpermissive\
	         -T$(LDSCRIPT)\
	          $(OPT_FLAG)\
	          -mcpu=$(CPU)\
	          -mthumb


CFLAG+= $(TOOL_CFLAG)
CPFLAG+= $(TOOL_CPFLAG)
LDFLAG+= $(TOOL_LINKER_OPT:%=-Wl,%)		

TOOL_LIBS_DIR=
TOOL_LIBS_FLAG = $(TOOL_LIBS:%=-l%)
TOOL_PREFIX=arm-none-eabi-


ifeq ($(FPU),HARD)
	ifeq ($(CPU),cortex-m4)
		FLOAT_OPTION += -mfpu=fpv4-sp-d16
	endif
	FLOAT_OPTION += -mfloat-abi=hard -DMFEATURE_HFLOAT=1
endif

ifeq ($(FPU),HALFSOFT)
	ifeq ($(CPU),cortex-m4)
		FLOAT_OPTION += -mfpu=fpv4-sp-d16
	endif
	FLOAT_OPTION += -mfloat-abi=softfp -DMFEATURE_HFLOAT=1
endif

ifeq ($(FPU),SOFT)
	FLOAT_OPTION += -mfloat-abi=soft
endif

LIBS+=$(TOOL_LIBS_FLAG)
LIB_DIR+=$(TOOL_LIBS_DIR)   


CC=$(TOOL_PREFIX)gcc
CPP=$(TOOL_PREFIX)g++
OBJCP=$(TOOL_PREFIX)objcopy
SIZEPRINT=$(TOOL_PREFIX)size