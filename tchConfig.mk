
############################### Base Configuration #######################
#

# Architecture Conf.
ifeq ($(ARCH),)
	ARCH = ARM
endif

# CPU Family  cortex-m3 | cortex-m0
ifeq ($(CPU),)
	CPU = cortex-m4
endif

# FPU Option   HARD | SOFT | NO
ifeq ($(FPU),)
	FPU = HARD
endif

# Hardware Vendor Option
ifeq ($(HW_VENDOR),)
	HW_VENDOR = "ST Mocro"
endif

# IC_FAMILY Option
ifeq ($(HW_PLF),)
	HW_PLF = STM32F40_41xxx
endif







