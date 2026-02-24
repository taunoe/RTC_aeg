################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include User/subdir.mk
-include Startup/subdir.mk
-include Peripheral/src/subdir.mk
-include Debug/subdir.mk
-include Core/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
RTC_aeg.hex \

SECONDARY_LIST += \
RTC_aeg.lst \

SECONDARY_SIZE += \
RTC_aeg.siz \


# All Target
all: 
	$(MAKE) --no-print-directory main-build 

main-build: RTC_aeg.elf secondary-outputs

# Tool invocations
RTC_aeg.elf: $(OBJS) $(USER_OBJS_ESCAPE)
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -T "/home/taunoerik/mounriver-studio-projects/RTC_aeg/Ld/Link.ld" -nostartfiles -Xlinker --gc-sections -Wl,-Map,"RTC_aeg.map" --specs=nano.specs --specs=nosys.specs -o "RTC_aeg.elf" $(OBJS) $(LIBS) -lprintf
RTC_aeg.hex: RTC_aeg.elf
	@	riscv-none-embed-objcopy -O ihex "RTC_aeg.elf" "RTC_aeg.hex"
RTC_aeg.lst: RTC_aeg.elf
	@	riscv-none-embed-objdump --all-headers --demangle --disassemble -M xw "RTC_aeg.elf" > "RTC_aeg.lst"
RTC_aeg.siz: RTC_aeg.elf
	riscv-none-embed-size --format=berkeley "RTC_aeg.elf"

# Other Targets
clean:
	-$(RM) $(DIR_OBJS) $(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE) RTC_aeg.elf
	-$(RM) $(DIR_EXPANDS) $(CALLGRAPH_DOT)
	-$(RM) $(DIR_DEPS)
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets