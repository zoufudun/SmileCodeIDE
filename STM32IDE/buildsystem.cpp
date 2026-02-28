/*
 * @Description: 
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-07 23:45:41
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-04-10 22:18:28
 */
#include "buildsystem.h"
#include <QDir>
#include <QDebug>

BuildSystem::BuildSystem(QObject *parent) : QObject(parent), m_process(nullptr)
{
    m_process = new QProcess(this);
    
    // 连接进程信号
    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildSystem::handleProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildSystem::handleProcessOutput);
    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &BuildSystem::handleProcessFinished);
}

BuildSystem::~BuildSystem()
{
    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            m_process->waitForFinished(3000);
        }
    }
}



void BuildSystem::setProjectPath(const QString &path)
{
    m_projectPath = path;
    
    // 确保项目路径存在
    QDir dir(m_projectPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

void BuildSystem::setOutputPath(const QString &path)
{
    m_outputPath = path;
    
    // 确保输出路径存在
    QDir dir(m_outputPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

bool BuildSystem::compileFile(const QString &filePath)
{
    if (m_projectPath.isEmpty()) {
        m_lastError = "项目路径未设置";
        return false;
    }
    
    if (m_outputPath.isEmpty()) {
        m_outputPath = m_projectPath + "/build";
        QDir dir(m_outputPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
    
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    QString baseName = fileInfo.baseName();
    QString outputFile = m_outputPath + "/" + baseName + ".o";
    
    QString gccPath = m_toolchainPath.isEmpty() ? 
        "arm-none-eabi-gcc" : 
        m_toolchainPath + "/bin/arm-none-eabi-gcc";
    QString gppPath = m_toolchainPath.isEmpty() ? 
        "arm-none-eabi-g++" : 
        m_toolchainPath + "/bin/arm-none-eabi-g++";
    
    QStringList arguments;
    QString program;
    
    if (extension == "c") {
        program = gccPath;
        arguments << "-c" << "-mcpu=cortex-m3" << "-mthumb" << "-Wall" << "-g" << "-O0"
                  << "-DSTM32F103xB"
                  << "-I" + m_projectPath
                  << "-I" + m_projectPath + "/Inc"
                  << "-I" + m_projectPath + "/Drivers/CMSIS/Include"
                  << "-I" + m_projectPath + "/Drivers/CMSIS/Device/ST/STM32F1xx/Include"
                  << "-I" + m_projectPath + "/Drivers/STM32F1xx_HAL_Driver/Inc"
                  << "-o" << outputFile << filePath;
    } else if (extension == "cpp") {
        program = gppPath;
        arguments << "-c" << "-mcpu=cortex-m3" << "-mthumb" << "-Wall" << "-g" << "-O0"
                  << "-fno-exceptions" << "-fno-rtti"
                  << "-DSTM32F103xB"
                  << "-I" + m_projectPath
                  << "-I" + m_projectPath + "/Inc"
                  << "-I" + m_projectPath + "/Drivers/CMSIS/Include"
                  << "-I" + m_projectPath + "/Drivers/CMSIS/Device/ST/STM32F1xx/Include"
                  << "-I" + m_projectPath + "/Drivers/STM32F1xx_HAL_Driver/Inc"
                  << "-o" << outputFile << filePath;
    } else if (extension == "s" || extension == "asm") {
        program = gccPath;
        arguments << "-c" << "-x" << "assembler-with-cpp"
                  << "-mcpu=cortex-m3" << "-mthumb"
                  << "-DSTM32F103xB"
                  << "-o" << outputFile << filePath;
    } else {
        m_lastError = "不支持的文件类型";
        return false;
    }
    
    m_process->setWorkingDirectory(m_projectPath);
    
    qDebug() << "执行命令:" << program << arguments.join(" ");
    m_process->start(program, arguments);
    
    return m_process->waitForStarted();
}

// 实现STM32特定的方法
void BuildSystem::setToolchainPath(const QString &path)
{
    m_toolchainPath = path;
}

void BuildSystem::setTargetChip(const QString &chip)
{
    m_targetChip = chip;
}

// 在generateMakefile方法中添加生成链接脚本的代码
bool BuildSystem::generateMakefile()
{
    if (m_projectPath.isEmpty()) {
        m_lastError = "项目路径未设置";
        return false;
    }
    
    QDir projectDir(m_projectPath);
    if (!projectDir.exists("Inc")) {
        projectDir.mkdir("Inc");
    }
    if (!projectDir.exists("Src")) {
        projectDir.mkdir("Src");
    }
    if (!projectDir.exists("Startup")) {
        projectDir.mkdir("Startup");
    }
    if (!projectDir.exists("Drivers")) {
        projectDir.mkdir("Drivers");
        projectDir.mkdir("Drivers/CMSIS");
        projectDir.mkdir("Drivers/CMSIS/Include");
        projectDir.mkdir("Drivers/CMSIS/Device");
        projectDir.mkdir("Drivers/CMSIS/Device/ST");
        projectDir.mkdir("Drivers/CMSIS/Device/ST/STM32F1xx");
        projectDir.mkdir("Drivers/CMSIS/Device/ST/STM32F1xx/Include");
        projectDir.mkdir("Drivers/CMSIS/Device/ST/STM32F1xx/Source");
        projectDir.mkdir("Drivers/STM32F1xx_HAL_Driver");
        projectDir.mkdir("Drivers/STM32F1xx_HAL_Driver/Inc");
        projectDir.mkdir("Drivers/STM32F1xx_HAL_Driver/Src");
    }
    if (!projectDir.exists("build")) {
        projectDir.mkdir("build");
    }
    
    if (!generateLinkerScript()) {
        return false;
    }
    
    QFile file(m_projectPath + "/Makefile");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "无法创建Makefile文件";
        return false;
    }
    
    QTextStream out(&file);
    
    out << "# STM32 Makefile - Auto Generated\n\n";
    
    out << "# Toolchain\n";
    if (m_toolchainPath.isEmpty()) {
        out << "PREFIX = arm-none-eabi-\n";
    } else {
        out << "TOOLCHAIN_PATH = " << m_toolchainPath << "\n";
        out << "PREFIX = $(TOOLCHAIN_PATH)/bin/arm-none-eabi-\n";
    }
    out << "CC = $(PREFIX)gcc\n";
    out << "CXX = $(PREFIX)g++\n";
    out << "AS = $(PREFIX)gcc -x assembler-with-cpp\n";
    out << "OBJCOPY = $(PREFIX)objcopy\n";
    out << "OBJDUMP = $(PREFIX)objdump\n";
    out << "SIZE = $(PREFIX)size\n\n";
    
    out << "# Target\n";
    QString targetName = m_targetChip.isEmpty() ? "firmware" : m_targetChip.toLower();
    out << "TARGET = " << targetName << "\n";
    out << "BUILD_DIR = build\n\n";
    
    out << "# MCU Flags\n";
    out << "CPU = -mcpu=cortex-m3\n";
    out << "FPU =\n";
    out << "FLOAT-ABI =\n";
    out << "MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)\n\n";
    
    out << "# Compile Flags\n";
    out << "AS_DEFS =\n";
    out << "C_DEFS = -DSTM32F103xB -DUSE_HAL_DRIVER\n\n";
    
    out << "AS_INCLUDES =\n";
    out << "C_INCLUDES = \\\n";
    out << "  -I. \\\n";
    out << "  -IInc \\\n";
    out << "  -IDrivers/CMSIS/Include \\\n";
    out << "  -IDrivers/CMSIS/Device/ST/STM32F1xx/Include \\\n";
    out << "  -IDrivers/STM32F1xx_HAL_Driver/Inc\n\n";
    
    out << "ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) -Wall -fdata-sections -ffunction-sections\n";
    out << "CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -Wall -fdata-sections -ffunction-sections -g -O0\n";
    out << "CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti\n\n";
    
    out << "# Linker\n";
    out << "LDSCRIPT = STM32F103C8Tx_FLASH.ld\n";
    out << "LIBS = -lc -lm -lnosys\n";
    out << "LIBDIR =\n";
    out << "LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections\n\n";
    
    out << "# Sources\n";
    out << "C_SOURCES = $(wildcard Src/*.c)\n";
    out << "C_SOURCES += $(wildcard Drivers/STM32F1xx_HAL_Driver/Src/*.c)\n";
    out << "C_SOURCES += $(wildcard Drivers/CMSIS/Device/ST/STM32F1xx/Source/*.c)\n\n";
    
    out << "CXX_SOURCES = $(wildcard Src/*.cpp)\n\n";
    
    out << "ASM_SOURCES = $(wildcard Startup/*.s)\n";
    out << "ASM_SOURCES += $(wildcard Src/*.s)\n\n";
    
    out << "# Objects\n";
    out << "OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))\n";
    out << "OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CXX_SOURCES:.cpp=.o)))\n";
    out << "OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))\n";
    out << "vpath %.c $(sort $(dir $(C_SOURCES)))\n";
    out << "vpath %.cpp $(sort $(dir $(CXX_SOURCES)))\n";
    out << "vpath %.s $(sort $(dir $(ASM_SOURCES)))\n\n";
    
    out << "# Build Rules\n";
    out << "all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin\n\n";
    
    out << "$(BUILD_DIR):\n";
    out << "\tmkdir -p $@\n\n";
    
    out << "$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)\n";
    out << "\t$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@\n\n";
    
    out << "$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)\n";
    out << "\t$(CXX) -c $(CXXFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cpp=.lst)) $< -o $@\n\n";
    
    out << "$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)\n";
    out << "\t$(AS) -c $(ASFLAGS) $< -o $@\n\n";
    
    out << "$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)\n";
    out << "\t$(CC) $(OBJECTS) $(LDFLAGS) -o $@\n";
    out << "\t$(SIZE) $@\n\n";
    
    out << "$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf\n";
    out << "\t$(OBJCOPY) -O ihex $< $@\n\n";
    
    out << "$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf\n";
    out << "\t$(OBJCOPY) -O binary -S $< $@\n\n";
    
    out << "clean:\n";
    out << "\trm -rf $(BUILD_DIR)\n\n";
    
    out << "flash: $(BUILD_DIR)/$(TARGET).bin\n";
    out << "\tst-flash write $< 0x8000000\n\n";
    
    out << ".PHONY: all clean flash\n";
    
    file.close();
    return true;
}


bool BuildSystem::buildProject()
{
    if (m_projectPath.isEmpty()) {
        m_lastError = "项目路径未设置";
        return false;
    }
    
    // 确保输出路径存在
    if (m_outputPath.isEmpty()) {
        m_outputPath = m_projectPath + "/build";
    }
    
    QDir dir(m_outputPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 生成Makefile
    if (!generateMakefile()) {
        return false;
    }
    
    // 设置工作目录
    m_process->setWorkingDirectory(m_projectPath);
    
    // 启动make进程
    QString command = "make";
    QStringList arguments;
    arguments << "-j4"; // 使用4个线程进行编译
    
    qDebug() << "执行命令: " << command << arguments.join(" ");
    m_process->start(command, arguments);
    
    return m_process->waitForStarted();
}

// 修改getCompileCommand方法以支持STM32工程
// QString BuildSystem::getCompileCommand(const QString &filePath)
// {
//     QFileInfo fileInfo(filePath);
//     QString extension = fileInfo.suffix().toLower();
    
//     if (extension == "c") {
//         // 使用arm-none-eabi-gcc编译C文件
//         return QString("%1/bin/arm-none-eabi-gcc -c -mcpu=cortex-m3 -mthumb -Wall -g -O0 -DSTM32F103xB -I. -I./Inc -I./Drivers/CMSIS/Include -I./Drivers/CMSIS/Device/ST/STM32F1xx/Include -I./Drivers/STM32F1xx_HAL_Driver/Inc -o \"%2/%3.o\" \"%4\"")
//             .arg(m_toolchainPath)
//             .arg(m_outputPath)
//             .arg(fileInfo.baseName())
//             .arg(filePath);
//     } else if (extension == "cpp") {
//         // 使用arm-none-eabi-g++编译C++文件
//         return QString("%1/bin/arm-none-eabi-g++ -c -mcpu=cortex-m3 -mthumb -Wall -g -O0 -DSTM32F103xB -I. -I./Inc -I./Drivers/CMSIS/Include -I./Drivers/CMSIS/Device/ST/STM32F1xx/Include -I./Drivers/STM32F1xx_HAL_Driver/Inc -o \"%2/%3.o\" \"%4\"")
//             .arg(m_toolchainPath)
//             .arg(m_outputPath)
//             .arg(fileInfo.baseName())
//             .arg(filePath);
//     }
    
//     return QString();
// }

QString BuildSystem::getCompileCommand(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    QString baseName = fileInfo.baseName();
    QString outputFile = m_outputPath + "/" + baseName + ".o";
    
    // 构建编译器路径
    QString gccPath = m_toolchainPath.isEmpty() ? 
        "arm-none-eabi-gcc" : 
        m_toolchainPath + "/bin/arm-none-eabi-gcc";
    QString gppPath = m_toolchainPath.isEmpty() ? 
        "arm-none-eabi-g++" : 
        m_toolchainPath + "/bin/arm-none-eabi-g++";
    
    // 构建编译标志
    QString cflags = "-mcpu=cortex-m3 -mthumb -Wall -g -O0";
    QString defines = "-DSTM32F103xB";
    QString includes = QString("-I\"%1\" -I\"%1/Inc\" -I\"%1/Drivers/CMSIS/Include\" "
                               "-I\"%1/Drivers/CMSIS/Device/ST/STM32F1xx/Include\" "
                               "-I\"%1/Drivers/STM32F1xx_HAL_Driver/Inc\"")
                       .arg(m_projectPath);
    
    // 根据文件类型选择编译器
    if (extension == "c") {
        // 使用arm-none-eabi-gcc编译C文件
        return QString("\"%1\" -c %2 %3 %4 -o \"%5\" \"%6\"")
            .arg(gccPath, cflags, defines, includes, outputFile, filePath);
    } else if (extension == "cpp") {
        // 使用arm-none-eabi-g++编译C++文件
        return QString("\"%1\" -c %2 %3 %4 -fno-exceptions -fno-rtti -o \"%5\" \"%6\"")
            .arg(gppPath, cflags, defines, includes, outputFile, filePath);
    } else if (extension == "s" || extension == "asm") {
        // 使用arm-none-eabi-gcc编译汇编文件
        return QString("\"%1\" -c -x assembler-with-cpp %2 %3 -o \"%4\" \"%5\"")
            .arg(gccPath, cflags, defines, outputFile, filePath);
    }
    
    return QString();
}

void BuildSystem::handleProcessOutput()
{
    // 读取标准输出
    QByteArray output = m_process->readAllStandardOutput();
    if (!output.isEmpty()) {
        emit buildOutput(QString::fromLocal8Bit(output));
    }
    
    // 读取标准错误
    QByteArray error = m_process->readAllStandardError();
    if (!error.isEmpty()) {
        emit buildOutput(QString::fromLocal8Bit(error));
    }
}

void BuildSystem::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    
    if (!success) {
        m_lastError = "编译失败，退出代码: " + QString::number(exitCode);
    }
    
    emit buildFinished(success);
}

// 添加关闭工程的方法
// 添加关闭工程的方法
void BuildSystem::closeProject()
{
    // 如果有正在运行的进程，终止它
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(3000);
    }
    
    // 清空项目路径和输出路径
    m_projectPath.clear();
    m_outputPath.clear();
    
    // 清空错误信息
    m_lastError.clear();
    
    // 发出工程关闭信号
    emit projectClosed();
}

QString BuildSystem::lastError() const
{
    return m_lastError;
}

// 添加生成链接脚本的方法
bool BuildSystem::generateLinkerScript()
{
    if (m_projectPath.isEmpty()) {
        m_lastError = "项目路径未设置";
        return false;
    }
    
    // 检查链接脚本是否已存在
    QFile ldFile(m_projectPath + "/STM32F103C8Tx_FLASH.ld");
    if (ldFile.exists()) {
        // 如果已存在，不覆盖
        return true;
    }
    
    // 创建链接脚本文件
    if (!ldFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "无法创建链接脚本文件";
        return false;
    }
    
    QTextStream out(&ldFile);
    
    // 写入链接脚本内容
    out << "/*\n";
    out << " * STM32F103C8Tx链接脚本\n";
    out << " * 内存布局：\n";
    out << " * FLASH: 64KB (0x10000)\n";
    out << " * RAM: 20KB (0x5000)\n";
    out << " */\n\n";
    
    out << "/* 入口点 */\n";
    out << "ENTRY(Reset_Handler)\n\n";
    
    out << "/* 内存定义 */\n";
    out << "MEMORY\n";
    out << "{\n";
    out << "  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 64K\n";
    out << "  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 20K\n";
    out << "}\n\n";
    
    out << "/* 堆栈大小定义 */\n";
    out << "_Min_Heap_Size = 0x200;  /* 最小堆大小 */\n";
    out << "_Min_Stack_Size = 0x400; /* 最小栈大小 */\n\n";
    
    out << "/* 定义输出段 */\n";
    out << "SECTIONS\n";
    out << "{\n";
    out << "  /* 中断向量表和代码段 */\n";
    out << "  .isr_vector :\n";
    out << "  {\n";
    out << "    . = ALIGN(4);\n";
    out << "    KEEP(*(.isr_vector))\n";
    out << "    . = ALIGN(4);\n";
    out << "  } >FLASH\n\n";
    
    out << "  /* 代码段 */\n";
    out << "  .text :\n";
    out << "  {\n";
    out << "    . = ALIGN(4);\n";
    out << "    *(.text)\n";
    out << "    *(.text*)\n";
    out << "    *(.glue_7)\n";
    out << "    *(.glue_7t)\n";
    out << "    *(.eh_frame)\n";
    out << "    KEEP (*(.init))\n";
    out << "    KEEP (*(.fini))\n";
    out << "    . = ALIGN(4);\n";
    out << "    _etext = .;\n";
    out << "  } >FLASH\n\n";
    
    out << "  /* 只读数据段 */\n";
    out << "  .rodata :\n";
    out << "  {\n";
    out << "    . = ALIGN(4);\n";
    out << "    *(.rodata)\n";
    out << "    *(.rodata*)\n";
    out << "    . = ALIGN(4);\n";
    out << "  } >FLASH\n\n";
    
    out << "  /* 用于调用静态构造函数的段 */\n";
    out << "  .ARM.extab :\n";
    out << "  {\n";
    out << "    *(.ARM.extab* .gnu.linkonce.armextab.*)\n";
    out << "  } >FLASH\n\n";
    
    out << "  .ARM :\n";
    out << "  {\n";
    out << "    __exidx_start = .;\n";
    out << "    *(.ARM.exidx*)\n";
    out << "    __exidx_end = .;\n";
    out << "  } >FLASH\n\n";
    
    out << "  /* 数据段 */\n";
    out << "  _sidata = LOADADDR(.data);\n";
    out << "  .data :\n";
    out << "  {\n";
    out << "    . = ALIGN(4);\n";
    out << "    _sdata = .;\n";
    out << "    *(.data)\n";
    out << "    *(.data*)\n";
    out << "    . = ALIGN(4);\n";
    out << "    _edata = .;\n";
    out << "  } >RAM AT> FLASH\n\n";
    
    out << "  /* BSS段 */\n";
    out << "  . = ALIGN(4);\n";
    out << "  .bss :\n";
    out << "  {\n";
    out << "    _sbss = .;\n";
    out << "    __bss_start__ = _sbss;\n";
    out << "    *(.bss)\n";
    out << "    *(.bss*)\n";
    out << "    *(COMMON)\n";
    out << "    . = ALIGN(4);\n";
    out << "    _ebss = .;\n";
    out << "    __bss_end__ = _ebss;\n";
    out << "  } >RAM\n\n";
    
    out << "  /* 用户堆栈段 */\n";
    out << "  ._user_heap_stack :\n";
    out << "  {\n";
    out << "    . = ALIGN(8);\n";
    out << "    PROVIDE ( end = . );\n";
    out << "    PROVIDE ( _end = . );\n";
    out << "    . = . + _Min_Heap_Size;\n";
    out << "    . = . + _Min_Stack_Size;\n";
    out << "    . = ALIGN(8);\n";
    out << "  } >RAM\n\n";
    
    out << "  /* Discard unused sections */\n";
    out << "  /DISCARD/ :\n";
    out << "  {\n";
    out << "    *(.comment)\n";
    out << "    *(.note*)\n";
    out << "  }\n\n";
    
    out << "  .ARM.attributes 0 : { *(.ARM.attributes) }\n";
    out << "}\n";
    
    ldFile.close();
    return true;
}

// Add this implementation to your BuildSystem class
bool BuildSystem::cleanProject()
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    // Create a process to run the clean command
    QProcess process;
    process.setWorkingDirectory(m_projectPath);

    // Set up the clean command (typically 'make clean')
    QStringList arguments;
    arguments << "clean";

    // Start the process
    process.start("make", arguments);

    // Wait for the process to finish
    if (!process.waitForFinished(-1)) {
        return false;
    }

    // Check if the process was successful
    return (process.exitCode() == 0);
}
