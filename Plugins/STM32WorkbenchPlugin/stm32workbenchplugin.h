#ifndef STM32WORKBENCHPLUGIN_H
#define STM32WORKBENCHPLUGIN_H

#include <QObject>
#include <QPointer>
#include "iappplugin.h"
#include "UI/STM32WorkbenchWidget.h"

/**
 * @brief STM32 嵌入式编译构建与硬件调试工作台 Qt 动态插件
 */
class STM32WorkbenchPlugin : public QObject, public IAppPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IAppPlugin_IID FILE "plugin.json")
    Q_INTERFACES(IAppPlugin)

public:
    explicit STM32WorkbenchPlugin(QObject *parent = nullptr);
    ~STM32WorkbenchPlugin() override = default;

    // 插件元数据
    QString id() const override { return QStringLiteral("stm32_workbench"); }
    QString name() const override { return QStringLiteral("STM32 编译构建与 GDB 调试工作台"); }
    QString subtitle() const override { return QStringLiteral("GCC/Make 一键编译构建、固件烧录与硬件 GDB 在线断点单步调试"); }
    QString version() const override { return QStringLiteral("v1.0.0"); }
    QString author() const override { return QStringLiteral("SmileCode Team"); }
    QString category() const override { return QStringLiteral("嵌入式开发"); }
    QString description() const override {
        return QStringLiteral("专为 STM32 嵌入式工程打造的一体化工作台：支持 Cortex-M0/M3/M4/M7 全系列、自动化 Makefile 依赖追踪构建、多线程并行编译、Flash/RAM 资源监控、OpenOCD/J-Link/ST-Link 一键烧录与原生 GDB/MI 异步断点/调用栈/寄存器/局部变量硬件调试。");
    }
    QStringList tags() const override {
        return QStringList() << QStringLiteral("STM32") << QStringLiteral("GCC") << QStringLiteral("Make") << QStringLiteral("GDB调试") << QStringLiteral("固件烧录") << QStringLiteral("OpenOCD");
    }
    QString iconUnicode() const override { return QStringLiteral("0xe7d2"); }
    QString colorHex() const override { return QStringLiteral("#3867d6"); }

    // 生命周期与界面工厂
    bool initialize(IPluginContext *context) override;
    void shutdown() override;
    QWidget* createWidget(QWidget *parent = nullptr) override;
    void applyTheme(const QString &themeId) override;

private:
    IPluginContext *m_context = nullptr;
    QPointer<STM32WorkbenchWidget> m_currentWidget;
};

#endif // STM32WORKBENCHPLUGIN_H
