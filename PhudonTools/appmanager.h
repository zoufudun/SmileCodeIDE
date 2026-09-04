#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QVector>
#include <QWidget>
#include <QPointer>
#include <QPluginLoader>
#include <QFileSystemWatcher>
#include <QTimer>
#include <functional>
#include "include/iappplugin.h"
#include "include/iplugincontext.h"

// 应用分类枚举
enum class AppCategory {
    All = 0,            // 全部应用
    Favorites,          // 常用 / 收藏
    EmbeddedDev,        // 嵌入式开发
    BusProtocol,        // 总线与通信
    Measurement,        // 测量与分析
    Flashing,           // 固件与烧录
    Utility,            // 实用工具
    PluginExtensions    // 插件与扩展
};

// 插件类型
enum class AppPluginType {
    BuiltIn = 0,        // 内置 Qt 窗口组件
    QtDynamicPlugin,    // Qt 原生动态库插件 (.dll / .so)
    ExternalExecutable, // 外部独立可执行文件
    CustomScript        // 外部脚本/命令
};

// 应用元数据结构
struct AppInfo {
    QString id;                 // 唯一 ID (如 "smilecode_ide")
    QString name;               // 显示名称
    QString subtitle;           // 副标题 / 核心亮点
    QString version;            // 版本号 (如 "v2.1.0")
    QString author;             // 作者 / 团队
    AppCategory category = AppCategory::Utility;       // 分类
    QString categoryName;       // 分类显示文本
    QString iconPath;           // 图标路径 (资源或本地文件)
    QString iconUnicode;        // 图标字体编码 (可选，如 "0xe7d2")
    QString colorHex;           // 强调主题色 (如 "#6c5ce7")
    QString description;        // 详细功能简介
    QStringList tags;           // 标签列表
    bool isBuiltIn = true;      // 是否为内置核心应用
    bool isEnabled = true;      // 是否启用
    bool isFavorite = false;    // 是否已收藏
    int launchCount = 0;        // 启动次数
    QDateTime lastLaunchTime;   // 最近启动时间
    AppPluginType pluginType = AppPluginType::BuiltIn; // 插件类型
    QString execPath;           // 外部可执行文件路径
    QStringList execArgs;       // 外部可执行文件参数
    QString dllPath;            // 动态库绝对路径 (针对 QtDynamicPlugin)
    QString interfaceIid;       // 接口 IID
    IAppPlugin *pluginInstance = nullptr; // 动态插件接口句柄 (针对 QtDynamicPlugin)

    // 辅助转换方法
    static QString categoryToString(AppCategory cat);
    static AppCategory stringToCategory(const QString &str);
};

// 动态库插件运行时记录
struct DynamicPluginRecord {
    QPluginLoader *loader = nullptr;
    IAppPlugin *instance = nullptr;
    QString filePath;
    AppInfo info;
};

// 应用管理器单例 (同时实现 IPluginContext 宿主服务接口)
class AppManager : public QObject, public IPluginContext {
    Q_OBJECT

public:
    using WidgetFactory = std::function<QWidget*(QWidget* parent)>;

    static AppManager* instance();

    // 初始化管理器并加载持久化配置与动态/外部插件
    void initialize();

    // 注册内置应用
    void registerBuiltInApp(const AppInfo &info, WidgetFactory factory);

    // =========================================================================
    // Qt Dynamic Plugin (动态库插件系统核心 API)
    // =========================================================================

    // 批量扫描并加载动态库插件目录 (支持 .dll, .so, .dylib)
    int loadDynamicPluginsFromDir(const QString &dirPath = QString());

    // 加载单个动态库插件文件
    bool loadDynamicPlugin(const QString &filePath, QString *errorMsg = nullptr);

    // 卸载单个动态库插件
    bool unloadDynamicPlugin(const QString &appId);

    // 重新扫描所有插件目录 (增量发现新加入的 DLL 并清理已删除的)
    int rescanPlugins();

    // 获取已加载的所有 Qt 动态插件记录
    QMap<QString, DynamicPluginRecord> getDynamicPlugins() const { return m_dynamicPlugins; }

    // =========================================================================
    // 传统配置插件与外部工具 API
    // =========================================================================

    // 注册/安装外部插件 (从 JSON 配置文件)
    bool installPluginFromJson(const QString &jsonFilePath, QString *errorMsg = nullptr);

    // 手动注册外部插件
    bool registerExternalPlugin(const AppInfo &info, QString *errorMsg = nullptr);

    // 卸载插件 (仅限非内置应用)
    bool uninstallPlugin(const QString &appId);

    // 设置插件启用/禁用
    void setAppEnabled(const QString &appId, bool enabled);

    // 设置收藏状态
    void setAppFavorite(const QString &appId, bool favorite);

    // 获取应用信息
    AppInfo getAppInfo(const QString &appId) const;
    bool hasApp(const QString &appId) const;

    // 获取应用列表
    QVector<AppInfo> getAllApps() const;
    QVector<AppInfo> getAppsByCategory(AppCategory category) const;
    QVector<AppInfo> searchApps(const QString &keyword, AppCategory category = AppCategory::All) const;

    // 启动/激活应用
    QWidget* launchApp(const QString &appId, QWidget *parent = nullptr);

    // 关闭应用
    void closeApp(const QString &appId);
    void closeAllApps();

    // 检查应用是否正在运行
    bool isAppRunning(const QString &appId) const;

    // 获取运行中的应用控件
    QWidget* getRunningWidget(const QString &appId) const;

    // 运行中的应用数量
    int getRunningAppCount() const;

    // 获取插件安装目录
    QString getPluginsDirectory() const;

    // 全局主题管理与同步
    QString getCurrentTheme() const;
    void setCurrentTheme(const QString &themeId);
    void applyThemeToWidget(QWidget *widget);

    // 设置宿主主窗口句柄
    void setMainWindow(QWidget *window) { m_mainWindow = window; }

    // 保存和载入配置
    void saveSettings();
    void loadSettings();

    // =========================================================================
    // IPluginContext 宿主服务实现
    // =========================================================================
    QString currentTheme() const override { return m_currentTheme; }
    void showToast(const QString &message, const QString &type = "info", int durationMs = 2500) override;
    void log(const QString &level, const QString &message) override;
    QString appVersion() const override { return "v2.1.0"; }
    QWidget* mainWindow() const override { return m_mainWindow.data(); }

signals:
    void appLaunched(const QString &appId, QWidget *widget);
    void appClosed(const QString &appId);
    void appStatusChanged(const QString &appId, bool isRunning);
    void pluginListChanged();
    void appFavoriteChanged(const QString &appId, bool isFavorite);
    void globalThemeChanged(const QString &themeId);
    void pluginDiscovered(const QString &appId, const QString &name);
    void pluginLoadError(const QString &filePath, const QString &error);

private slots:
    void onPluginDirectoryChanged(const QString &path);
    void onPluginFileChanged(const QString &path);
    void onDebounceScanTriggered();

private:
    explicit AppManager(QObject *parent = nullptr);
    ~AppManager() override;

    // 内部存储
    QMap<QString, AppInfo> m_apps;
    QMap<QString, WidgetFactory> m_factories;
    QMap<QString, QPointer<QWidget>> m_runningWidgets;
    QMap<QString, DynamicPluginRecord> m_dynamicPlugins;
    QPointer<QWidget> m_mainWindow;
    QString m_currentTheme = "dark";

    // 目录热监听与防抖自动发现
    QFileSystemWatcher *m_dirWatcher = nullptr;
    QTimer *m_scanDebounceTimer = nullptr;

    void setupDirectoryWatcher();
    void loadExternalPluginsFromDir();
};

#endif // APPMANAGER_H
