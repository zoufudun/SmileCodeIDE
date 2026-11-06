#ifndef TOOLCHAINDETECTOR_H
#define TOOLCHAINDETECTOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QSettings>

/**
 * @brief 嵌入式工具链配置数据结构
 */
struct ToolchainPaths {
    QString gccPath;          // arm-none-eabi-gcc 可执行文件完整路径
    QString gppPath;          // arm-none-eabi-g++ 可执行文件完整路径
    QString gdbPath;          // arm-none-eabi-gdb 可执行文件完整路径
    QString objcopyPath;      // arm-none-eabi-objcopy 可执行文件完整路径
    QString sizePath;         // arm-none-eabi-size 可执行文件完整路径
    QString makePath;         // make 或 mingw32-make 可执行文件完整路径
    
    // 烧录器与调试服务工具
    QString openocdPath;      // openocd 可执行文件完整路径
    QString openocdScriptsDir;// openocd scripts 脚本目录路径
    QString jlinkPath;        // JLink.exe / JLinkExe 可执行文件完整路径
    QString stm32ProgPath;    // STM32_Programmer_CLI 可执行文件路径
    
    bool isGccValid() const;
    bool isMakeValid() const;
    bool isGdbValid() const;
    bool isOpenocdValid() const;
    bool isJlinkValid() const;
};

/**
 * @brief 跨平台嵌入式交叉编译工具链智能探测与健康自检器
 */
class ToolchainDetector : public QObject
{
    Q_OBJECT

public:
    explicit ToolchainDetector(QObject *parent = nullptr);
    ~ToolchainDetector() override = default;

    // 自动扫描与智能探测工具链（探测 PATH、默认安装路径、D:\Toolchains 等）
    ToolchainPaths autoDetect(const QString &hintDir = QString());

    // 专项自动查找系统中安装的 SEGGER J-Link 或 Keil 内置 J-Link 可执行文件路径
    QString detectJLinkPath(const QString &hintDir = QString());

    // 从 QSettings 加载用户已保存的配置
    ToolchainPaths loadSettings(QSettings *settings);

    // 将工具链配置保存到 QSettings
    void saveSettings(QSettings *settings, const ToolchainPaths &paths);

    // 获取某个工具的版本信息（异步或同步执行 --version）
    static QString getToolVersion(const QString &executablePath);

    // 检查给定的目录是否包含有效的 ARM GCC 工具链
    static bool validateArmGccDir(const QString &dirPath, ToolchainPaths &outPaths);

    // 格式化输出环境诊断自检报告
    QString generateDiagnosticReport(const ToolchainPaths &paths);

    // 获取当前默认全局路径
    ToolchainPaths currentPaths() const { return m_paths; }
    void setCurrentPaths(const ToolchainPaths &paths) { m_paths = paths; }

signals:
    void detectionProgress(const QString &message);
    void detectionFinished(const ToolchainPaths &paths);

private:
    QString findExecutable(const QString &name, const QStringList &searchDirs);
    QString probeToolchainsDir();

private:
    ToolchainPaths m_paths;
};

#endif // TOOLCHAINDETECTOR_H
