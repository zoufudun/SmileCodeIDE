#include "stm32workbenchplugin.h"
#include <QSettings>

STM32WorkbenchPlugin::STM32WorkbenchPlugin(QObject *parent)
    : QObject(parent)
{
}

bool STM32WorkbenchPlugin::initialize(IPluginContext *context)
{
    m_context = context;
    if (m_context) {
        m_context->log("INFO", QStringLiteral("STM32 编译构建与 GDB 调试工作台插件已成功注册初始化"));
    }
    return true;
}

void STM32WorkbenchPlugin::shutdown()
{
    if (m_currentWidget) {
        m_currentWidget->deleteLater();
    }
}

QWidget* STM32WorkbenchPlugin::createWidget(QWidget *parent)
{
    static QSettings s_settings("SmileCode", "STM32Workbench");
    m_currentWidget = new STM32WorkbenchWidget(parent, &s_settings);
    if (m_context) m_currentWidget->setSourceNavigationHost(m_context->mainWindow());

    if (m_context) {
        m_context->showToast(QStringLiteral("STM32 工作台已启动"), "success", 2000);
    }

    return m_currentWidget;
}

void STM32WorkbenchPlugin::applyTheme(const QString &themeId)
{
    Q_UNUSED(themeId);
}
