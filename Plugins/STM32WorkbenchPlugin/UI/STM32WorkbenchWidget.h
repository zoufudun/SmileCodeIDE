#ifndef STM32WORKBENCHWIDGET_H
#define STM32WORKBENCHWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QProgressBar>
#include <QLabel>
#include <QSplitter>
#include <QSettings>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QPointer>
#include <QTabWidget>
#include <QTreeWidget>

#include "../Core/ToolchainDetector.h"
#include "../Core/STM32Profile.h"
#include "../Core/BuildManager.h"
#include "../Core/FlashProgrammer.h"
#include "../Core/GdbServerManager.h"
#include "../Core/GdbMiEngine.h"
#include "GdbDebugPanel.h"

/**
 * @brief 独立 STM32 一键编译构建、固件烧录与硬件 GDB 调试工作台主窗体
 */
class STM32WorkbenchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit STM32WorkbenchWidget(QWidget *parent = nullptr, QSettings *settings = nullptr);
    ~STM32WorkbenchWidget() override;

    // 设置当前工程根目录
    void setProjectPath(const QString &path);
    QString projectPath() const { return m_projectPath; }

    // 暴露核心引擎与调试接口
    BuildManager* buildManager() const { return m_buildManager; }
    FlashProgrammer* flashProgrammer() const { return m_flashProgrammer; }
    GdbMiEngine* gdbEngine() const { return m_gdbEngine; }
    void setSourceNavigationHost(QWidget *host) { m_sourceNavigationHost = host; }

    // 断点快速操作（供外部代码编辑器调用联动）
    void toggleBreakpoint(const QString &file, int line);

signals:
    void jumpToSourceLine(const QString &file, int line);
    void projectPathChanged(const QString &newPath);

public slots:
    void onBuildClicked();
    void onCleanClicked();
    void onRebuildClicked();
    void onFlashClicked();
    void onToggleDebugSession();
    void onOpenToolchainSettings();
    void onOpenFlashSettings();
    void onOpenChipSelectDialog();

private slots:
    void onBrowseProjectDir();
    void onChipChanged(const QString &chipName);
    void onBuildOutput(const QString &text, bool isError);
    void onFlashOutput(const QString &text, bool isError);
    void onFlashProgress(int percent, const QString &text);
    void onMemoryUsageUpdated(quint32 flashUsed, quint32 flashTotal, quint32 ramUsed, quint32 ramTotal);
    void onServerReady(int port);
    void onGdbStateChanged(GdbTargetState state);

private:
    void setupUI();
    void applyTheme();
    void loadSettings();
    void saveSettings();
    void appendLogText(const QString &text, bool isError);
    void appendLogLine(const QString &line, bool isError);
    void flushLogText();
    void beginFlashLog(const QString &firmwarePath, const FlashOptions &options);
    void finishFlashLog(bool success, const QString &message);
    void renderFlashLog();
    void leaveFlashLog();
    void beginBuildLog(const QString &action);
    void renderBuildLog(bool reset = false);
    void refreshBuildProblems();
    void navigateBuildProblem(int direction);
    void activateBuildProblem(QTreeWidgetItem *item);
    void navigateToSource(const QString &file, int line, int column = 0);

    QSettings *m_settings;
    QString m_projectPath;

    // 核心引擎层
    ToolchainDetector *m_detector;
    BuildManager *m_buildManager;
    FlashProgrammer *m_flashProgrammer;
    GdbServerManager *m_serverManager;
    GdbMiEngine *m_gdbEngine;
    ToolchainPaths m_paths;
    STM32ChipProfile m_currentProfile;

    // 工具栏控件
    QLineEdit *m_projectPathEdit;
    QPushButton *m_browseProjectBtn;
    QComboBox *m_chipCombo;
    QPushButton *m_selectChipBtn;
    QComboBox *m_configCombo;

    QPushButton *m_buildBtn;
    QPushButton *m_cleanBtn;
    QPushButton *m_rebuildBtn;
    QPushButton *m_flashBtn;
    QPushButton *m_debugBtn;
    QPushButton *m_toolchainConfigBtn;
    QPushButton *m_flashConfigBtn;

    // 终端与监控窗格
    QTextBrowser *m_buildTerminal;
    QTabWidget *m_outputTabs;
    QTreeWidget *m_buildProblems;
    QComboBox *m_problemFilter;
    QLabel *m_problemCounts;
    QPushButton *m_previousProblem;
    QPushButton *m_nextProblem;
    QPointer<QWidget> m_sourceNavigationHost;
    bool m_usingBuildLogModel = false;
    bool m_buildViewActive = false;
    qsizetype m_renderedBuildEntries = 0;
    QString m_buildLogHeading;
    bool m_buildHasSize = false;
    QCheckBox *m_flashDetailedLog;
    FlashLogModel m_flashLog;
    QVector<FlashLogEntry> m_flashLogHeader;
    QElapsedTimer m_flashTimer;
    FlashOptions m_flashOptions;
    bool m_showingFlashLog = false;
    bool m_flashActive = false;
    QString m_pendingLogOut;
    QString m_pendingLogErr;
    QProgressBar *m_flashMemBar;
    QLabel *m_flashMemLabel;
    QProgressBar *m_ramMemBar;
    QLabel *m_ramMemLabel;
    QProgressBar *m_taskProgressBar;
    QLabel *m_taskStatusLabel;
    bool m_retryRebuildAfterMigration = false;

    // GDB 调试面板
    GdbDebugPanel *m_debugPanel;
    QSplitter *m_mainSplitter;
};

#endif // STM32WORKBENCHWIDGET_H
