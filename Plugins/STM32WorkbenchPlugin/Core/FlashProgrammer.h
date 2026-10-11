#ifndef FLASHPROGRAMMER_H
#define FLASHPROGRAMMER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include "ToolchainDetector.h"
#include "STM32Profile.h"
#include "ProbeDetector.h"

/**
 * @brief 烧录器驱动类型
 */
enum class FlashDriverType {
    OpenOCD_STLink,     // OpenOCD + ST-Link 适配器
    OpenOCD_CMSISDAP,   // OpenOCD + CMSIS-DAP / DAPLink 适配器
    OpenOCD_JLink,      // OpenOCD + J-Link 适配器
    JLink_Native,       // SEGGER J-Link 官方原生工具 (JLink.exe)
    STM32CubeProg,      // ST 官方 STM32_Programmer_CLI
    StFlash             // 开源 st-flash (stlink-tools)
};

/**
 * @brief 烧录选项配置
 */
struct FlashOptions {
    FlashDriverType driver = FlashDriverType::OpenOCD_STLink;
    QString targetAddress = "0x08000000";
    bool verify = true;
    bool resetAndRun = true;
    bool eraseAll = false;
    int swdSpeedKhz = 4000;
    QString probeSerial;
    QString probeInstanceId;
    QString customOpenocdInterfaceCfg;
    QString customOpenocdTargetCfg;
};

/**
 * @brief 多设备固件烧录与下载调度引擎
 */
class FlashProgrammer : public QObject
{
    Q_OBJECT

public:
    explicit FlashProgrammer(QObject *parent = nullptr);
    ~FlashProgrammer() override;

    void setToolchainPaths(const ToolchainPaths &paths) { m_paths = paths; }
    void setChipProfile(const STM32ChipProfile &profile) { m_profile = profile; }

    bool isBusy() const;

    // 执行一键烧录
    bool flashFirmware(const QString &firmwarePath, const FlashOptions &options);
    static QStringList openOcdArguments(const QString &firmwarePath, const FlashOptions &options,
                                        const QString &targetCfg);
    static QStringList cubeProgrammerArguments(const QString &firmwarePath, const FlashOptions &options);
    static QStringList jLinkArguments(const FlashOptions &options, const QString &device,
                                      const QString &scriptPath);
    static QString jLinkCommandScript(const QString &firmwarePath, const FlashOptions &options,
                                      const QString &device);
    static bool driverSupportsProbe(FlashDriverType driver, ProbeType probe);
    static bool validTargetAddress(const QString &address);
    static bool driverSupportsMassErase(FlashDriverType driver);

    // 终止当前烧录
    void cancelFlash();

signals:
    void flashStarted(const QString &driverName);
    void flashOutput(const QString &text, bool isError);
    void flashProgress(int percentage, const QString &statusText);
    void flashFinished(bool success, const QString &message);

private slots:
    void onProcessReadyReadStd();
    void onProcessReadyReadErr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool flashViaOpenOCD(const QString &firmwarePath, const FlashOptions &options);
    bool flashViaJLink(const QString &firmwarePath, const FlashOptions &options);
    bool flashViaSTM32Cube(const QString &firmwarePath, const FlashOptions &options);
    bool flashViaStFlash(const QString &firmwarePath, const FlashOptions &options);

    ToolchainPaths m_paths;
    STM32ChipProfile m_profile;
    QProcess *m_process = nullptr;
    QString m_tempScriptPath;
    QString m_tempFirmwarePath;
    qint64 m_startTime = 0;
};

#endif // FLASHPROGRAMMER_H
