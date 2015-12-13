#tool chain specific build option


# Linker Option
LDFLAG_COMMON +=-Map,$(TARGET:%.elf=%.map)\
				--gc-sections

# C build option
CFLAG_COMMON += -fsigned-char\
				-ffunction-sections\
				-sections\
				-ffreestanding\
				-nostartfiles\
				-T$(LDSCRIPT)\
				$(FLOAT_OPTION)\
				-mcpu=$(CONFIG_ARCH_NAME)\
				-mthumb

# Cpp build option
CPFLAG_COMMON = -mlong-calls\
				-sections\
				-ffreestanding\
				-fno-rtti\
				-fno-exceptions\
				-Wall\
				-fpermissive\
				-T$(LDSCRIPT)\
				$(FLOAT_OPTION)\
				-mcpu=$(CONFIG_ARCH_NAME)\
				-mthumb

# floating point option
FLOAT_OPTION= -mfloat-abi=$(CONFIG_FPU_ABI) -mfpu=$(CONFIG_FPU_VERSION)
