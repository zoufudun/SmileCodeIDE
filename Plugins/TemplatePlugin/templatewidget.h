#ifndef TEMPLATEWIDGET_H
#define TEMPLATEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include "iplugincontext.h"

/**
 * @brief 插件自定义主界面控件模板
 */
class TemplateWidget : public QWidget {
    Q_OBJECT

public:
    explicit TemplateWidget(IPluginContext *context, QWidget *parent = nullptr);
    ~TemplateWidget() override = default;

    /**
     * @brief 响应宿主全局主题切换
     * @param themeId 当前主题 ID ("dark", "light", "purple" 等)
     */
    void applyTheme(const QString &themeId);

private slots:
    void onSendToastClicked();
    void onWriteLogClicked();
    void onClearLogClicked();

private:
    void setupUi();

    IPluginContext *m_context;

    QLabel *m_titleLabel;
    QLineEdit *m_toastInput;
    QPushButton *m_toastBtn;
    QLineEdit *m_logInput;
    QPushButton *m_logBtn;
    QPushButton *m_clearBtn;
    QTextEdit *m_console;
};

#endif // TEMPLATEWIDGET_H

