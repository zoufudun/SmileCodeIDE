#ifndef BUILDMANAGER_H
#define BUILDMANAGER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QThread>
#include "ToolchainDetector.h"
#include "STM32Profile.h"
#include "MakefileGenerator.h"

/**
 * @brief 编译诊断信息级别
 */
enum class DiagnosticSeverity {
    Info,
    Warning,
    Error,
    Fatal
};

/**
 * @brief 编译报错/警告定位条目
 */
struct BuildDiagnostic {
    QString filePath;
    int line = 0;
    int column = 0;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    QString message;
};

/**
 * @brief 异步构建调度引擎
 */
class BuildManager : public QObject
{
    Q_OBJECT

public:
    explicit BuildManager(QObject *parent = nullptr);
    ~BuildManager() override;

    // 设置工具链与芯片配置
    void setToolchainPaths(const ToolchainPaths &paths) { m_paths = paths; }
    ToolchainPaths toolchainPaths() const { return m_paths; }

    void setChipProfile(const STM32ChipProfile &profile) { m_profile = profile; }
    STM32ChipProfile chipProfile() const { return m_profile; }

    bool isBusy() const;

    // 一键构建 (自动生成最新 Makefile 并启动多核 make)
    bool startBuild(const QString &projectDir,
                    BuildConfiguration config = BuildConfiguration::Debug,
                    int parallelJobs = 0);

    // 一键清理 (make clean 或跨平台安全删除 build 目录)
    bool startClean(const QString &projectDir);

    // 一键全量重新构建 (clean + build)
    bool startRebuild(const QString &projectDir,
                      BuildConfiguration config = BuildConfiguration::Debug,
                      int parallelJobs = 0);

    // 终止当前构建
    void stopBuild();

    // 获取生成的目标文件路径
    QString getElfPath(const QString &projectDir) const;
    QString getBinPath(const QString &projectDir) const;
    QString getHexPath(const QString &projectDir) const;

signals:
    void buildStarted(const QString &action);
    void buildOutput(const QString &text, bool isError);
    void diagnosticFound(const BuildDiagnostic &diag);
    void memoryUsageUpdated(quint32 flashUsed, quint32 flashTotal,
                            quint32 ramUsed, quint32 ramTotal);
    void buildFinished(bool success, int exitCode, int durationMs);

private slots:
    void onProcessReadyReadStd();
    void onProcessReadyReadErr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void parseOutputLine(const QString &line, bool isError);
    void consumeOutput(const QString &text, bool isError);
    void parseSizeOutput(const QString &line);

    ToolchainPaths m_paths;
    STM32ChipProfile m_profile;
    QProcess *m_process = nullptr;
    QString m_currentProjectDir;
    bool m_isRebuilding = false;
    BuildConfiguration m_pendingConfig = BuildConfiguration::Debug;
    int m_pendingJobs = 0;
    qint64 m_startTime = 0;
    QString m_pendingStdout;
    QString m_pendingStderr;
    int m_errorCount = 0;
    int m_warningCount = 0;
};

#endif // BUILDMANAGER_H
