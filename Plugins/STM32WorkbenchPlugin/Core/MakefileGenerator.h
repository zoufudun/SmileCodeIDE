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
    Debug,      // 默认 -O1 -g3 -gdwarf-2 (兼顾软延时准确与调试)
    Release,    // -O2 -g0
    SizeOpt     // -Os -g0
};

/**
 * @brief 细粒度编译器优化与构建选项结构
 */
struct CompilerBuildOptions {
    QString optLevel = QStringLiteral("-O1");              // -O0, -O1, -O2, -O3, -Os, -Og
    QString debugLevel = QStringLiteral("-g3 -gdwarf-2");  // -g3 -gdwarf-2, -g, -g0
    bool enableToleranceFlags = true;                      // -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types
    QString customCFlags;                                  // 用户自定义额外编译参数

    QString toOptimizationFlag() const {
        return optLevel.trimmed().isEmpty() ? QStringLiteral("-O1") : optLevel.trimmed();
    }
    QString toDebugFlag() const {
        return debugLevel.trimmed();
    }
    QString toFullFlags() const {
        QStringList list;
        list << toOptimizationFlag();
        if (!toDebugFlag().isEmpty()) list << toDebugFlag();
        if (enableToleranceFlags) {
            list << QStringLiteral("-Wno-error=implicit-function-declaration")
                 << QStringLiteral("-Wno-error=incompatible-pointer-types");
        }
        if (!customCFlags.trimmed().isEmpty()) {
            list << customCFlags.trimmed();
        }
        return list.join(QLatin1Char(' '));
    }
};

/**
 * @brief 高性能工程通用 Makefile 自动化构建脚本生成引擎
 */
class MakefileGenerator
{
public:
    MakefileGenerator();

    static CompilerBuildOptions defaultOptionsForConfig(BuildConfiguration config);

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
                         QString *outError = nullptr,
                         const CompilerBuildOptions &buildOptions = CompilerBuildOptions());

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
                                          const QStringList &defines,
                                          const CompilerBuildOptions &buildOptions = CompilerBuildOptions());
};

#endif // MAKEFILEGENERATOR_H
