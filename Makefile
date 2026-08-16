# STM32 Makefile - Auto Generated

# Toolchain
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
AS = $(PREFIX)gcc -x assembler-with-cpp
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE = $(PREFIX)size

# Target
TARGET = firmware
BUILD_DIR = build

# MCU Flags
CPU = -mcpu=cortex-m3
FPU =
FLOAT-ABI =
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# Compile Flags
AS_DEFS =
C_DEFS = -DSTM32F103xB -DUSE_HAL_DRIVER

AS_INCLUDES =
C_INCLUDES = \
  -I. \
  -IInc \
  -IDrivers/CMSIS/Include \
  -IDrivers/CMSIS/Device/ST/STM32F1xx/Include \
  -IDrivers/STM32F1xx_HAL_Driver/Inc

ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) -Wall -fdata-sections -ffunction-sections
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -Wall -fdata-sections -ffunction-sections -g -O0
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

# Linker
LDSCRIPT = STM32F103C8Tx_FLASH.ld
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# Sources
C_SOURCES = $(wildcard Src/*.c)
C_SOURCES += $(wildcard Drivers/STM32F1xx_HAL_Driver/Src/*.c)
C_SOURCES += $(wildcard Drivers/CMSIS/Device/ST/STM32F1xx/Source/*.c)

CXX_SOURCES = $(wildcard Src/*.cpp)

ASM_SOURCES = $(wildcard Startup/*.s)
ASM_SOURCES += $(wildcard Src/*.s)

# Objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CXX_SOURCES:.cpp=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.cpp $(sort $(dir $(CXX_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Build Rules
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cpp=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary -S $< $@

clean:
	rm -rf $(BUILD_DIR)

flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash write $< 0x8000000

.PHONY: all clean flash
