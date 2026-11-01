#include "templatewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>

TemplateWidget::TemplateWidget(IPluginContext *context, QWidget *parent)
    : QWidget(parent), m_context(context) {
    setWindowTitle(QStringLiteral("自定义扩展插件模板 (Plugin Template)"));
    resize(640, 480);
    setupUi();
}

void TemplateWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 1. 顶部标题与说明
    m_titleLabel = new QLabel(QStringLiteral("✨ SmileCode IDE 插件系统演示面板"), this);
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #00cec9;");
    mainLayout->addWidget(m_titleLabel);

    QLabel *descLabel = new QLabel(
        QStringLiteral("本面板演示了如何开发一个独立 Qt 动态库插件，并调用宿主提供的 IPluginContext 核心服务（如全局 Toast 提示、日志记录与主题自适应）。"),
        this);
    descLabel->setStyleSheet("color: #95a5a6; font-size: 12px; line-height: 1.4;");
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // 2. 宿主服务交互区
    QGroupBox *serviceBox = new QGroupBox(QStringLiteral("🛠 宿主上下文服务交互 (IPluginContext)"), this);
    QVBoxLayout *serviceLayout = new QVBoxLayout(serviceBox);
    serviceLayout->setSpacing(10);

    // Toast 触发行
    QHBoxLayout *toastRow = new QHBoxLayout();
    m_toastInput = new QLineEdit(QStringLiteral("这是来自自定义插件的全局悬浮通知！"), serviceBox);
    m_toastBtn = new QPushButton(QStringLiteral("🔔 弹出全局 Toast"), serviceBox);
    m_toastBtn->setCursor(Qt::PointingHandCursor);
    m_toastBtn->setStyleSheet("background-color: #00b894; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    connect(m_toastBtn, &QPushButton::clicked, this, &TemplateWidget::onSendToastClicked);
    toastRow->addWidget(m_toastInput, 1);
    toastRow->addWidget(m_toastBtn);
    serviceLayout->addLayout(toastRow);

    // 日志写入行
    QHBoxLayout *logRow = new QHBoxLayout();
    m_logInput = new QLineEdit(QStringLiteral("插件核心业务模块正常运行中..."), serviceBox);
    m_logBtn = new QPushButton(QStringLiteral("📝 记录系统日志"), serviceBox);
    m_logBtn->setCursor(Qt::PointingHandCursor);
    m_logBtn->setStyleSheet("background-color: #0984e3; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    connect(m_logBtn, &QPushButton::clicked, this, &TemplateWidget::onWriteLogClicked);
    logRow->addWidget(m_logInput, 1);
    logRow->addWidget(m_logBtn);
    serviceLayout->addLayout(logRow);

    mainLayout->addWidget(serviceBox);

    // 3. 插件内部控制台/输出区
    QGroupBox *consoleBox = new QGroupBox(QStringLiteral("🖥 插件内部运行日志"), this);
    QVBoxLayout *consoleLayout = new QVBoxLayout(consoleBox);

    m_console = new QTextEdit(consoleBox);
    m_console->setReadOnly(true);
    m_console->setStyleSheet("background: rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.08); border-radius: 4px; font-family: Consolas, monospace; color: #ecf0f1;");
    consoleLayout->addWidget(m_console);

    QHBoxLayout *consoleBottom = new QHBoxLayout();
    m_clearBtn = new QPushButton(QStringLiteral("清空控制台"), consoleBox);
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    connect(m_clearBtn, &QPushButton::clicked, this, &TemplateWidget::onClearLogClicked);
    consoleBottom->addStretch();
    consoleBottom->addWidget(m_clearBtn);
    consoleLayout->addLayout(consoleBottom);

    mainLayout->addWidget(consoleBox, 1);

    // 初始输出
    m_console->append(QString("[%1] 插件界面组件加载成功。").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

void TemplateWidget::onSendToastClicked() {
    QString msg = m_toastInput->text().trimmed();
    if (msg.isEmpty()) return;

    if (m_context) {
        // 调用宿主上下文服务
        m_context->showToast(msg, "success");
    }

    m_console->append(QString("[%1] [Toast 发送] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), msg));
}

void TemplateWidget::onWriteLogClicked() {
    QString logText = m_logInput->text().trimmed();
    if (logText.isEmpty()) return;

    if (m_context) {
        // 输出到宿主全局日志
        m_context->log("INFO", QString("[TemplatePlugin] %1").arg(logText));
    }

    m_console->append(QString("[%1] [Log 记录] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), logText));
}

void TemplateWidget::onClearLogClicked() {
    m_console->clear();
}

void TemplateWidget::applyTheme(const QString &themeId) {
    bool isLight = (themeId.compare("light", Qt::CaseInsensitive) == 0);
    if (isLight) {
        m_console->setStyleSheet("background: #f8f9fa; border: 1px solid #dfe6e9; border-radius: 4px; font-family: Consolas, monospace; color: #2d3436;");
    } else {
        m_console->setStyleSheet("background: rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.08); border-radius: 4px; font-family: Consolas, monospace; color: #ecf0f1;");
    }
}

