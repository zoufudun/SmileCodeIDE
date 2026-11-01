################################################################################
# SmileCodeIDE STM32 Makefile - Auto Generated
# Target Chip: STM32F103C8 (STM32F1 / cortex-m3)
################################################################################

TARGET = stm32f103c8
BUILD_DIR = build

# Toolchain Paths
BIN_DIR = D:/Toolchains/arm-gnu-toolchain/bin/
PREFIX = $(BIN_DIR)arm-none-eabi-
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# Platform shell commands
ifeq ($(OS),Windows_NT)
  RM = cmd /c if exist $(BUILD_DIR) rd /s /q $(BUILD_DIR)
  MKDIR = cmd /c if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
else
  RM = rm -rf $(BUILD_DIR)
  MKDIR = mkdir -p $(BUILD_DIR)
endif

# MCU Architecture Flags
CPU = -mcpu=cortex-m3
MCU = $(CPU) -mthumb

# C / C++ Defines
C_DEFS = -DSTM32F103xB -DUSE_HAL_DRIVER

# Header Includes
C_INCLUDES = \
  -I. \
  -IInc \
  -ISrc

# Compiler Flags
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -O0 -g3 -gdwarf-2 -Wall -fdata-sections -ffunction-sections
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -O0 -g3 -gdwarf-2 -Wall -fdata-sections -ffunction-sections

# Generate dependency files for incremental builds
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
CXXFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# Linker Script & Flags
LDSCRIPT = STM32F103C8Tx_FLASH.ld
LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# C Sources
C_SOURCES = \
  Src/main.c

# Assembler Sources
ASM_SOURCES = \
  Startup/startup_stm32f103xb.s

# Object File Mapping
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Build Targets
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	$(MKDIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

clean:
	$(RM)

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
