
############################### Base Configuration #######################




#### initialize Configuartion into default value ####


ifeq ($(BUILD),)
	BUILD=Debug
endif


ifeq ($(MAJOR_VER),)
	MAJOR_VER=0
endif

ifeq ($(MINOR_VER),)
	MINOR_VER=1
endif

ifeq ($(TOOLCHAIN_NAME),)
	TOOLCHAIN_NAME=ARM_GCC
endif


### Open 407Z ###\
ARCH = ARM\
CPU = cortex-m4\
FPU = HALFSOFT\
HW_VENDOR = ST_Micro\
HW_PLF = STM32F40_41xxx\
BOARD_NAME=Open_407Z\
APP_NAME = 

### Open 427Z ###\
ARCH = ARM\
CPU = cortex-m4\
FPU = HALFSOFT\
HW_VENDOR = ST_Micro\
HW_PLF = STM32F427_437xx\
APP_NAME =\

###
ARCH=ARM
CPU=cortex-m3
FPU=SOFT
HW_VENDOR=ST_Micro
HW_PLF=STM32F2XX
BOARD_NAME=Port_103Z
APP_NAME=#iic_slaveWriter





# Architecture Conf.
ifeq ($(ARCH),)
	ARCH = ARM
endif

# CPU Family  cortex-m3 | cortex-m0
ifeq ($(CPU),)
	CPU = cortex-m4
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

ifeq ($(APP_NAME),)
	APP_NAME=default
endif



