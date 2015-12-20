#tool chain specific build option


# Linker Option
LDFLAG_COMMON +=-Map,$(DEBUG_TARGET:%.elf=%.map)\
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
				-mthumb

# Cpp build option
CPFLAG_COMMON = -mlong-calls\
				-fdata-sections\
				-ffreestanding\
				-fno-rtti\
				-fno-exceptions\
				-Wall\
				-fpermissive\
				-T$(LDSCRIPT)\
				$(FLOAT_OPTION)\
				-mcpu=$(CONFIG_ARCH_NAME)\
				-mthumb

CROSS_COMPILE=arm-none-eabi-
# floating point option
FLOAT_OPTION= -mfloat-abi=$(CONFIG_FPU_ABI) -mfpu=$(CONFIG_FPU_VERSION)
