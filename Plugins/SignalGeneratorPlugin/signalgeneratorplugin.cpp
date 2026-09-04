#include "signalgeneratorplugin.h"

SignalGeneratorPlugin::SignalGeneratorPlugin(QObject *parent) : QObject(parent) {
}

bool SignalGeneratorPlugin::initialize(IPluginContext *context) {
    m_context = context;
    if (m_context) {
        m_context->log("INFO", "SignalGeneratorPlugin 插件初始化成功，宿主上下文已就绪。");
    }
    return true;
}

void SignalGeneratorPlugin::shutdown() {
    if (m_context) {
        m_context->log("INFO", "SignalGeneratorPlugin 插件正在释放资源并关闭。");
    }
    m_context = nullptr;
}

QWidget* SignalGeneratorPlugin::createWidget(QWidget *parent) {
    SignalGeneratorWidget *widget = new SignalGeneratorWidget(parent);
    m_currentWidget = widget;
    if (m_context) {
        widget->applyTheme(m_context->currentTheme());
    }
    return widget;
}

void SignalGeneratorPlugin::applyTheme(const QString &themeId) {
    if (m_currentWidget) {
        m_currentWidget->applyTheme(themeId);
    }
}

