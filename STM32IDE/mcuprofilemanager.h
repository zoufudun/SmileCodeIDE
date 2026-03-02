/*
 * @Description: MCU Profile Manager - manages STM32 MCU configuration profiles
 * @Version: 1.0
 * @Autor: Atlas
 * @Date: 2026-02-27
 */
#ifndef MCUPROFILEMANAGER_H
#define MCUPROFILEMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>

// MCU Memory definition
struct McuMemory {
    QString origin;
    QString size;
};

// MCU Profile structure
struct McuProfile {
    QString name;           // e.g., "STM32F103xB"
    QString series;         // e.g., "F1", "F4", "H7"
    QString chip;           // e.g., "STM32F103", "STM32F407"
    QString core;           // e.g., "Cortex-M3", "Cortex-M4"
    bool fpu;              // has FPU
    
    QString cpuFlags;       // e.g., "-mcpu=cortex-m3 -mthumb"
    QString cFlags;        // e.g., "-Wall -g -O0"
    QString cppFlags;      // e.g., "-fno-exceptions -fno-rtti"
    QStringList defines;   // e.g., ["STM32F103xB", "USE_HAL_DRIVER"]
    QStringList includes;  // include paths
    
    QMap<QString, McuMemory> memory;  // flash, ram, sram1, etc.
    QString linkerTemplate;  // e.g., "STM32F1xx_FLASH.ld"
    
    bool isValid() const;
};

// Profile index entry
struct ProfileIndexEntry {
    QString file;
    QString name;
    QString series;
    QString core;
};

class McuProfileManager : public QObject
{
    Q_OBJECT

public:
    explicit McuProfileManager(QObject *parent = nullptr);
    ~McuProfileManager();
    
    // Load all profiles from resources/mcu-profiles/
    bool loadProfiles();
    
    // Get profile by chip name (e.g., "STM32F407", "F407", "F4")
    McuProfile getProfile(const QString &chipName) const;
    
    // Get all profiles for a series (e.g., "F1", "F4")
    QVector<McuProfile> getProfilesBySeries(const QString &series) const;
    
    // Get all available series
    QStringList getAvailableSeries() const;
    
    // Get all profile names
    QStringList getProfileNames() const;
    
    // Get core for a chip
    QString getCore(const QString &chipName) const;
    
    // Check if profile exists
    bool hasProfile(const QString &chipName) const;

private:
    // Parse JSON profile
    McuProfile parseProfile(const QJsonObject &obj) const;
    
    // Get profile directory path
    QString getProfileDir() const;

signals:
    void profilesLoaded(int count);
    void loadError(const QString &error);

private:
    QVector<McuProfile> m_profiles;
    QMap<QString, int> m_profileIndex;  // name -> index
    QMap<QString, QVector<int>> m_seriesIndex;  // series -> indices
};

#endif // MCUPROFILEMANAGER_H
