################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/util/subdir.mk
-include src/lld/subdir.mk
-include src/core/port/subdir.mk
-include src/core/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
tachyOS_v0.9.hex \

SECONDARY_SIZE += \
tachyOS_v0.9.siz \


# All Target
all: tachyOS_v0.9.elf secondary-outputs

# Tool invocations
tachyOS_v0.9.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C Linker'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding  -g3 -T "C:\Users\innocentevil\arm_cworkspace\tachyOS_v0.9\ld\stm32f4xx_flash.ld" -nostartfiles -nostdlib -Xlinker --gc-sections -L"C:\Users\innocentevil\arm_cworkspace\STM32F4xx_LIBRARY\Debug" -Wl,-Map,"tachyOS_v0.9.map" -o "tachyOS_v0.9.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

tachyOS_v0.9.hex: tachyOS_v0.9.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex "tachyOS_v0.9.elf"  "tachyOS_v0.9.hex"
	@echo 'Finished building: $@'
	@echo ' '

tachyOS_v0.9.siz: tachyOS_v0.9.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "tachyOS_v0.9.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(S_UPPER_DEPS) tachyOS_v0.9.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
