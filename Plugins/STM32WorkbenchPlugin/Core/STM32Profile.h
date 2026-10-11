#ifndef STM32PROFILE_H
#define STM32PROFILE_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>

/**
 * @brief STM32 芯片内存定义
 */
struct STM32MemoryRegion {
    QString name;
    QString origin;
    quint32 sizeBytes = 0;
    QString sizeStr;
};

/**
 * @brief STM32 目标芯片规格档案
 */
struct STM32ChipProfile {
    QString chipName;         // 如 "STM32F103C8" 或 "STM32F407ZG"
    QString family;           // 如 "STM32F1", "STM32F4", "STM32H7"
    QString core;             // 如 "cortex-m3", "cortex-m4", "cortex-m7", "cortex-m0plus"
    bool hasFpu = false;
    QString fpuFlags;         // 如 "-mfpu=fpv4-sp-d16 -mfloat-abi=hard"
    
    QString defaultMacro;     // 编译器宏 如 "STM32F103xB", "STM32F407xx"
    QStringList extraMacros;  // 额外宏 如 "USE_HAL_DRIVER"
    
    quint32 flashSizeBytes = 64 * 1024;
    quint32 ramSizeBytes   = 20 * 1024;
    // Linker 的默认 RAM 区域大小。为 0 时使用 ramSizeBytes。
    // H7 等器件的片上 RAM 并非从 ramOrigin 起连续分布，不能直接使用总容量。
    quint32 linkerRamSizeBytes = 0;
    QString flashOrigin    = "0x08000000";
    QString ramOrigin      = "0x20000000";
    
    QString openocdTargetCfg; // OpenOCD 脚本 如 "target/stm32f1x.cfg"
    QString jlinkDeviceName;  // J-Link 设备标识 如 "STM32F103C8"
    QString defaultLdFile;    // 默认链接脚本文件名
    QString linkerResourcePath; // 打包的 CubeMX 原版脚本；空值表示尚无审计过的脚本
    QString linkerSha256; // 内置脚本原始字节的 SHA-256
};

/**
 * @brief STM32 芯片配置数据库与生成工厂
 */
class STM32ProfileManager
{
public:
    static STM32ProfileManager& instance();

    // 获取所有受支持的芯片列表
    QStringList supportedChips() const;

    // 根据芯片名称精确查询档案
    STM32ChipProfile getProfile(const QString &chipName) const;

    // 自动为给定芯片生成高标准 GCC 链接脚本 (.ld)
    static QString generateLinkerScript(const STM32ChipProfile &profile);

    // 自动根据系列推荐 OpenOCD target 配置
    static QString getOpenocdTarget(const QString &familyOrChip);

private:
    STM32ProfileManager();
    void initDatabase();

    QMap<QString, STM32ChipProfile> m_database;
};

#endif // STM32PROFILE_H
