#include "MakefileGenerator.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDirIterator>
#include <QSet>
#include <QXmlStreamReader>

static bool scanKeilProject(const QString &projectDir, QStringList &cSources,
                            QStringList &cppSources, QStringList &asmSources,
                            QStringList &includes)
{
    QDir root(projectDir);
    QDirIterator projects(projectDir, {"*.uvprojx"}, QDir::Files,
                          QDirIterator::Subdirectories);
    while (projects.hasNext()) {
        const QString projectFile = projects.next();
        QFile file(projectFile);
        if (!file.open(QIODevice::ReadOnly)) continue;
        QXmlStreamReader xml(&file);
        QSet<QString> includeSet = {"."};
        QStringList c, cpp, assembly;
        const QDir projectFolder(QFileInfo(projectFile).absolutePath());
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) continue;
            const QString name = xml.name().toString();
            if (name == "IncludePath") {
                const QStringList paths = xml.readElementText().split(';', Qt::SkipEmptyParts);
                for (const QString &path : paths) {
                    const QString absolute = QDir::cleanPath(projectFolder.absoluteFilePath(
                        QDir::fromNativeSeparators(path.trimmed())));
                    if (QDir(absolute).exists())
                        includeSet.insert(QDir::fromNativeSeparators(root.relativeFilePath(absolute)));
                }
            } else if (name == "FilePath") {
                QString absolute = QDir::cleanPath(projectFolder.absoluteFilePath(
                    QDir::fromNativeSeparators(xml.readElementText().trimmed())));
                const QFileInfo sourceInfo(absolute);
                if (!sourceInfo.exists()) continue;
                const QString suffix = sourceInfo.suffix().toLower();
                if (suffix == "s" && sourceInfo.fileName().startsWith("startup_")) {
                    QString gcc = QDir::fromNativeSeparators(absolute);
                    gcc.replace("/Templates/arm/", "/Templates/gcc/", Qt::CaseInsensitive);
                    gcc.replace("/Templates/iar/", "/Templates/gcc/", Qt::CaseInsensitive);
                    if (QFileInfo::exists(gcc)) absolute = gcc;
                }
                const QString relative = QDir::fromNativeSeparators(root.relativeFilePath(absolute));
                if (suffix == "c" && !c.contains(relative)) c << relative;
                else if ((suffix == "cpp" || suffix == "cc" || suffix == "cxx") && !cpp.contains(relative)) cpp << relative;
                else if ((suffix == "s" || suffix == "asm") && !assembly.contains(relative)) assembly << relative;
            }
        }
        if (!xml.hasError() && (!c.isEmpty() || !cpp.isEmpty() || !assembly.isEmpty())) {
            cSources = c;
            cppSources = cpp;
            asmSources = assembly;
            includes = includeSet.values();
            includes.sort();
            return true;
        }
    }
    return false;
}

MakefileGenerator::MakefileGenerator()
{
}

void MakefileGenerator::scanProjectSources(const QString &projectDir,
                                          QStringList &outCSources,
                                          QStringList &outCppSources,
                                          QStringList &outAsmSources,
                                          QStringList &outIncludes)
{
    outCSources.clear();
    outCppSources.clear();
    outAsmSources.clear();
    outIncludes.clear();

    if (scanKeilProject(projectDir, outCSources, outCppSources, outAsmSources, outIncludes))
        return;

    QDir rootDir(projectDir);
    QSet<QString> includeSet;

    // 默认包含工程根目录
    includeSet.insert(".");

    // 递归遍历工程所有文件与目录
    QDirIterator it(projectDir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString fullPath = it.filePath();
        QString relPath = rootDir.relativeFilePath(fullPath).replace('\\', '/');

        // 忽略 build 输出目录、.git、.vscode、.qtcreator 等隐藏缓存
        if (relPath.startsWith("build/") || relPath.startsWith(".git/") ||
            relPath.startsWith(".vscode/") || relPath.startsWith("release/") ||
            relPath.startsWith("debug/")) {
            continue;
        }

        QString suffix = it.fileInfo().suffix().toLower();
        if (suffix == "c") {
            outCSources.append(relPath);
        } else if (suffix == "cpp" || suffix == "cc" || suffix == "cxx") {
            outCppSources.append(relPath);
        } else if (suffix == "s" || suffix == "asm") {
            outAsmSources.append(relPath);
        } else if (suffix == "h" || suffix == "hpp") {
            // 如果所在目录是头文件目录，加入 include 路径
            QString parentRel = rootDir.relativeFilePath(it.fileInfo().absolutePath()).replace('\\', '/');
            if (parentRel.isEmpty()) parentRel = ".";
            includeSet.insert(parentRel);
        }
    }

    // 常见结构目录保底包含
    if (rootDir.exists("Inc")) includeSet.insert("Inc");
    if (rootDir.exists("Core/Inc")) includeSet.insert("Core/Inc");
    if (rootDir.exists("Drivers/CMSIS/Include")) includeSet.insert("Drivers/CMSIS/Include");

    outIncludes = includeSet.values();
    outIncludes.sort();
}

bool MakefileGenerator::generate(const QString &projectDir,
                                const STM32ChipProfile &profile,
                                const ToolchainPaths &paths,
                                BuildConfiguration config,
                                const QString &customLdFile,
                                const QStringList &extraDefines,
                                const QStringList &extraIncludes,
                                QString *outError)
{
    QDir dir(projectDir);
    if (!dir.exists()) {
        if (outError) *outError = "工程目录不存在: " + projectDir;
        return false;
    }

    // 1. 扫描源文件
    QStringList cSources, cppSources, asmSources, includes;
    scanProjectSources(projectDir, cSources, cppSources, asmSources, includes);

    // 2. 合并额外包含路径和宏
    for (const QString &inc : extraIncludes) {
        if (!includes.contains(inc)) includes.append(inc);
    }

    QStringList defines = profile.extraMacros;
    if (!profile.defaultMacro.isEmpty() && !defines.contains(profile.defaultMacro)) {
        defines.prepend(profile.defaultMacro);
    }
    for (const QString &def : extraDefines) {
        if (!defines.contains(def)) defines.append(def);
    }

    // 3. 处理链接脚本
    QString ldFileName = customLdFile;
    if (ldFileName.isEmpty()) {
        const QString generatedName = profile.defaultLdFile.isEmpty()
                                          ? (profile.chipName + "_FLASH.ld")
                                          : profile.defaultLdFile;
        const QString generatedPath = dir.absoluteFilePath(generatedName);

        // 精确匹配默认文件名。SmileCodeIDE 生成的旧脚本应随芯片档案刷新，
        // 用户自行维护的链接脚本则保持原样。
        if (QFileInfo::exists(generatedPath)) {
            ldFileName = generatedName;
            QFile existing(generatedPath);
            bool autoGenerated = false;
            if (existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
                autoGenerated = existing.read(512).contains(
                    "SmileCodeIDE Auto-Generated Linker Script");
                existing.close();
            }
            if (autoGenerated) {
                QFile ldFile(generatedPath);
                if (!ldFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                    if (outError) *outError = "无法更新自动生成的链接脚本: " + generatedPath;
                    return false;
                }
                QTextStream(&ldFile) << STM32ProfileManager::generateLinkerScript(profile);
            }
        } else {
            // 在工程根目录搜寻用户已有的 .ld 文件
            QStringList ldFiles = dir.entryList(QStringList() << "*.ld", QDir::Files);
            if (!ldFiles.isEmpty()) {
                ldFileName = ldFiles.first();
            } else {
                // 自动生成链接脚本
                ldFileName = generatedName;
                QString ldContent = STM32ProfileManager::generateLinkerScript(profile);
                QFile ldFile(dir.absoluteFilePath(ldFileName));
                if (!ldFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    if (outError) *outError = "无法生成链接脚本: " + ldFile.fileName();
                    return false;
                }
                QTextStream ldOut(&ldFile);
                ldOut << ldContent;
                ldFile.close();
            }
        }
    }

    // 4. 生成 Makefile 内容
    QString content = generateMakefileContent(projectDir, profile, paths, config, ldFileName,
                                            cSources, cppSources, asmSources, includes, defines);

    // 5. 写入 Makefile 文件
    QFile makefile(dir.absoluteFilePath("Makefile"));
    if (!makefile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (outError) *outError = "无法写入 Makefile 文件: " + makefile.errorString();
        return false;
    }

    QTextStream out(&makefile);
    out << content;
    makefile.close();

    return true;
}

QString MakefileGenerator::generateMakefileContent(const QString &projectDir,
                                                  const STM32ChipProfile &profile,
                                                  const ToolchainPaths &paths,
                                                  BuildConfiguration config,
                                                  const QString &ldFileName,
                                                  const QStringList &cSources,
                                                  const QStringList &cppSources,
                                                  const QStringList &asmSources,
                                                  const QStringList &includes,
                                                  const QStringList &defines)
{
    Q_UNUSED(projectDir);
    QString out;
    QTextStream stream(&out);

    stream << "################################################################################\n";
    stream << "# SmileCodeIDE STM32 Makefile - Auto Generated\n";
    stream << "# Target Chip: " << profile.chipName << " (" << profile.family << " / " << profile.core << ")\n";
    stream << "################################################################################\n\n";

    // 目标与目录定义
    QString targetName = profile.chipName.toLower();
    stream << "TARGET = " << targetName << "\n";
    stream << "BUILD_DIR = build\n\n";

    // 工具链前缀配置
    stream << "# Toolchain Paths\n";
    if (!paths.gccPath.isEmpty()) {
        QString gccBinDir = QFileInfo(paths.gccPath).absolutePath().replace('\\', '/');
        stream << "BIN_DIR = " << gccBinDir << "/\n";
        stream << "PREFIX = $(BIN_DIR)arm-none-eabi-\n";
        stream << "CC = \"$(PREFIX)gcc\"\n";
        stream << "CXX = \"$(PREFIX)g++\"\n";
        stream << "AS = \"$(PREFIX)gcc\" -x assembler-with-cpp\n";
        stream << "CP = \"$(PREFIX)objcopy\"\n";
        stream << "SZ = \"$(PREFIX)size\"\n";
    } else {
        stream << "PREFIX = arm-none-eabi-\n";
        stream << "CC = $(PREFIX)gcc\n";
        stream << "CXX = $(PREFIX)g++\n";
        stream << "AS = $(PREFIX)gcc -x assembler-with-cpp\n";
        stream << "CP = $(PREFIX)objcopy\n";
        stream << "SZ = $(PREFIX)size\n";
    }
    stream << "HEX = $(CP) -O ihex\n";
    stream << "BIN = $(CP) -O binary -S\n\n";

    // 平台命令兼容 (Windows / Linux)
    stream << "# Platform shell commands\n";
    stream << "ifeq ($(OS),Windows_NT)\n";
    stream << "  RM = cmd /c rd /s /q\n";
    stream << "  MKDIR = cmd /c if not exist \"$(BUILD_DIR)\" mkdir \"$(BUILD_DIR)\"\n";
    stream << "else\n";
    stream << "  RM = rm -rf\n";
    stream << "  MKDIR = mkdir -p $(BUILD_DIR)\n";
    stream << "endif\n\n";

    // MCU 编译架构标志
    stream << "# MCU Architecture Flags\n";
    stream << "CPU = -mcpu=" << profile.core << "\n";
    if (profile.hasFpu && !profile.fpuFlags.isEmpty()) {
        stream << "FPU = " << profile.fpuFlags << "\n";
        stream << "MCU = $(CPU) -mthumb $(FPU)\n\n";
    } else {
        stream << "MCU = $(CPU) -mthumb\n\n";
    }

    // 宏定义
    stream << "# C / C++ Defines\n";
    stream << "C_DEFS =";
    for (const QString &def : defines) {
        stream << " -D" << def;
    }
    stream << "\n\n";

    // 包含路径
    stream << "# Header Includes\n";
    stream << "C_INCLUDES =";
    for (const QString &inc : includes) {
        stream << " \\\n  -I" << inc;
    }
    stream << "\n\n";

    // 编译优化与选项
    stream << "# Compiler Flags\n";
    QString optFlag;
    switch (config) {
        case BuildConfiguration::Release: optFlag = "-O2 -g0"; break;
        case BuildConfiguration::SizeOpt: optFlag = "-Os -g0"; break;
        case BuildConfiguration::Debug:
        default:                          optFlag = "-O0 -g3 -gdwarf-2"; break;
    }

    stream << "CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) " << optFlag << " -Wall -fdata-sections -ffunction-sections\n";
    stream << "CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti\n";
    stream << "ASFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) " << optFlag << " -Wall -fdata-sections -ffunction-sections\n\n";

    // 自动依赖生成标志 (修改头文件只重编相关C文件)
    stream << "# Generate dependency files for incremental builds\n";
    stream << "CFLAGS += -MMD -MP -MF\"$(@:%.o=%.d)\"\n";
    stream << "CXXFLAGS += -MMD -MP -MF\"$(@:%.o=%.d)\"\n\n";

    // 链接配置
    stream << "# Linker Script & Flags\n";
    stream << "LDSCRIPT = " << ldFileName << "\n";
    stream << "LIBS = -lc -lm -lnosys\n";
    stream << "LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections\n\n";

    // 源文件列表
    stream << "# C Sources\n";
    stream << "C_SOURCES =";
    for (const QString &c : cSources) {
        stream << " \\\n  " << c;
    }
    stream << "\n\n";

    stream << "# C++ Sources\n";
    stream << "CXX_SOURCES =";
    for (const QString &cpp : cppSources) {
        stream << " \\\n  " << cpp;
    }
    stream << "\n\n";

    stream << "# Assembler Sources\n";
    stream << "ASM_SOURCES =";
    for (const QString &s : asmSources) {
        stream << " \\\n  " << s;
    }
    stream << "\n\n";

    // 对象列表与 vpath 路径推导
    stream << "# Object File Mapping\n";
    stream << "OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))\n";
    stream << "OBJECTS += $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $(notdir $(CXX_SOURCES)))))\n";
    stream << "OBJECTS += $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $(notdir $(ASM_SOURCES)))))\n\n";

    stream << "vpath %.c $(sort $(dir $(C_SOURCES)))\n";
    if (!cppSources.isEmpty()) {
        stream << "vpath %.cpp $(sort $(dir $(CXX_SOURCES)))\n";
    }
    if (!asmSources.isEmpty()) {
        stream << "vpath %.s $(sort $(dir $(ASM_SOURCES)))\n";
        stream << "vpath %.S $(sort $(dir $(ASM_SOURCES)))\n";
        stream << "vpath %.asm $(sort $(dir $(ASM_SOURCES)))\n";
    }
    stream << "\n";

    // 构建规则
    stream << "# Build Targets\n";
    stream << "all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin\n\n";

    stream << "$(BUILD_DIR):\n";
    stream << "\t$(MKDIR)\n\n";

    stream << "$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)\n";
    stream << "\t$(CC) -c $(CFLAGS) $< -o $@\n\n";

    stream << "$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)\n";
    stream << "\t$(CXX) -c $(CXXFLAGS) $< -o $@\n\n";

    stream << "$(BUILD_DIR)/%.o: %.cc | $(BUILD_DIR)\n";
    stream << "\t$(CXX) -c $(CXXFLAGS) $< -o $@\n\n";

    stream << "$(BUILD_DIR)/%.o: %.cxx | $(BUILD_DIR)\n";
    stream << "\t$(CXX) -c $(CXXFLAGS) $< -o $@\n\n";

    stream << "$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)\n";
    stream << "\t$(AS) -c $(ASFLAGS) $< -o $@\n\n";

    stream << "$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)\n";
    stream << "\t$(AS) -c $(ASFLAGS) $< -o $@\n\n";

    stream << "$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)\n";
    stream << "\t$(AS) -c $(ASFLAGS) $< -o $@\n\n";

    stream << "$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)\n";
    stream << "\t$(CC) $(OBJECTS) $(LDFLAGS) -o $@\n";
    stream << "\t$(SZ) $@\n\n";

    stream << "$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)\n";
    stream << "\t$(HEX) $< $@\n\n";

    stream << "$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)\n";
    stream << "\t$(BIN) $< $@\n\n";

    stream << "clean:\n";
    stream << "\t$(RM) $(BUILD_DIR)\n\n";

    stream << "-include $(wildcard $(BUILD_DIR)/*.d)\n\n";
    stream << ".PHONY: all clean\n";

    return out;
}
