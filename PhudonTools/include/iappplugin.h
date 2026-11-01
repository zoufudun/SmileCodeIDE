#ifndef IAPPPLUGIN_H
#define IAPPPLUGIN_H

#include <QtPlugin>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QVariantMap>
#include "iplugincontext.h"

/**
 * @brief Qt 动态插件核心接口 (IAppPlugin Interface)
 * 
 * 适用于所有通过 DLL/SO 动态加载的扩展插件，
 * 遵循 Qt 原生插件规范 (Q_PLUGIN_METADATA / Q_DECLARE_INTERFACE)。
 * 实现“零修改扩展”：将编译好的 DLL 放入 plugins/ 目录即可自动发现并加载。
 */
class IAppPlugin {
public:
    virtual ~IAppPlugin() = default;

    // =========================================================================
    // 插件核心元数据 (Plugin Metadata)
    // =========================================================================

    /**
     * @brief 插件唯一标识符 (如 "signal_generator", "modbus_master")
     */
    virtual QString id() const = 0;

    /**
     * @brief 插件显示名称 (如 "高频信号发生器")
     */
    virtual QString name() const = 0;

    /**
     * @brief 插件副标题 / 亮点功能简述
     */
    virtual QString subtitle() const = 0;

    /**
     * @brief 插件语义化版本号 (如 "v1.0.0")
     */
    virtual QString version() const = 0;

    /**
     * @brief 插件开发者或机构组织
     */
    virtual QString author() const = 0;

    /**
     * @brief 插件所属分类 ("嵌入式开发", "总线与通信", "测量与分析", "固件与烧录", "实用工具", "插件与扩展")
     */
    virtual QString category() const = 0;

    /**
     * @brief 插件详细功能介绍
     */
    virtual QString description() const = 0;

    /**
     * @brief 检索与过滤标签列表
     */
    virtual QStringList tags() const = 0;

    /**
     * @brief 图标字体十六进制编码 (可选，如 "0xe86e"、"0xe661")
     */
    virtual QString iconUnicode() const { return QString(); }

    /**
     * @brief 本地图标或资源图片路径 (可选，如 ":/icons/plugin.png")
     */
    virtual QString iconPath() const { return QString(); }

    /**
     * @brief 主题强调色 Hex 字符串 (可选，如 "#00b894")
     */
    virtual QString colorHex() const { return QStringLiteral("#00b894"); }

    // =========================================================================
    // 插件生命周期管理 (Plugin Lifecycle)
    // =========================================================================

    /**
     * @brief 插件初始化钩子 (在插件被宿主装载校验通过后调用)
     * @param context 宿主服务上下文句柄
     * @return 初始化是否成功
     */
    virtual bool initialize(IPluginContext *context) = 0;

    /**
     * @brief 插件析构与卸载钩子 (在插件被移除或程序退出前调用)
     */
    virtual void shutdown() = 0;

    // =========================================================================
    // 界面工厂与主题交互 (UI Factory & Theme Sync)
    // =========================================================================

    /**
     * @brief 创建插件主界面 Widget 实例
     * @param parent 父控件指针
     * @return 独立可用的 QWidget 指针
     */
    virtual QWidget* createWidget(QWidget *parent = nullptr) = 0;

    /**
     * @brief 当宿主全局切换主题时触发此通知
     * @param themeId 当前主题 ID ("dark", "light", "purple" 等)
     */
    virtual void applyTheme(const QString &themeId) { Q_UNUSED(themeId); }

    /**
     * @brief 扩展属性与配置参数键值对 (可选)
     */
    virtual QVariantMap customProperties() const { return QVariantMap(); }
};

#define IAppPlugin_IID "com.smilecode.plugin.IAppPlugin/1.0"
Q_DECLARE_INTERFACE(IAppPlugin, IAppPlugin_IID)

#endif // IAPPPLUGIN_H

