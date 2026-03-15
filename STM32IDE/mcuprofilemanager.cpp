/*
 * @Description: MCU Profile Manager implementation
 * @Version: 1.0
 * @Autor: Atlas
 * @Date: 2026-02-27
 */
#include "mcuprofilemanager.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QDebug>

McuProfileManager::McuProfileManager(QObject *parent)
    : QObject(parent)
{
}

McuProfileManager::~McuProfileManager()
{
}

QString McuProfileManager::getProfileDir() const
{
    // Get the application directory
    QDir dir(QCoreApplication::applicationDirPath());
    
    // Try resources/mcu-profiles in app directory
    QString appDir = dir.absolutePath();
    QString profilesPath = appDir + "/resources/mcu-profiles";
    
    // Also check relative to source directory for development
    QFileInfo sourceFile(__FILE__);
    QString sourceDir = sourceFile.absolutePath();
    QString devProfilesPath = sourceDir + "/resources/mcu-profiles";
    
    if (QDir(profilesPath).exists()) {
        return profilesPath;
    } else if (QDir(devProfilesPath).exists()) {
        return devProfilesPath;
    }
    
    return profilesPath;
}

bool McuProfileManager::loadProfiles()
{
    QString profileDir = getProfileDir();
    QDir dir(profileDir);
    
    if (!dir.exists()) {
        qWarning() << "MCU profiles directory does not exist:" << profileDir;
        emit loadError("Profiles directory not found: " + profileDir);
        return false;
    }
    
    // Load index.json first
    QString indexFile = profileDir + "/index.json";
    QFile file(indexFile);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open index.json:" << file.errorString();
        emit loadError("Cannot open index.json");
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        emit loadError("JSON parse error: " + parseError.errorString());
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray profiles = root["profiles"].toArray();
    
    m_profiles.clear();
    m_profileIndex.clear();
    m_seriesIndex.clear();
    
    // Load each profile file
    for (int i = 0; i < profiles.size(); ++i) {
        QJsonObject entry = profiles[i].toObject();
        QString profileFile = profileDir + "/" + entry["file"].toString();
        
        QFile pfFile(profileFile);
        if (!pfFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot open profile file:" << profileFile;
            continue;
        }
        
        QByteArray pfData = pfFile.readAll();
        pfFile.close();
        
        QJsonDocument pfDoc = QJsonDocument::fromJson(pfData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Profile JSON parse error:" << parseError.errorString();
            continue;
        }
        
        McuProfile profile = parseProfile(pfDoc.object());
        if (profile.isValid()) {
            int index = m_profiles.size();
            m_profiles.append(profile);
            
            // Index by name
            m_profileIndex[profile.name.toLower()] = index;
            m_profileIndex[profile.chip.toLower()] = index;
            m_profileIndex[profile.series.toLower()] = index;
            
            // Index by series
            m_seriesIndex[profile.series.toLower()].append(index);
        }
    }
    
    qDebug() << "Loaded" << m_profiles.size() << "MCU profiles";
    emit profilesLoaded(m_profiles.size());
    
    return m_profiles.size() > 0;
}

McuProfile McuProfileManager::parseProfile(const QJsonObject &obj) const
{
    McuProfile profile;
    
    profile.name = obj["name"].toString();
    profile.series = obj["series"].toString();
    profile.chip = obj["chip"].toString();
    profile.core = obj["core"].toString();
    profile.fpu = obj["fpu"].toBool(false);
    
    profile.cpuFlags = obj["cpu_flags"].toString();
    profile.cFlags = obj["c_flags"].toString();
    profile.cppFlags = obj["cpp_flags"].toString();
    
    // Parse defines
    QJsonArray defines = obj["defines"].toArray();
    for (int i = 0; i < defines.size(); ++i) {
        profile.defines.append(defines[i].toString());
    }
    
    // Parse includes
    QJsonArray includes = obj["includes"].toArray();
    for (int i = 0; i < includes.size(); ++i) {
        profile.includes.append(includes[i].toString());
    }
    
    // Parse memory
    QJsonObject memory = obj["memory"].toObject();
    for (auto it = memory.begin(); it != memory.end(); ++it) {
        QJsonObject mem = it.value().toObject();
        McuMemory m;
        m.origin = mem["origin"].toString();
        m.size = mem["size"].toString();
        profile.memory[it.key()] = m;
    }
    
    profile.linkerTemplate = obj["linker_template"].toString();
    
    return profile;
}

McuProfile McuProfileManager::getProfile(const QString &chipName) const
{
    QString key = chipName.toLower();
    
    // Direct lookup
    if (m_profileIndex.contains(key)) {
        return m_profiles[m_profileIndex[key]];
    }
    
    // Partial match
    for (int i = 0; i < m_profiles.size(); ++i) {
        const McuProfile &p = m_profiles[i];
        if (p.name.toLower().contains(key) || 
            p.chip.toLower().contains(key) ||
            p.series.toLower() == key) {
            return p;
        }
    }
    
    // Return default F1 profile if not found
    if (m_profileIndex.contains("stm32f103xb")) {
        return m_profiles[m_profileIndex["stm32f103xb"]];
    }
    
    return McuProfile();
}

QVector<McuProfile> McuProfileManager::getProfilesBySeries(const QString &series) const
{
    QVector<McuProfile> result;
    QString key = series.toLower();
    
    if (m_seriesIndex.contains(key)) {
        const QVector<int> &indices = m_seriesIndex[key];
        for (int idx : indices) {
            result.append(m_profiles[idx]);
        }
    }
    
    return result;
}

QStringList McuProfileManager::getAvailableSeries() const
{
    return m_seriesIndex.keys();
}

QStringList McuProfileManager::getProfileNames() const
{
    QStringList names;
    for (const McuProfile &p : m_profiles) {
        names.append(p.name);
    }
    return names;
}

QString McuProfileManager::getCore(const QString &chipName) const
{
    McuProfile p = getProfile(chipName);
    return p.core;
}

bool McuProfileManager::hasProfile(const QString &chipName) const
{
    QString key = chipName.toLower();
    
    if (m_profileIndex.contains(key)) {
        return true;
    }
    
    for (const McuProfile &p : m_profiles) {
        if (p.name.toLower().contains(key) || 
            p.chip.toLower().contains(key)) {
            return true;
        }
    }
    
    return false;
}

bool McuProfile::isValid() const
{
    return !name.isEmpty() && !series.isEmpty() && !core.isEmpty();
}
