#ifndef IPLUGINCONTEXT_H
#define IPLUGINCONTEXT_H

#include <QString>
#include <QObject>
#include <QWidget>

/**
 * @brief 宿主上下文接口 (Plugin Host Context Interface)
 * 
 * 为所有动态加载的 Qt 插件提供宿主环境的核心服务，
 * 包括全局主题查询、通知提示 (Toast)、全局日志输出以及宿主主窗口句柄，
 * 实现插件与宿主之间的解耦交互。
 */
class IPluginContext {
public:
    virtual ~IPluginContext() = default;

    /**
     * @brief 获取当前宿主系统的主题 ID (如 "dark", "light", "purple")
     */
    virtual QString currentTheme() const = 0;

    /**
     * @brief 在宿主界面弹出悬浮 Toast 通知
     * @param message 通知文本
     * @param type 通知类型 ("info", "success", "warning", "error")
     * @param durationMs 停留时长 (毫秒)
     */
    virtual void showToast(const QString &message, const QString &type = "info", int durationMs = 2500) = 0;

    /**
     * @brief 输出全局日志到宿主控制台/日志记录器
     * @param level 日志级别 ("DEBUG", "INFO", "WARN", "ERROR")
     * @param message 日志内容
     */
    virtual void log(const QString &level, const QString &message) = 0;

    /**
     * @brief 获取宿主主应用程序版本号
     */
    virtual QString appVersion() const = 0;

    /**
     * @brief 获取宿主主窗口或工作台指针
     */
    virtual QWidget* mainWindow() const = 0;
};

#endif // IPLUGINCONTEXT_H

