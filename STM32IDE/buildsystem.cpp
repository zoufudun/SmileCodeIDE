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
    
    // 获取编译命令
    QString command = getCompileCommand(filePath);
    if (command.isEmpty()) {
        m_lastError = "不支持的文件类型";
        return false;
    }
    
    // 设置工作目录
    m_process->setWorkingDirectory(m_projectPath);
    
    // 启动编译进程
    qDebug() << "执行命令: " << command;
    m_process->start(command);
    
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
    
    // 创建标准STM32目录结构
    QDir projectDir(m_projectPath);
    if (!projectDir.exists("Inc")) {
        projectDir.mkdir("Inc");
    }
    if (!projectDir.exists("Src")) {
        projectDir.mkdir("Src");
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
    
    // 创建Makefile文件
    QFile file(m_projectPath + "/Makefile");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "无法创建Makefile文件";
        return false;
    }
    
    QTextStream out(&file);
    
    // 写入Makefile内容
    out << "# 自动生成的Makefile\n\n";
    out << "# 工具链设置\n";
    out << "TOOLCHAIN_PATH = " << m_toolchainPath << "\n";
    out << "CC = $(TOOLCHAIN_PATH)/bin/arm-none-eabi-gcc\n";
    out << "CXX = $(TOOLCHAIN_PATH)/bin/arm-none-eabi-g++\n";
    out << "LD = $(TOOLCHAIN_PATH)/bin/arm-none-eabi-ld\n";
    out << "OBJCOPY = $(TOOLCHAIN_PATH)/bin/arm-none-eabi-objcopy\n";
    out << "SIZE = $(TOOLCHAIN_PATH)/bin/arm-none-eabi-size\n\n";
    
    out << "# 目标芯片设置\n";
    out << "TARGET = " << m_targetChip << "\n";
    out << "BUILD_DIR = build\n\n";
    
    out << "# 编译标志\n";
    out << "CFLAGS = -mcpu=cortex-m3 -mthumb -Wall -g -O0\n";
    out << "CFLAGS += -DSTM32F103xB\n\n";
    
    out << "# 包含路径\n";
    out << "INCLUDES = -I.\n";
    out << "INCLUDES += -I./Inc\n";
    out << "INCLUDES += -I./Drivers/CMSIS/Include\n";
    out << "INCLUDES += -I./Drivers/CMSIS/Device/ST/STM32F1xx/Include\n";
    out << "INCLUDES += -I./Drivers/STM32F1xx_HAL_Driver/Inc\n\n";
    
    out << "# 源文件\n";
    out << "C_SOURCES = $(wildcard Src/*.c)\n";
    out << "C_SOURCES += $(wildcard Drivers/STM32F1xx_HAL_Driver/Src/*.c)\n\n";
    
    out << "# 目标文件\n";
    out << "OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))\n\n";
    
    out << "# 链接脚本\n";
    out << "LDSCRIPT = STM32F103C8Tx_FLASH.ld\n\n";
    
    out << "# 默认目标\n";
    out << "all: $(BUILD_DIR) $(BUILD_DIR)/firmware.elf $(BUILD_DIR)/firmware.hex $(BUILD_DIR)/firmware.bin\n\n";
    
    out << "# 创建构建目录\n";
    out << "$(BUILD_DIR):\n";
    out << "\tmkdir -p $@\n\n";
    
    out << "# 编译规则\n";
    out << "$(BUILD_DIR)/%.o: Src/%.c\n";
    out << "\t$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@\n\n";
    
    out << "$(BUILD_DIR)/%.o: Drivers/STM32F1xx_HAL_Driver/Src/%.c\n";
    out << "\t$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@\n\n";
    
    out << "# 链接规则\n";
    out << "$(BUILD_DIR)/firmware.elf: $(OBJECTS)\n";
    out << "\t$(CC) $(CFLAGS) -T$(LDSCRIPT) $(OBJECTS) -o $@ -lc -lm -lnosys\n";
    out << "\t$(SIZE) $@\n\n";
    
    out << "# 生成hex文件\n";
    out << "$(BUILD_DIR)/firmware.hex: $(BUILD_DIR)/firmware.elf\n";
    out << "\t$(OBJCOPY) -O ihex $< $@\n\n";
    
    out << "# 生成bin文件\n";
    out << "$(BUILD_DIR)/firmware.bin: $(BUILD_DIR)/firmware.elf\n";
    out << "\t$(OBJCOPY) -O binary $< $@\n\n";
    
    out << "# 清理\n";
    out << "clean:\n";
    out << "\trm -rf $(BUILD_DIR)\n\n";
    
    out << "# 烧录\n";
    out << "flash: $(BUILD_DIR)/firmware.bin\n";
    out << "\tst-flash write $< 0x8000000\n\n";
    
    out << ".PHONY: all clean flash\n";
    
    file.close();
    return true;
}

// 修改buildProject方法以支持STM32工程
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
    QString fileName = fileInfo.fileName();
    QString baseName = fileInfo.baseName();
    QString outputFile = m_outputPath + "/" + baseName + ".o";
    
    // 根据文件类型选择编译器
    if (extension == "c") {
        // 使用GCC编译C文件
        return QString("gcc -c -Wall -std=c11 -o \"%1\" \"%2\"").arg(outputFile, filePath);
    } else if (extension == "cpp") {
        // 使用G++编译C++文件
        return QString("g++ -c -Wall -std=c++11 -o \"%1\" \"%2\"").arg(outputFile, filePath);
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
    
    out << "  /* 移除无用的段 */\n";
    out << "  /DISCARD/ :\n";
    out << "  {\n";
    out << "    libc.a ( * )\n";
    out << "    libm.a ( * )\n";
    out << "    libgcc.a ( * )\n";
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
