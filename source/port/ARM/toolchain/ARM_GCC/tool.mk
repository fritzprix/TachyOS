#################################################################################
########### Created on: 2015. 1. 25.     Author: innocentevil        ############
#################################################################################


TOOL_SEPCIFIC_CFLAG=--specs=nano.specs

TOOL_LIBS = c_nano\
			g_nano\
			stdc++_nano
			
TOOL_LIBS_DIR=
TOOL_LIBS_FLAG = $(TOOL_LIBS:%=-l%)
TOOL_PREFIX=arm-none-eabi-



ifeq ($(CPU),cortex-m4)
	FLOAT_OPTION += -mfpu=fpv4-sp-d16
endif

ifeq ($(FPU),HARD)
	FLOAT_OPTION += -mfloat-abi=hard\
		-DMFEATURE_HFLOAT=1
endif
ifeq ($(FPU),HALFSOFT)
	FLOAT_OPTION += -mfloat-abi=softfp\
		-DMFEATURE_HFLOAT=1
endif
ifeq ($(FPU),SOFT)
	FLOAT_OPTION += -mfloat-abi=soft
endif

CFLAG += $(TOOL_SEPCIFIC_OPTION)
CPFLAG += $(TOOL_SEPCIFIC_OPTION)

LIBS+=$(TOOL_LIBS_FLAG)
LIB_DIR+=$(TOOL_LIBS_DIR)   