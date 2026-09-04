#ifndef TEMPLATEPLUGIN_H
#define TEMPLATEPLUGIN_H

#include <QObject>
#include <QPointer>
#include "iappplugin.h"
#include "templatewidget.h"

/**
 * @brief Qt 动态插件入口类模板 (Template Plugin Entry)
 * 
 * 必须继承 QObject 与 IAppPlugin 抽象接口，
 * 并通过 Q_PLUGIN_METADATA 宏声明接口 IID 与嵌入的元数据 JSON。
 */
class TemplatePlugin : public QObject, public IAppPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IAppPlugin_IID FILE "plugin.json")
    Q_INTERFACES(IAppPlugin)

public:
    explicit TemplatePlugin(QObject *parent = nullptr);
    ~TemplatePlugin() override = default;

    // =========================================================================
    // 1. 插件元数据配置 (Plugin Metadata)
    // =========================================================================
    QString id() const override { return QStringLiteral("template_plugin"); }
    QString name() const override { return QStringLiteral("自定义扩展插件模板"); }
    QString subtitle() const override { return QStringLiteral("开箱即用的 Qt 动态插件开发范例与接口演示"); }
    QString version() const override { return QStringLiteral("v1.0.0"); }
    QString author() const override { return QStringLiteral("SmileCode Team"); }
    QString category() const override { return QStringLiteral("实用工具"); }
    QString description() const override {
        return QStringLiteral("本插件为标准 Qt Plugin 动态库模板工程，展示了元数据注册、界面实例化、宿主上下文服务交互与主题联动机制。");
    }
    QStringList tags() const override {
        return QStringList() << QStringLiteral("开发模板") << QStringLiteral("范例") << QStringLiteral("热插拔");
    }
    QString iconUnicode() const override { return QStringLiteral("0xe7d2"); }
    QString colorHex() const override { return QStringLiteral("#6c5ce7"); }

    // =========================================================================
    // 2. 插件生命周期管理 (Plugin Lifecycle)
    // =========================================================================
    bool initialize(IPluginContext *context) override;
    void shutdown() override;

    // =========================================================================
    // 3. 界面工厂与主题联动 (UI Factory & Theme Sync)
    // =========================================================================
    QWidget* createWidget(QWidget *parent = nullptr) override;
    void applyTheme(const QString &themeId) override;

private:
    IPluginContext *m_context = nullptr;
    QPointer<TemplateWidget> m_currentWidget;
};

#endif // TEMPLATEPLUGIN_H

