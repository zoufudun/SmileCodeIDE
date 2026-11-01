#ifndef SIGNALGENERATORPLUGIN_H
#define SIGNALGENERATORPLUGIN_H

#include <QObject>
#include <QPointer>
#include "iappplugin.h"
#include "signalgeneratorwidget.h"

class SignalGeneratorPlugin : public QObject, public IAppPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IAppPlugin_IID FILE "plugin.json")
    Q_INTERFACES(IAppPlugin)

public:
    explicit SignalGeneratorPlugin(QObject *parent = nullptr);
    ~SignalGeneratorPlugin() override = default;

    // 核心元数据
    QString id() const override { return QStringLiteral("signal_generator"); }
    QString name() const override { return QStringLiteral("高频信号发生与模拟器"); }
    QString subtitle() const override { return QStringLiteral("正弦波/方波/三角波/锯齿波/白噪声实时高频仿真与数据源生成"); }
    QString version() const override { return QStringLiteral("v1.0.0"); }
    QString author() const override { return QStringLiteral("SmileCode Team"); }
    QString category() const override { return QStringLiteral("测量与分析"); }
    QString description() const override {
        return QStringLiteral("基于 QPainter 实时动态绘制多波形高频信号发生器，支持频率、幅值、相位、偏置与噪声实时调节，支持单通道/多通道混叠与数据点导出。");
    }
    QStringList tags() const override {
        return QStringList() << QStringLiteral("信号发生") << QStringLiteral("波形仿真") << QStringLiteral("DAC模拟") << QStringLiteral("实时渲染");
    }
    QString iconUnicode() const override { return QStringLiteral("0xe86e"); }
    QString colorHex() const override { return QStringLiteral("#00b894"); }

    // 生命周期管理
    bool initialize(IPluginContext *context) override;
    void shutdown() override;

    // UI 工厂与主题联动
    QWidget* createWidget(QWidget *parent = nullptr) override;
    void applyTheme(const QString &themeId) override;

private:
    IPluginContext *m_context = nullptr;
    QPointer<SignalGeneratorWidget> m_currentWidget;
};

#endif // SIGNALGENERATORPLUGIN_H

