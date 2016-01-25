#tool chain specific build option

# Linker Option
LDFLAG_COMMON +=-Map,$(@:%.elf=%.map)\
				--gc-sections

# C build option
CFLAG_COMMON += -fsigned-char\
				-ffunction-sections\
				-fdata-sections\
				-ffreestanding\
				-nostartfiles\
				-T$(LDSCRIPT)\
				$(FLOAT_OPTION)\
				-mcpu=$(CONFIG_ARCH_NAME)\
				-mthumb\
				-std=gnu89

# Cpp build option
CPFLAG_COMMON = -mlong-calls\
				-fdata-sections\
				-ffreestanding\
				-fno-rtti\
				-fno-exceptions\
				-fpermissive\
				-T$(LDSCRIPT)\
				$(FLOAT_OPTION)\
				-mcpu=$(CONFIG_ARCH_NAME)\
				-mthumb\
				-std=gnu89

CROSS_COMPILE=arm-none-eabi-
# floating point option
FLOAT_OPTION= -mfloat-abi=$(CONFIG_FPU_ABI)
ifneq ($(CONFIG_FPU_VERSION),)
	FLOAT_OPTION+= -mfpu=$(CONFIG_FPU_VERSION)
endif
