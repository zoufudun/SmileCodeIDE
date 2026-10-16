#ifndef MAKEFILEGENERATOR_H
#define MAKEFILEGENERATOR_H

#include <QString>
#include <QStringList>
#include <QDir>
#include "STM32Profile.h"
#include "ToolchainDetector.h"

/**
 * @brief 构建模式
 */
enum class BuildConfiguration {
    Debug,      // -O0 -g3
    Release,    // -O2 -g0
    SizeOpt     // -Os -g0
};

/**
 * @brief 高性能工程通用 Makefile 自动化构建脚本生成引擎
 */
class MakefileGenerator
{
public:
    MakefileGenerator();

    // 扫描工程根目录下所有可用源文件与包含路径
    static void scanProjectSources(const QString &projectDir,
                                  QStringList &outCSources,
                                  QStringList &outCppSources,
                                  QStringList &outAsmSources,
                                  QStringList &outIncludes,
                                  const QString &linkerContext = QString());

    // 为指定 STM32 工程生成生产级 Makefile
    static bool generate(const QString &projectDir,
                         const STM32ChipProfile &profile,
                         const ToolchainPaths &paths,
                         BuildConfiguration config = BuildConfiguration::Debug,
                         const QString &customLdFile = QString(),
                         const QStringList &extraDefines = QStringList(),
                         const QStringList &extraIncludes = QStringList(),
                         QString *outError = nullptr);

    // Explicitly back up and replace a recognized legacy generated linker script.
    static bool migrateLegacyLinkerScript(const QString &projectDir,
                                          const STM32ChipProfile &profile,
                                          QString *outBackupPath = nullptr,
                                          QString *outError = nullptr);

    // 格式化输出 Makefile 文本内容
    static QString generateMakefileContent(const QString &projectDir,
                                          const STM32ChipProfile &profile,
                                          const ToolchainPaths &paths,
                                          BuildConfiguration config,
                                          const QString &ldFileName,
                                          const QStringList &cSources,
                                          const QStringList &cppSources,
                                          const QStringList &asmSources,
                                          const QStringList &includes,
                                          const QStringList &defines);
};

#endif // MAKEFILEGENERATOR_H
