#include "templateplugin.h"

TemplatePlugin::TemplatePlugin(QObject *parent) : QObject(parent) {
}

bool TemplatePlugin::initialize(IPluginContext *context) {
    m_context = context;
    if (m_context) {
        m_context->log("INFO", "TemplatePlugin 已成功加载并完成初始化。");
    }
    return true;
}

void TemplatePlugin::shutdown() {
    if (m_context) {
        m_context->log("INFO", "TemplatePlugin 正在安全释放资源并卸载。");
    }
    m_context = nullptr;
}

QWidget* TemplatePlugin::createWidget(QWidget *parent) {
    TemplateWidget *widget = new TemplateWidget(m_context, parent);
    m_currentWidget = widget;

    // 如果宿主已有当前主题，同步应用
    if (m_context) {
        widget->applyTheme(m_context->currentTheme());
    }

    return widget;
}

void TemplatePlugin::applyTheme(const QString &themeId) {
    if (m_currentWidget) {
        m_currentWidget->applyTheme(themeId);
    }
}

