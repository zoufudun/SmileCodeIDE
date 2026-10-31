#include "MakefileGenerator.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDirIterator>
#include <QSet>
#include <QXmlStreamReader>
#include <QCryptographicHash>
#include <QDateTime>
#include <QSaveFile>
#include <QRegularExpression>

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

static bool scanCubeIdeProject(const QString &projectDir, const QString &context,
                               QStringList &cSources, QStringList &cppSources,
                               QStringList &asmSources, QStringList &includes)
{
    const QDir root(projectDir);
    QDirIterator projectFiles(projectDir, {".project"}, QDir::Files | QDir::Hidden,
                              QDirIterator::Subdirectories);
    QString projectFile;
    while (projectFiles.hasNext()) {
        const QString candidate = projectFiles.next();
        const QString folder = QDir::fromNativeSeparators(QFileInfo(candidate).absolutePath());
        if (!folder.contains("/STM32CubeIDE/", Qt::CaseInsensitive) &&
            !folder.endsWith("/STM32CubeIDE", Qt::CaseInsensitive)) continue;
        if (!context.isEmpty() && !folder.endsWith("/" + context, Qt::CaseInsensitive))
            continue;
        projectFile = candidate;
        break;
    }
    if (projectFile.isEmpty()) return false;
    const QDir projectFolder(QFileInfo(projectFile).absolutePath());
    const auto relativeInsideRoot = [&](const QString &absolute) -> QString {
        const QString relative = QDir::fromNativeSeparators(root.relativeFilePath(absolute));
        return relative == ".." || relative.startsWith("../") ? QString() : relative;
    };
    const auto addSource = [&](const QString &absolute) {
        const QFileInfo info(absolute);
        if (!info.isFile()) return;
        const QString relative = relativeInsideRoot(info.absoluteFilePath());
        if (relative.isEmpty()) return;
        const QString suffix = info.suffix().toLower();
        if (suffix == "c" && !cSources.contains(relative)) cSources << relative;
        else if ((suffix == "cpp" || suffix == "cc" || suffix == "cxx") &&
                 !cppSources.contains(relative)) cppSources << relative;
        else if ((suffix == "s" || suffix == "asm") && !asmSources.contains(relative))
            asmSources << relative;
    };
    QFile file(projectFile);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QXmlStreamReader xml(&file);
    const QRegularExpression parentUri("^PARENT-(\\d+)-PROJECT_LOC/(.+)$");
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != "locationURI") continue;
        const QString uri = xml.readElementText();
        const auto match = parentUri.match(uri);
        if (!match.hasMatch()) continue;
        QDir base = projectFolder;
        const int parents = match.captured(1).toInt();
        bool valid = true;
        for (int i = 0; i < parents; ++i) valid = base.cdUp() && valid;
        if (valid) addSource(base.absoluteFilePath(match.captured(2)));
    }
    if (xml.hasError()) return false;

    QDirIterator localSources(projectFolder.absolutePath(),
                              {"*.c", "*.cpp", "*.cc", "*.cxx", "*.s", "*.S", "*.asm"},
                              QDir::Files, QDirIterator::Subdirectories);
    while (localSources.hasNext()) {
        const QString source = localSources.next();
        const QString local = QDir::fromNativeSeparators(projectFolder.relativeFilePath(source));
        if (local.startsWith("Debug/", Qt::CaseInsensitive) ||
            local.startsWith("Release/", Qt::CaseInsensitive)) continue;
        addSource(source);
    }
    if (cSources.isEmpty() && cppSources.isEmpty() && asmSources.isEmpty()) return false;

    QSet<QString> includeSet = {"."};
    QFile cproject(projectFolder.filePath(".cproject"));
    if (cproject.open(QIODevice::ReadOnly)) {
        QXmlStreamReader settings(&cproject);
        bool inIncludes = false;
        while (!settings.atEnd()) {
            settings.readNext();
            if (settings.isStartElement() && settings.name() == "option")
                inIncludes = settings.attributes().value("superClass").toString()
                    .endsWith(".tool.c.compiler.option.includepaths");
            else if (settings.isEndElement() && settings.name() == "option")
                inIncludes = false;
            else if (inIncludes && settings.isStartElement() &&
                     settings.name() == "listOptionValue") {
                const QString value = settings.attributes().value("value").toString();
                if (value.contains("${")) continue;
                // CubeIDE stores include paths relative to its Debug build folder.
                const QString absolute = QDir::cleanPath(
                    projectFolder.absoluteFilePath("Debug/" + value));
                const QString relative = relativeInsideRoot(absolute);
                if (!relative.isEmpty() && QDir(absolute).exists()) includeSet.insert(relative);
            }
        }
    }
    includes = includeSet.values();
    includes.sort();
    return true;
}

MakefileGenerator::MakefileGenerator()
{
}

bool MakefileGenerator::migrateLegacyLinkerScript(const QString &projectDir,
                                                  const STM32ChipProfile &profile,
                                                  QString *outBackupPath,
                                                  QString *outError)
{
    if (outBackupPath) outBackupPath->clear();
    if (profile.defaultLdFile.isEmpty() || profile.linkerResourcePath.isEmpty()) {
        if (outError) *outError = "此型号没有可用于迁移的 CubeMX 原版链接脚本。";
        return false;
    }
    const QString linkerPath = QDir(projectDir).absoluteFilePath(profile.defaultLdFile);
    QFile existing(linkerPath);
    if (!existing.open(QIODevice::ReadOnly)) {
        if (outError) *outError = "无法读取待迁移的链接脚本: " + linkerPath;
        return false;
    }
    const QByteArray oldBytes = existing.readAll();
    existing.close();
    if (!oldBytes.left(512).contains("SmileCodeIDE Auto-Generated Linker Script")) {
        if (outError) *outError = "目标不是旧版自动生成的链接脚本，已保留原文件: " + linkerPath;
        return false;
    }

    QFile resource(profile.linkerResourcePath);
    if (!resource.open(QIODevice::ReadOnly)) {
        if (outError) *outError = "无法读取 CubeMX 链接脚本: " + profile.linkerResourcePath;
        return false;
    }
    const QByteArray newBytes = resource.readAll();
    const QString actualHash = QString::fromLatin1(
        QCryptographicHash::hash(newBytes, QCryptographicHash::Sha256).toHex());
    if (profile.linkerSha256.isEmpty() ||
        actualHash.compare(profile.linkerSha256, Qt::CaseInsensitive) != 0) {
        if (outError) *outError = "内置 CubeMX 链接脚本 SHA-256 校验失败: " +
            profile.linkerResourcePath;
        return false;
    }

    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString backupPath = linkerPath + ".legacy-" + stamp + ".bak";
    for (int suffix = 1; QFileInfo::exists(backupPath); ++suffix)
        backupPath = linkerPath + ".legacy-" + stamp + "-" + QString::number(suffix) + ".bak";
    QSaveFile backup(backupPath);
    if (!backup.open(QIODevice::WriteOnly) || backup.write(oldBytes) != oldBytes.size() ||
        !backup.commit()) {
        if (outError) *outError = "无法创建旧链接脚本备份，原文件未修改: " + backupPath;
        return false;
    }

    QSaveFile replacement(linkerPath);
    if (!replacement.open(QIODevice::WriteOnly) ||
        replacement.write(newBytes) != newBytes.size() || !replacement.commit()) {
        if (outError) *outError = "备份已创建，但无法安装 CubeMX 链接脚本: " + linkerPath;
        return false;
    }
    if (outBackupPath) *outBackupPath = backupPath;
    return true;
}

void MakefileGenerator::scanProjectSources(const QString &projectDir,
                                          QStringList &outCSources,
                                          QStringList &outCppSources,
                                          QStringList &outAsmSources,
                                          QStringList &outIncludes,
                                          const QString &linkerContext)
{
    outCSources.clear();
    outCppSources.clear();
    outAsmSources.clear();
    outIncludes.clear();

    if (scanKeilProject(projectDir, outCSources, outCppSources, outAsmSources, outIncludes))
        return;
    if (scanCubeIdeProject(projectDir, linkerContext, outCSources, outCppSources,
                           outAsmSources, outIncludes)) return;

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
        if (relPath.startsWith("build/") || relPath.startsWith(".smilecode/") || relPath.startsWith(".git/") ||
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

CompilerBuildOptions MakefileGenerator::defaultOptionsForConfig(BuildConfiguration config)
{
    CompilerBuildOptions opt;
    switch (config) {
        case BuildConfiguration::Release:
            opt.optLevel = QStringLiteral("-O2");
            opt.debugLevel = QStringLiteral("-g0");
            break;
        case BuildConfiguration::SizeOpt:
            opt.optLevel = QStringLiteral("-Os");
            opt.debugLevel = QStringLiteral("-g0");
            break;
        case BuildConfiguration::Debug:
        default:
            opt.optLevel = QStringLiteral("-O1");
            opt.debugLevel = QStringLiteral("-g3 -gdwarf-2");
            break;
    }
    opt.enableToleranceFlags = true;
    return opt;
}

bool MakefileGenerator::generate(const QString &projectDir,
                                const STM32ChipProfile &profile,
                                const ToolchainPaths &paths,
                                BuildConfiguration config,
                                const QString &customLdFile,
                                const QStringList &extraDefines,
                                const QStringList &extraIncludes,
                                QString *outError,
                                const CompilerBuildOptions &buildOptions)
{
    QDir dir(projectDir);
    if (!dir.exists()) {
        if (outError) *outError = "工程目录不存在: " + projectDir;
        return false;
    }

    // 1. 扫描源文件
    QStringList cSources, cppSources, asmSources, includes;
    scanProjectSources(projectDir, cSources, cppSources, asmSources, includes,
                       profile.linkerContext);
    if (!profile.linkerContext.isEmpty()) {
        const QString otherCore = profile.linkerContext.compare("CM4", Qt::CaseInsensitive) == 0
                                      ? "CM7" : "CM4";
        const auto fromOtherCore = [&](const QString &path) {
            return path.compare(otherCore, Qt::CaseInsensitive) == 0 ||
                   path.startsWith(otherCore + "/", Qt::CaseInsensitive) ||
                   path.contains("/" + otherCore + "/", Qt::CaseInsensitive);
        };
        cSources.removeIf(fromOtherCore);
        cppSources.removeIf(fromOtherCore);
        asmSources.removeIf(fromOtherCore);
        includes.removeIf(fromOtherCore);
    }

    // Keil examples may ship only ARMASM/IAR startup code. GCC cannot assemble
    // it; prefer the project's GCC sibling, then an exact bundled ST variant.
    for (QString &source : asmSources) {
        const QString normalized = QDir::fromNativeSeparators(source);
        if (!QFileInfo(source).fileName().startsWith("startup_", Qt::CaseInsensitive) ||
            (!normalized.contains("/arm/", Qt::CaseInsensitive) &&
             !normalized.contains("/iar/", Qt::CaseInsensitive))) continue;
        QString gccSource = normalized;
        gccSource.replace("/arm/", "/gcc/", Qt::CaseInsensitive);
        gccSource.replace("/iar/", "/gcc/", Qt::CaseInsensitive);
        if (QFileInfo::exists(dir.absoluteFilePath(gccSource))) {
            source = gccSource;
            continue;
        }
        const QString fileName = QFileInfo(source).fileName();
        QFile resource(":/stm32/startup/" + fileName);
        const QString expectedName = "startup_" + profile.defaultMacro.toLower() + ".s";
        if (fileName.compare(expectedName, Qt::CaseInsensitive) != 0 ||
            !resource.open(QIODevice::ReadOnly)) {
            if (outError) *outError = "启动文件为 Keil/IAR 格式，缺少当前芯片对应的 GCC 启动文件: " + source;
            return false;
        }
        const QByteArray contents = resource.readAll();
        const QString destination = ".smilecode/startup/" + fileName;
        const QString absolute = dir.absoluteFilePath(destination);
        QFile existing(absolute);
        const bool unchanged = existing.open(QIODevice::ReadOnly) && existing.readAll() == contents;
        existing.close();
        if (!unchanged) {
            QSaveFile output(absolute);
            if (!dir.mkpath(".smilecode/startup") || !output.open(QIODevice::WriteOnly) ||
                output.write(contents) != contents.size() || !output.commit()) {
                if (outError) *outError = "无法安装 GCC 启动文件: " + absolute;
                return false;
            }
        }
        source = destination;
    }

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
        const auto writeDefaultScript = [&](const QString &path) -> bool {
            QByteArray contents;
            if (!profile.linkerResourcePath.isEmpty()) {
                QFile resource(profile.linkerResourcePath);
                if (!resource.open(QIODevice::ReadOnly)) {
                    if (outError) *outError = "无法读取 CubeMX 链接脚本: " + profile.linkerResourcePath;
                    return false;
                }
                contents = resource.readAll();
                const QString actualHash = QString::fromLatin1(
                    QCryptographicHash::hash(contents, QCryptographicHash::Sha256).toHex());
                if (profile.linkerSha256.isEmpty() ||
                    actualHash.compare(profile.linkerSha256, Qt::CaseInsensitive) != 0) {
                    if (outError) *outError = "内置 CubeMX 链接脚本 SHA-256 校验失败: " +
                        profile.linkerResourcePath;
                    return false;
                }
            } else {
                if (outError) *outError = "此型号尚无打包的 CubeMX 原版链接脚本，请在工程中提供 .ld 文件并明确选择。";
                return false;
            }
            if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
                if (outError) *outError = "无法创建链接脚本目录: " + QFileInfo(path).absolutePath();
                return false;
            }
            QFile linker(path);
            if (!linker.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
                linker.write(contents) != contents.size()) {
                if (outError) *outError = "无法写入链接脚本: " + path;
                return false;
            }
            return true;
        };
        const QString generatedName = profile.defaultLdFile.isEmpty()
                                          ? (profile.chipName + "_FLASH.ld")
                                          : profile.defaultLdFile;
        const QString generatedPath = dir.absoluteFilePath(generatedName);

        // 只考虑当前芯片或未标注芯片的候选；多个候选需要用户显式选择。
        QStringList ldFiles;
        for (const QString &candidate : dir.entryList(QStringList() << "*.ld", QDir::Files)) {
            if (!profile.linkerContext.isEmpty()) continue;
            if (candidate.startsWith("STM32", Qt::CaseInsensitive) &&
                !candidate.startsWith(profile.chipName, Qt::CaseInsensitive))
                continue;
            ldFiles << candidate;
        }
        if (ldFiles.size() > 1) {
            if (outError) *outError = "找到多个可用链接脚本，请在工程配置中明确指定 .ld 文件: " +
                ldFiles.join(", ");
            return false;
        }
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
                if (outError) *outError = "已有旧版自动生成的链接脚本，需先迁移或备份并明确选择: " +
                    generatedPath;
                return false;
            }
        } else {
            if (ldFiles.size() == 1) {
                ldFileName = ldFiles.first();
            } else {
                // 自动生成链接脚本
                ldFileName = generatedName;
                if (!writeDefaultScript(dir.absoluteFilePath(ldFileName))) return false;
            }
        }
    }

    // 4. 生成 Makefile 内容
    QString content = generateMakefileContent(projectDir, profile, paths, config, ldFileName,
                                            cSources, cppSources, asmSources, includes, defines,
                                            buildOptions);

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
                                                  const QStringList &defines,
                                                  const CompilerBuildOptions &buildOptions)
{
    Q_UNUSED(projectDir);
    QString out;
    QTextStream stream(&out);

    stream << "################################################################################\n";
    stream << "# SmileCodeIDE STM32 Makefile - Auto Generated\n";
    stream << "# Target Chip: " << profile.chipName << " (" << profile.family << " / " << profile.core << ")\n";
    stream << "################################################################################\n\n";

    // 目标与目录定义
    stream << "TARGET = " << profile.artifactBaseName() << "\n";
    stream << "BUILD_DIR = " << profile.buildDirectory() << "\n\n";

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
    CompilerBuildOptions finalOpts = buildOptions;
    if (finalOpts.optLevel.trimmed().isEmpty()) {
        finalOpts = defaultOptionsForConfig(config);
    }
    const QString optFlag = finalOpts.toOptimizationFlag();
    const QString debugFlag = finalOpts.toDebugFlag();
    QStringList cflagsList;
    cflagsList << "$(MCU)" << "$(C_DEFS)" << "$(C_INCLUDES)" << optFlag;
    if (!debugFlag.isEmpty()) cflagsList << debugFlag;
    cflagsList << "-Wall" << "-fdata-sections" << "-ffunction-sections";
    if (finalOpts.enableToleranceFlags) {
        cflagsList << "-Wno-error=implicit-function-declaration" << "-Wno-error=incompatible-pointer-types";
    }
    if (!finalOpts.customCFlags.trimmed().isEmpty()) {
        cflagsList << finalOpts.customCFlags.trimmed();
    }

    QString asOpt = optFlag;
    if (!debugFlag.isEmpty()) asOpt += " " + debugFlag;

    stream << "CFLAGS = " << cflagsList.join(" ") << "\n";
    stream << "CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti\n";
    stream << "ASFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) " << asOpt << " -Wall -fdata-sections -ffunction-sections\n\n";

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

    // Keep each object tied to the scanned source. MinGW vpath lookup can fail
    // for deep Windows paths, or silently pick a different file with the same name.
    stream << "# Object File Mapping\n";
    stream << "OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))\n";
    stream << "OBJECTS += $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $(notdir $(CXX_SOURCES)))))\n";
    stream << "OBJECTS += $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $(notdir $(ASM_SOURCES)))))\n\n";

    // 构建规则
    stream << "# Build Targets\n";
    stream << "all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin\n\n";

    stream << "$(BUILD_DIR):\n";
    stream << "\t$(MKDIR)\n\n";

    const auto writeSourceRules = [&](const QStringList &sources, const QString &compiler,
                                      const QString &flags) {
        for (const QString &source : sources) {
            stream << "$(BUILD_DIR)/" << QFileInfo(source).completeBaseName()
                   << ".o: " << source << " | $(BUILD_DIR)\n";
            stream << "\t@echo [BUILD:" << compiler << "] \"$<\"\n";
            stream << "\t$(" << compiler << ") -c $(" << flags << ") \"$<\" -o \"$@\"\n\n";
        }
    };
    writeSourceRules(cSources, "CC", "CFLAGS");
    writeSourceRules(cppSources, "CXX", "CXXFLAGS");
    writeSourceRules(asmSources, "AS", "ASFLAGS");

    stream << "$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)\n";
    stream << "\t@echo [BUILD:LD] \"$@\"\n";
    stream << "\t$(CC) $(OBJECTS) $(LDFLAGS) -o $@\n";
    stream << "\t$(SZ) $@\n\n";

    stream << "$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)\n";
    stream << "\t@echo [BUILD:HEX] \"$@\"\n";
    stream << "\t$(HEX) $< $@\n\n";

    stream << "$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)\n";
    stream << "\t@echo [BUILD:BIN] \"$@\"\n";
    stream << "\t$(BIN) $< $@\n\n";

    stream << "clean:\n";
    stream << "\t$(RM) $(BUILD_DIR)\n\n";

    stream << "-include $(wildcard $(BUILD_DIR)/*.d)\n\n";
    stream << ".PHONY: all clean\n";

    return out;
}
