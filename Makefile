# Build system for STM32F401CEU6 blinky firmware.
# Produces build/blinky.elf (for debugging) and build/blinky.bin (raw flash image).

TARGET  = blinky
BUILD   = build

# GNU ARM Embedded toolchain. `-x assembler-with-cpp` lets .s files use the C preprocessor.
CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# MCU arch flags. Must be identical at compile and link time or the linker
# picks the wrong multilib (soft-float libs vs. hard-float libs, etc).
#   cortex-m4 + thumb: the core and its instruction set
#   fpv4-sp-d16 + hard: STM32F401 has a single-precision FPU; pass floats in FPU regs
MCU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Preprocessor defines.
#   STM32F401xE  - selects the correct register map in stm32f4xx.h for our part
#   USE_HAL_DRIVER - enables the STM32 HAL
DEFS    = -DUSE_HAL_DRIVER -DSTM32F401xE

# Header search paths: app code + HAL + CMSIS (device + core).
INCLUDES = \
	-ICore/Inc \
	-IDrivers/STM32F4xx_HAL_Driver/Inc \
	-IDrivers/STM32F4xx_HAL_Driver/Inc/Legacy \
	-IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
	-IDrivers/CMSIS/Include

# Compile flags.
#   -Og                   optimize but stay debuggable (better than -O0 for size)
#   -g3                   full debug info including macro definitions
#   -ffunction-sections   one ELF section per function; enables --gc-sections at link
#   -fdata-sections       same, but for data
CFLAGS  = $(MCU) $(DEFS) $(INCLUDES) -Og -g3 -Wall -ffunction-sections -fdata-sections
ASFLAGS = $(MCU) -Wall

# Linker script: defines memory regions (512K flash @ 0x08000000, 96K SRAM @ 0x20000000)
# and section layout (.isr_vector first in flash, .data copied to RAM on boot, etc).
LDSCRIPT = stm32f401.ld

# Link flags.
#   --specs=nano.specs    link against newlib-nano (tiny libc)
#   --specs=nosys.specs   stub _read/_write/_sbrk/etc. so link succeeds without syscalls
#                         (produces harmless "unimplemented" warnings; GC discards them)
#   -Wl,--gc-sections     drop unreferenced sections; combined with -ffunction-sections
#                         this is why 17 HAL files compile down to ~5 KB of flash
#   -Wl,-Map=...          emit map file showing final symbol/section placement
LDFLAGS  = $(MCU) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs \
	-Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

# Application + HAL sources. HAL modules listed here must match the HAL_*_MODULE_ENABLED
# defines in Core/Inc/stm32f4xx_hal_conf.h. Enabling a new module there (e.g. UART)
# means adding the matching stm32f4xx_hal_<module>.c here too.
C_SOURCES = \
	Core/Src/main.c \
	Core/Src/stm32f4xx_it.c \
	Core/Src/stm32f4xx_hal_msp.c \
	Core/Src/system_stm32f4xx.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c

# Startup code: vector table, Reset_Handler (copies .data, zeros .bss, calls SystemInit, jumps to main).
# Use the GCC-flavored version; the IAR/arm variants have different syntax.
ASM_SOURCES = Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f401xe.s

# Flatten source paths into build/*.o (e.g. Core/Src/main.c -> build/main.o).
OBJS  = $(addprefix $(BUILD)/, $(notdir $(C_SOURCES:.c=.o)))
OBJS += $(addprefix $(BUILD)/, $(notdir $(ASM_SOURCES:.s=.o)))

# VPATH tells make where to find the source files referenced by basename in the pattern
# rules below. Without it, the `%.c` rule couldn't locate Core/Src/main.c from "main.c".
VPATH = $(sort $(dir $(C_SOURCES) $(ASM_SOURCES)))

# Default target: build the raw flash image (which depends on the .elf).
all: $(BUILD)/$(TARGET).bin

$(BUILD):
	mkdir -p $@

# Compile rule. The `| $(BUILD)` is an order-only prerequisite: the directory must
# exist, but changes to its mtime don't trigger rebuilds.
#   -MMD -MP -MF build/foo.d  auto-generate a makefile fragment listing every header
#                             this .c includes, included at the bottom of this file
$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) -c $(CFLAGS) -MMD -MP -MF $(@:.o=.d) $< -o $@

$(BUILD)/%.o: %.s | $(BUILD)
	$(AS) -c $(ASFLAGS) $< -o $@

# Link all objects into the ELF, then print text/data/bss size.
$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

# Convert ELF to raw binary for flash programming.
$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Flash via ST-Link + OpenOCD.
#
# The two `configure -event` overrides replace the default speed handlers in
# target/stm32f4x.cfg. The ST-Link V2 only supports discrete SWD speeds (..., 1800,
# 4000, 4500 kHz); the default handlers ask for 2000/8000 and the adapter rounds
# down, which logs "Unable to match requested speed" warnings. We request 1800/4000
# directly. The reset-init body replicates the PLL boost from the stock cfg (required
# for fast programming) and only swaps the final `adapter speed` line.
flash:
	openocd \
	-f interface/stlink.cfg \
	-f target/stm32f4x.cfg \
	-c "stm32f4x.cpu configure -event reset-start {adapter speed 1800}" \
	-c "stm32f4x.cpu configure -event reset-init { \
		mww 0x40023804 0x08012008; \
		mww 0x40023C00 0x00000102; \
		mmw 0x40023800 0x01000000 0; \
		sleep 10; \
		mmw 0x40023808 0x00001000 0; \
		mmw 0x40023808 0x00000002 0; \
		adapter speed 4000; \
	}" \
	-c "init" \
	-c "reset halt" \
	-c "program $(BUILD)/$(TARGET).elf verify" \
	-c "reset run" \
	-c "exit"

clean:
	rm -rf $(BUILD)

# Clean rebuild. Uses recursive $(MAKE) so the two steps run sequentially
# even under `make -j` (listing them as prereqs would let them race).
rebuild:
	$(MAKE) clean
	$(MAKE) all

# Pull in the auto-generated header dependencies. Leading `-` so the first build
# (when no .d files exist yet) doesn't error.
-include $(OBJS:.o=.d)

.PHONY: all rebuild flash clean
