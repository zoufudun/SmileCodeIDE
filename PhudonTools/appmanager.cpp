#include "appmanager.h"
#include "mainwindow.h"
#include "serialportplot.h"
#include "cantool.h"
#include "networktool.h"
#include "iaptool.h"
#include "oscilloscopewindow.h"
#include "idetheme.h"
#include "toastwidget.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QSettings>
#include <QLibrary>
#include <QDebug>

QString AppInfo::categoryToString(AppCategory cat) {
    switch (cat) {
    case AppCategory::All: return QStringLiteral("全部应用");
    case AppCategory::Favorites: return QStringLiteral("常用推荐");
    case AppCategory::EmbeddedDev: return QStringLiteral("嵌入式开发");
    case AppCategory::BusProtocol: return QStringLiteral("总线与通信");
    case AppCategory::Measurement: return QStringLiteral("测量与分析");
    case AppCategory::Flashing: return QStringLiteral("固件与烧录");
    case AppCategory::Utility: return QStringLiteral("实用工具");
    case AppCategory::PluginExtensions: return QStringLiteral("插件与扩展");
    default: return QStringLiteral("其他工具");
    }
}

AppCategory AppInfo::stringToCategory(const QString &str) {
    if (str == QStringLiteral("嵌入式开发") || str.compare("EmbeddedDev", Qt::CaseInsensitive) == 0) return AppCategory::EmbeddedDev;
    if (str == QStringLiteral("总线与通信") || str == QStringLiteral("通信调试") || str.compare("BusProtocol", Qt::CaseInsensitive) == 0) return AppCategory::BusProtocol;
    if (str == QStringLiteral("测量与分析") || str.compare("Measurement", Qt::CaseInsensitive) == 0) return AppCategory::Measurement;
    if (str == QStringLiteral("固件与烧录") || str.compare("Flashing", Qt::CaseInsensitive) == 0) return AppCategory::Flashing;
    if (str == QStringLiteral("实用工具") || str.compare("Utility", Qt::CaseInsensitive) == 0) return AppCategory::Utility;
    if (str == QStringLiteral("插件与扩展") || str.compare("PluginExtensions", Qt::CaseInsensitive) == 0) return AppCategory::PluginExtensions;
    return AppCategory::Utility;
}

AppManager* AppManager::instance() {
    static AppManager s_instance;
    return &s_instance;
}

AppManager::AppManager(QObject *parent) : QObject(parent) {
    m_scanDebounceTimer = new QTimer(this);
    m_scanDebounceTimer->setSingleShot(true);
    m_scanDebounceTimer->setInterval(400);
    connect(m_scanDebounceTimer, &QTimer::timeout, this, &AppManager::onDebounceScanTriggered);

    m_dirWatcher = new QFileSystemWatcher(this);
    connect(m_dirWatcher, &QFileSystemWatcher::directoryChanged, this, &AppManager::onPluginDirectoryChanged);
    connect(m_dirWatcher, &QFileSystemWatcher::fileChanged, this, &AppManager::onPluginFileChanged);
}

AppManager::~AppManager() {
    saveSettings();

    // 优雅关闭所有插件与释放动态库
    closeAllApps();
    for (auto it = m_dynamicPlugins.begin(); it != m_dynamicPlugins.end(); ++it) {
        if (it->instance) {
            it->instance->shutdown();
        }
        if (it->loader) {
            it->loader->unload();
            delete it->loader;
        }
    }
    m_dynamicPlugins.clear();
}

void AppManager::initialize() {
    loadSettings();
    loadExternalPluginsFromDir();
    loadDynamicPluginsFromDir();
    setupDirectoryWatcher();
}

void AppManager::setupDirectoryWatcher() {
    QString pluginsDir = getPluginsDirectory();
    QDir dir(pluginsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (!m_dirWatcher->directories().contains(pluginsDir)) {
        m_dirWatcher->addPath(pluginsDir);
    }
}

void AppManager::onPluginDirectoryChanged(const QString &path) {
    // 部分系统在目录内容改变后可能会丢失监控，重新加回
    if (!m_dirWatcher->directories().contains(path) && QDir(path).exists()) {
        m_dirWatcher->addPath(path);
    }
    m_scanDebounceTimer->start();
}

void AppManager::onPluginFileChanged(const QString &path) {
    Q_UNUSED(path);
    m_scanDebounceTimer->start();
}

void AppManager::onDebounceScanTriggered() {
    qDebug() << "[AppManager] 插件目录发生变更，正在执行热扫描与自动加载...";
    rescanPlugins();
}

void AppManager::registerBuiltInApp(const AppInfo &info, WidgetFactory factory) {
    AppInfo finalInfo = info;
    finalInfo.isBuiltIn = true;
    finalInfo.pluginType = AppPluginType::BuiltIn;
    finalInfo.categoryName = AppInfo::categoryToString(finalInfo.category);
    
    // 恢复用户自定义状态 (如收藏、启动次数)
    if (m_apps.contains(finalInfo.id)) {
        finalInfo.isFavorite = m_apps[finalInfo.id].isFavorite;
        finalInfo.isEnabled = m_apps[finalInfo.id].isEnabled;
        finalInfo.launchCount = m_apps[finalInfo.id].launchCount;
        finalInfo.lastLaunchTime = m_apps[finalInfo.id].lastLaunchTime;
    }

    m_apps[finalInfo.id] = finalInfo;
    m_factories[finalInfo.id] = factory;
}

// =============================================================================
// Qt 动态库插件系统核心实现 (QPluginLoader + IAppPlugin)
// =============================================================================

int AppManager::loadDynamicPluginsFromDir(const QString &dirPath) {
    QString targetDir = dirPath.isEmpty() ? getPluginsDirectory() : dirPath;
    QDir dir(targetDir);
    if (!dir.exists()) {
        dir.mkpath(".");
        return 0;
    }

    // 搜索动态链接库
    QStringList nameFilters;
#if defined(Q_OS_WIN)
    nameFilters << "*.dll";
#elif defined(Q_OS_MAC)
    nameFilters << "*.dylib" << "*.so";
#else
    nameFilters << "*.so";
#endif

    QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files | QDir::Readable);
    int loadedCount = 0;

    for (const QFileInfo &fileInfo : fileList) {
        QString fullPath = fileInfo.absoluteFilePath();

        // 排除非 Qt 插件库 (如 zlgcan.dll, ControlCAN.dll)
        if (fileInfo.fileName().startsWith("Qt5", Qt::CaseInsensitive) ||
            fileInfo.fileName().startsWith("zlgcan", Qt::CaseInsensitive) ||
            fileInfo.fileName().startsWith("ControlCAN", Qt::CaseInsensitive)) {
            continue;
        }

        // 检查该路径是否已经成功装载
        bool alreadyLoaded = false;
        for (const auto &record : m_dynamicPlugins) {
            if (record.filePath == fullPath) {
                alreadyLoaded = true;
                break;
            }
        }
        if (alreadyLoaded) continue;

        QString errorMsg;
        if (loadDynamicPlugin(fullPath, &errorMsg)) {
            loadedCount++;
        }
    }

    return loadedCount;
}

bool AppManager::loadDynamicPlugin(const QString &filePath, QString *errorMsg) {
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        if (errorMsg) *errorMsg = QStringLiteral("动态库文件不存在或不可读: ") + filePath;
        return false;
    }

    // 使用 QPluginLoader 进行安全的插件探查与实例装载
    QPluginLoader *loader = new QPluginLoader(filePath, this);
    QObject *instanceObj = loader->instance();

    if (!instanceObj) {
        QString err = loader->errorString();
        delete loader;
        if (errorMsg) *errorMsg = QStringLiteral("动态库未能实例化 (可能非Qt插件或缺少依赖): ") + err;
        emit pluginLoadError(filePath, err);
        return false;
    }

    // 强类型接口转换验证
    IAppPlugin *plugin = qobject_cast<IAppPlugin*>(instanceObj);
    if (!plugin) {
        QString err = QStringLiteral("该动态库未实现 IAppPlugin 接口或接口 IID 不匹配 (%1)").arg(IAppPlugin_IID);
        loader->unload();
        delete loader;
        if (errorMsg) *errorMsg = err;
        emit pluginLoadError(filePath, err);
        return false;
    }

    // 初始化插件 (注入宿主服务上下文 IPluginContext)
    if (!plugin->initialize(this)) {
        QString err = QStringLiteral("插件初始化 (initialize) 失败");
        loader->unload();
        delete loader;
        if (errorMsg) *errorMsg = err;
        emit pluginLoadError(filePath, err);
        return false;
    }

    // 提取插件元数据
    AppInfo info;
    info.id = plugin->id().trimmed();
    if (info.id.isEmpty()) {
        info.id = fileInfo.baseName();
    }
    info.name = plugin->name();
    if (info.name.isEmpty()) info.name = info.id;
    info.subtitle = plugin->subtitle();
    info.version = plugin->version().isEmpty() ? "v1.0.0" : plugin->version();
    info.author = plugin->author().isEmpty() ? QStringLiteral("第三方开发者") : plugin->author();
    info.category = AppInfo::stringToCategory(plugin->category());
    info.categoryName = AppInfo::categoryToString(info.category);
    info.description = plugin->description();
    info.tags = plugin->tags();
    if (info.tags.isEmpty()) {
        info.tags.append(QStringLiteral("Qt 动态插件"));
    }
    info.iconUnicode = plugin->iconUnicode();
    info.iconPath = plugin->iconPath();
    info.colorHex = plugin->colorHex().isEmpty() ? "#00b894" : plugin->colorHex();
    info.isBuiltIn = false;
    info.isEnabled = true;
    info.pluginType = AppPluginType::QtDynamicPlugin;
    info.dllPath = fileInfo.absoluteFilePath();
    info.interfaceIid = IAppPlugin_IID;
    info.pluginInstance = plugin;

    // 恢复用户配置
    if (m_apps.contains(info.id)) {
        info.isFavorite = m_apps[info.id].isFavorite;
        info.isEnabled = m_apps[info.id].isEnabled;
        info.launchCount = m_apps[info.id].launchCount;
        info.lastLaunchTime = m_apps[info.id].lastLaunchTime;
    }

    // 保存插件记录
    DynamicPluginRecord record;
    record.loader = loader;
    record.instance = plugin;
    record.filePath = fileInfo.absoluteFilePath();
    record.info = info;

    m_dynamicPlugins[info.id] = record;
    m_apps[info.id] = info;

    qDebug() << QString("[AppManager] 成功加载 Qt 动态库插件 -> [%1] %2 (%3)").arg(info.id, info.name, info.version);

    emit pluginDiscovered(info.id, info.name);
    emit pluginListChanged();
    return true;
}

bool AppManager::unloadDynamicPlugin(const QString &appId) {
    if (!m_dynamicPlugins.contains(appId)) return false;

    // 1. 如果正在运行，先关闭窗口
    closeApp(appId);

    DynamicPluginRecord record = m_dynamicPlugins.take(appId);
    m_apps.remove(appId);

    if (record.instance) {
        record.instance->shutdown();
    }

    if (record.loader) {
        record.loader->unload();
        delete record.loader;
    }

    saveSettings();
    emit pluginListChanged();
    qDebug() << QString("[AppManager] 已成功卸载动态库插件: %1").arg(appId);
    return true;
}

int AppManager::rescanPlugins() {
    // 1. 检查已被删除的 DLL 插件并注销
    QStringList toRemove;
    for (auto it = m_dynamicPlugins.begin(); it != m_dynamicPlugins.end(); ++it) {
        if (!QFile::exists(it->filePath)) {
            toRemove.append(it.key());
        }
    }
    for (const QString &id : toRemove) {
        qDebug() << "[AppManager] 检测到插件 DLL 已被移除:" << id;
        unloadDynamicPlugin(id);
    }

    // 2. 发现并加载新放入 plugins/ 的 DLL
    int newCount = loadDynamicPluginsFromDir();
    // 3. 发现并加载新的 JSON 配置插件
    loadExternalPluginsFromDir();

    if (newCount > 0 || !toRemove.isEmpty()) {
        emit pluginListChanged();
    }
    return newCount;
}

// =============================================================================
// JSON 扩展插件与外部工具支持
// =============================================================================

bool AppManager::installPluginFromJson(const QString &jsonFilePath, QString *errorMsg) {
    QFile file(jsonFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMsg) *errorMsg = QStringLiteral("无法读取插件配置文件: ") + jsonFilePath;
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMsg) *errorMsg = QStringLiteral("JSON 格式错误: ") + parseError.errorString();
        return false;
    }

    QJsonObject obj = doc.object();
    AppInfo info;
    info.id = obj.value("id").toString().trimmed();
    if (info.id.isEmpty()) {
        if (errorMsg) *errorMsg = QStringLiteral("插件配置缺少有效的 'id' 字段");
        return false;
    }

    info.name = obj.value("name").toString(info.id);
    info.subtitle = obj.value("subtitle").toString();
    info.version = obj.value("version").toString("v1.0.0");
    info.author = obj.value("author").toString(QStringLiteral("第三方开发者"));
    info.category = AppInfo::stringToCategory(obj.value("category").toString());
    info.categoryName = AppInfo::categoryToString(info.category);
    info.description = obj.value("description").toString();
    info.iconPath = obj.value("iconPath").toString();
    info.colorHex = obj.value("colorHex").toString("#6c5ce7");
    info.isBuiltIn = false;
    info.isEnabled = obj.value("isEnabled").toBool(true);
    info.pluginType = (obj.value("type").toString() == "script") ? AppPluginType::CustomScript : AppPluginType::ExternalExecutable;
    info.execPath = obj.value("execPath").toString();
    
    QJsonArray tagsArr = obj.value("tags").toArray();
    for (const QJsonValue &v : tagsArr) {
        info.tags.append(v.toString());
    }
    if (info.tags.isEmpty()) {
        info.tags.append(QStringLiteral("扩展插件"));
    }

    QJsonArray argsArr = obj.value("execArgs").toArray();
    for (const QJsonValue &v : argsArr) {
        info.execArgs.append(v.toString());
    }

    // 复制配置文件到插件目录
    QString pluginsDir = getPluginsDirectory();
    QDir().mkpath(pluginsDir);
    QString targetPath = pluginsDir + "/" + info.id + ".json";
    if (QFileInfo(jsonFilePath).absoluteFilePath() != QFileInfo(targetPath).absoluteFilePath()) {
        QFile::copy(jsonFilePath, targetPath);
    }

    return registerExternalPlugin(info, errorMsg);
}

bool AppManager::registerExternalPlugin(const AppInfo &info, QString *errorMsg) {
    if (info.id.isEmpty()) {
        if (errorMsg) *errorMsg = QStringLiteral("插件 ID 不能为空");
        return false;
    }

    AppInfo finalInfo = info;
    finalInfo.isBuiltIn = false;
    if (finalInfo.category == AppCategory::All) {
        finalInfo.category = AppCategory::PluginExtensions;
    }
    finalInfo.categoryName = AppInfo::categoryToString(finalInfo.category);

    m_apps[finalInfo.id] = finalInfo;
    saveSettings();
    emit pluginListChanged();
    return true;
}

bool AppManager::uninstallPlugin(const QString &appId) {
    if (!m_apps.contains(appId)) return false;
    if (m_apps[appId].isBuiltIn) return false;

    // 如果是 Qt 动态库插件
    if (m_apps[appId].pluginType == AppPluginType::QtDynamicPlugin) {
        QString dllPath = m_apps[appId].dllPath;
        unloadDynamicPlugin(appId);
        if (QFile::exists(dllPath)) {
            QFile::remove(dllPath);
        }
        return true;
    }

    // 如果正在运行则关闭
    closeApp(appId);

    // 删除插件配置文件
    QString jsonFile = getPluginsDirectory() + "/" + appId + ".json";
    if (QFile::exists(jsonFile)) {
        QFile::remove(jsonFile);
    }

    m_apps.remove(appId);
    m_factories.remove(appId);
    saveSettings();
    emit pluginListChanged();
    return true;
}

void AppManager::setAppEnabled(const QString &appId, bool enabled) {
    if (m_apps.contains(appId)) {
        m_apps[appId].isEnabled = enabled;
        saveSettings();
        emit pluginListChanged();
    }
}

void AppManager::setAppFavorite(const QString &appId, bool favorite) {
    if (m_apps.contains(appId)) {
        m_apps[appId].isFavorite = favorite;
        saveSettings();
        emit appFavoriteChanged(appId, favorite);
    }
}

AppInfo AppManager::getAppInfo(const QString &appId) const {
    return m_apps.value(appId);
}

bool AppManager::hasApp(const QString &appId) const {
    return m_apps.contains(appId);
}

QVector<AppInfo> AppManager::getAllApps() const {
    QVector<AppInfo> result;
    for (const auto &info : m_apps) {
        result.append(info);
    }
    return result;
}

QVector<AppInfo> AppManager::getAppsByCategory(AppCategory category) const {
    QVector<AppInfo> result;
    for (const auto &info : m_apps) {
        if (!info.isEnabled) continue;

        if (category == AppCategory::All) {
            result.append(info);
        } else if (category == AppCategory::Favorites) {
            if (info.isFavorite) {
                result.append(info);
            }
        } else if (info.category == category) {
            result.append(info);
        }
    }
    return result;
}

QVector<AppInfo> AppManager::searchApps(const QString &keyword, AppCategory category) const {
    QVector<AppInfo> result;
    QString key = keyword.trimmed().toLower();

    for (const auto &info : m_apps) {
        if (!info.isEnabled) continue;

        // 分类过滤
        if (category != AppCategory::All) {
            if (category == AppCategory::Favorites && !info.isFavorite) continue;
            if (category != AppCategory::Favorites && info.category != category) continue;
        }

        // 关键词过滤
        if (key.isEmpty()) {
            result.append(info);
            continue;
        }

        bool match = info.name.toLower().contains(key) ||
                     info.subtitle.toLower().contains(key) ||
                     info.description.toLower().contains(key) ||
                     info.categoryName.toLower().contains(key);

        if (!match) {
            for (const QString &tag : info.tags) {
                if (tag.toLower().contains(key)) {
                    match = true;
                    break;
                }
            }
        }

        if (match) {
            result.append(info);
        }
    }
    return result;
}

QWidget* AppManager::launchApp(const QString &appId, QWidget *parent) {
    if (!m_apps.contains(appId)) {
        qWarning() << "[AppManager] 未找到应用:" << appId;
        return nullptr;
    }

    AppInfo &info = m_apps[appId];
    info.launchCount++;
    info.lastLaunchTime = QDateTime::currentDateTime();

    // 1. 如果已经有实例在运行，直接置顶并激活
    if (m_runningWidgets.contains(appId) && !m_runningWidgets[appId].isNull()) {
        QWidget *w = m_runningWidgets[appId].data();
        w->show();
        w->raise();
        w->activateWindow();
        saveSettings();
        return w;
    }

    // 2. 如果是 Qt 动态库插件 (Qt Plugin)
    if (info.pluginType == AppPluginType::QtDynamicPlugin && m_dynamicPlugins.contains(appId)) {
        IAppPlugin *plugin = m_dynamicPlugins[appId].instance;
        if (plugin) {
            QWidget *widget = plugin->createWidget(parent);
            if (widget) {
                widget->setAttribute(Qt::WA_DeleteOnClose, true);
                m_runningWidgets[appId] = widget;

                connect(widget, &QObject::destroyed, this, [this, appId]() {
                    m_runningWidgets.remove(appId);
                    emit appStatusChanged(appId, false);
                    emit appClosed(appId);
                });

                // 同步应用主题
                plugin->applyTheme(m_currentTheme);
                applyThemeToWidget(widget);

                widget->show();
                widget->raise();
                widget->activateWindow();

                emit appStatusChanged(appId, true);
                emit appLaunched(appId, widget);
                saveSettings();
                return widget;
            }
        }
    }

    // 3. 如果是外部独立可执行程序/脚本
    if (info.pluginType == AppPluginType::ExternalExecutable || info.pluginType == AppPluginType::CustomScript) {
        if (!info.execPath.isEmpty()) {
            QProcess::startDetached(info.execPath, info.execArgs);
            saveSettings();
            emit appLaunched(appId, nullptr);
            return nullptr;
        }
    }

    // 4. 内置应用通过工厂创建
    if (m_factories.contains(appId) && m_factories[appId]) {
        QWidget *widget = m_factories[appId](parent);
        if (widget) {
            widget->setAttribute(Qt::WA_DeleteOnClose, true);
            m_runningWidgets[appId] = widget;
            
            connect(widget, &QObject::destroyed, this, [this, appId]() {
                m_runningWidgets.remove(appId);
                emit appStatusChanged(appId, false);
                emit appClosed(appId);
            });

            applyThemeToWidget(widget);

            widget->show();
            widget->raise();
            widget->activateWindow();

            emit appStatusChanged(appId, true);
            emit appLaunched(appId, widget);
            saveSettings();
            return widget;
        }
    }

    saveSettings();
    return nullptr;
}

void AppManager::closeApp(const QString &appId) {
    if (m_runningWidgets.contains(appId) && !m_runningWidgets[appId].isNull()) {
        m_runningWidgets[appId]->close();
    }
}

void AppManager::closeAllApps() {
    auto widgets = m_runningWidgets;
    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        if (!it.value().isNull()) {
            it.value()->close();
        }
    }
    m_runningWidgets.clear();
}

bool AppManager::isAppRunning(const QString &appId) const {
    return m_runningWidgets.contains(appId) && !m_runningWidgets[appId].isNull();
}

QWidget* AppManager::getRunningWidget(const QString &appId) const {
    if (m_runningWidgets.contains(appId)) {
        return m_runningWidgets[appId].data();
    }
    return nullptr;
}

int AppManager::getRunningAppCount() const {
    int count = 0;
    for (auto it = m_runningWidgets.begin(); it != m_runningWidgets.end(); ++it) {
        if (!it.value().isNull()) {
            count++;
        }
    }
    return count;
}

QString AppManager::getPluginsDirectory() const {
    return QCoreApplication::applicationDirPath() + "/plugins";
}

QString AppManager::getCurrentTheme() const {
    return m_currentTheme;
}

void AppManager::setCurrentTheme(const QString &themeId) {
    if (themeId.isEmpty()) return;
    m_currentTheme = themeId;

    // 1. 全局样式表
    QString styleQss = IdeTheme::generateStyleSheet(themeId);
    if (!styleQss.isEmpty()) {
        qApp->setStyleSheet(styleQss);
    } else {
        QString sheetPath = QString(":/resources/styles/%1.qss").arg(themeId);
        QFile f(sheetPath);
        if (f.open(QFile::ReadOnly | QFile::Text)) {
            qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
        }
    }

    // 2. 广播给所有动态插件实例
    for (auto it = m_dynamicPlugins.begin(); it != m_dynamicPlugins.end(); ++it) {
        if (it->instance) {
            it->instance->applyTheme(themeId);
        }
    }

    // 3. 广播给当前所有正在运行的子应用窗口
    for (auto it = m_runningWidgets.begin(); it != m_runningWidgets.end(); ++it) {
        if (!it.value().isNull()) {
            applyThemeToWidget(it.value().data());
        }
    }

    saveSettings();
    emit globalThemeChanged(themeId);
}

void AppManager::applyThemeToWidget(QWidget *widget) {
    if (!widget) return;

    if (MainWindow *mainWin = qobject_cast<MainWindow*>(widget)) {
        mainWin->applyTheme(m_currentTheme);
    } else if (SerialPortContainer *serialContainer = qobject_cast<SerialPortContainer*>(widget)) {
        serialContainer->applyGlobalTheme(m_currentTheme);
    } else if (SerialPortPlot *serialPlot = qobject_cast<SerialPortPlot*>(widget)) {
        serialPlot->applyGlobalTheme(m_currentTheme);
    } else if (CANTool *canTool = qobject_cast<CANTool*>(widget)) {
        canTool->applyTheme(m_currentTheme);
    } else if (NetworkTool *netTool = qobject_cast<NetworkTool*>(widget)) {
        netTool->applyTheme(m_currentTheme);
    } else if (IAPTool *iapTool = qobject_cast<IAPTool*>(widget)) {
        iapTool->applyTheme(m_currentTheme);
    } else if (OscilloscopeWindow *scopeWin = qobject_cast<OscilloscopeWindow*>(widget)) {
        scopeWin->applyTheme(m_currentTheme);
    }
}

void AppManager::loadExternalPluginsFromDir() {
    QString pluginsDir = getPluginsDirectory();
    QDir dir(pluginsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
        return;
    }

    QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &file : jsonFiles) {
        // 排除插件内部生成的 json
        if (file.endsWith("plugin.json", Qt::CaseInsensitive)) continue;
        QString fullPath = dir.absoluteFilePath(file);
        installPluginFromJson(fullPath);
    }
}

void AppManager::saveSettings() {
    QSettings settings("PhudonTools", "AppManager");
    settings.setValue("CurrentTheme", m_currentTheme);
    settings.beginGroup("Apps");
    for (const auto &info : m_apps) {
        settings.beginGroup(info.id);
        settings.setValue("isFavorite", info.isFavorite);
        settings.setValue("isEnabled", info.isEnabled);
        settings.setValue("launchCount", info.launchCount);
        settings.setValue("lastLaunchTime", info.lastLaunchTime.toString(Qt::ISODate));
        settings.endGroup();
    }
    settings.endGroup();
}

void AppManager::loadSettings() {
    QSettings settings("PhudonTools", "AppManager");
    m_currentTheme = settings.value("CurrentTheme", "dark").toString();
    settings.beginGroup("Apps");
    QStringList childGroups = settings.childGroups();
    for (const QString &id : childGroups) {
        settings.beginGroup(id);
        if (m_apps.contains(id)) {
            m_apps[id].isFavorite = settings.value("isFavorite", false).toBool();
            m_apps[id].isEnabled = settings.value("isEnabled", true).toBool();
            m_apps[id].launchCount = settings.value("launchCount", 0).toInt();
            QString timeStr = settings.value("lastLaunchTime").toString();
            if (!timeStr.isEmpty()) {
                m_apps[id].lastLaunchTime = QDateTime::fromString(timeStr, Qt::ISODate);
            }
        }
        settings.endGroup();
    }
    settings.endGroup();
}

// =============================================================================
// IPluginContext 宿主服务实现
// =============================================================================

void AppManager::showToast(const QString &message, const QString &type, int durationMs) {
    Q_UNUSED(durationMs);
    bool isSuccess = (type.compare("error", Qt::CaseInsensitive) != 0 && type.compare("warning", Qt::CaseInsensitive) != 0);
    QWidget *parent = m_mainWindow.data();
    ToastWidget::showToast(message, isSuccess, parent);
}

void AppManager::log(const QString &level, const QString &message) {
    qDebug() << QString("[PluginLog][%1] %2").arg(level, message);
}
