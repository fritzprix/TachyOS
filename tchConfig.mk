
############################### Base Configuration #######################
#



#### initialize Configuartion into default value ####
# Architecture Conf.
ifeq ($(ARCH),)
	ARCH = ARM
endif

# CPU Family  cortex-m3 | cortex-m0
ifeq ($(CPU),)
	CPU = cortex-m4
endif

# Instruction Set Option
ifeq ($(INSTR),)
	INSTR = thumb
endif

# FPU Option   HARD | HALFSOFT | SOFT | NO
ifeq ($(FPU),)
	FPU = SOFT
endif

# Hardware Vendor Option
ifeq ($(HW_VENDOR),)
	HW_VENDOR = ST_Micro
endif

# IC_FAMILY Option
ifeq ($(HW_PLF),)
	HW_PLF=STM32F40_41xxx
endif

ifeq ($(BOARD_NAME),)
	BOARD_NAME=Port_103Z
endif




### Open 407Z ###\
ARCH = ARM\
CPU = cortex-m4\
INSTR = thumb\
FPU = HALFSOFT\
HW_VENDOR = ST_Micro\
HW_PLF = STM32F40_41xxx\

### Open 427Z ###\
ARCH = ARM\
CPU = cortex-m4\
INSTR = thumb\
FPU = HALFSOFT\
HW_VENDOR = ST_Micro\
HW_PLF = STM32F427_437xx\

###
ARCH=ARM
CPU=cortex-m3
INSTR=thumb
FPU=SOFT
HW_VENDOR=ST_Micro
HW_PLF=STM32F2XX






