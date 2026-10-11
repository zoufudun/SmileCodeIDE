#include "canaiassistant.h"
#include "../Engine/udsengine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

CanAiAssistant::CanAiAssistant(KnowledgeEngine *knowledge, QWidget *parent)
    : QWidget(parent), m_knowledge(knowledge) {
  if (!m_knowledge) {
    m_knowledge = new KnowledgeEngine(this);
  }
  setupUi();

  // 欢迎语
  appendAssistantMessage(
      "### 👋 你好！我是 **AI 墩墩** —— 你的车载总线智能副驾！\n\n"
      "我可以帮助你完成：\n"
      "- 🔍 **DTC 故障码深度诊断** 与排障措施推荐\n"
      "- ⚡ **UDS 协议负响应 (NRC) 原因排查** 与解锁引导\n"
      "- 📊 **总线物理层/数据链路层异常分析**（错误帧、负载率超标）\n"
      "- 🛠️ **固件刷写失败原因推导**\n\n"
      "你可以点击下方快捷诊断按钮，或者在下方输入框中向我提问！"
  );
}

void CanAiAssistant::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(2, 2, 2, 2);
  mainLayout->setSpacing(4);

  // 1. 标准视图头部栏 (与 CanViewPanel 风格一致)
  m_headerWidget = new QWidget(this);
  m_headerWidget->setObjectName("viewHeader");
  m_headerWidget->setFixedHeight(32);

  QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
  headerLayout->setContentsMargins(10, 0, 10, 0);

  m_titleLabel = new QLabel("🤖 AI 智能诊断助手 (CANPilot)", m_headerWidget);
  m_titleLabel->setObjectName("viewTitle");
  headerLayout->addWidget(m_titleLabel);
  headerLayout->addStretch();

  m_closeBtn = new QPushButton("✕", m_headerWidget);
  m_closeBtn->setObjectName("viewCloseBtn");
  m_closeBtn->setToolTip("关闭 AI 助手视图");
  m_closeBtn->setFixedSize(22, 22);
  connect(m_closeBtn, &QPushButton::clicked, this, [this]() { emit closeRequested(this); });
  headerLayout->addWidget(m_closeBtn);

  mainLayout->addWidget(m_headerWidget);

  // 2. 快捷问题栏
  QHBoxLayout *quickLayout = new QHBoxLayout();
  quickLayout->setContentsMargins(4, 2, 4, 2);
  QPushButton *btnQ1 = new QPushButton("🚗 常见车载 DTC 故障分析", this);
  QPushButton *btnQ2 = new QPushButton("🔒 UDS 安全访问 0x33 解决办法", this);
  QPushButton *btnQ3 = new QPushButton("📉 总线持续报错误帧排查", this);

  quickLayout->addWidget(btnQ1);
  quickLayout->addWidget(btnQ2);
  quickLayout->addWidget(btnQ3);
  quickLayout->addStretch();
  mainLayout->addLayout(quickLayout);

  connect(btnQ1, &QPushButton::clicked, [this]() { onQuickActionClicked(1); });
  connect(btnQ2, &QPushButton::clicked, [this]() { onQuickActionClicked(2); });
  connect(btnQ3, &QPushButton::clicked, [this]() { onQuickActionClicked(3); });

  // 聊天展示区
  m_chatBrowser = new QTextBrowser(this);
  m_chatBrowser->setOpenExternalLinks(true);
  m_chatBrowser->setStyleSheet("background-color: #18181b; color: #f4f4f5; font-size: 13px; padding: 8px; border-radius: 6px;");
  mainLayout->addWidget(m_chatBrowser);

  // 输入区
  QHBoxLayout *inputLayout = new QHBoxLayout();
  m_inputEdit = new QLineEdit(this);
  m_inputEdit->setPlaceholderText("向 AI 墩墩提问关于 CAN / UDS / 故障码的问题...");
  m_btnSend = new QPushButton("发送 💬", this);
  m_btnSend->setStyleSheet("background-color: #0284c7; color: white; font-weight: bold; padding: 6px 14px;");

  inputLayout->addWidget(m_inputEdit);
  inputLayout->addWidget(m_btnSend);
  mainLayout->addLayout(inputLayout);

  connect(m_btnSend, &QPushButton::clicked, this, &CanAiAssistant::onSendQuestion);
  connect(m_inputEdit, &QLineEdit::returnPressed, this, &CanAiAssistant::onSendQuestion);
}

void CanAiAssistant::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss);
}

void CanAiAssistant::appendAssistantMessage(const QString &markdown) {
  m_chatBrowser->append(QString("<div style='background-color:#27272a; padding:10px; border-radius:8px; margin:6px 0; border-left: 4px solid #38bdf8;'><b>🤖 AI 墩墩:</b><br>%1</div>").arg(markdown.toHtmlEscaped().replace("\n", "<br>")));
}

void CanAiAssistant::appendUserMessage(const QString &text) {
  m_chatBrowser->append(QString("<div style='background-color:#1e3a8a; padding:8px; border-radius:8px; margin:6px 0; text-align:right;'><b>👤 我:</b> %1</div>").arg(text.toHtmlEscaped()));
}

void CanAiAssistant::analyzeDtcList(const QStringList &dtcCodes) {
  appendAssistantMessage(m_knowledge->generateTroubleshootingReport(dtcCodes));
}

void CanAiAssistant::analyzeNegativeResponse(quint8 sid, quint8 nrc) {
  QString rep = QString("### ⚠️ UDS 负响应诊断分析\n"
                        "请求服务: **0x%1** | 负响应码 NRC: **0x%2**\n\n").arg(sid, 2, 16, QLatin1Char('0')).arg(nrc, 2, 16, QLatin1Char('0'));
  rep += "- **含义**：" + UdsEngine::nrcToString(nrc) + "\n";
  if (nrc == 0x33) {
    rep += "- **排障建议**：请求受安全访问保护。请确保在此之前已通过 0x27 服务完成 Seed-Key 计算并成功解锁。\n";
  } else if (nrc == 0x22) {
    rep += "- **排障建议**：执行条件不满足。例如处于发动机运转中、车速大于0、或点火开关处于 OFF 档。\n";
  } else if (nrc == 0x7E || nrc == 0x7F) {
    rep += "- **排障建议**：当前会话不支持。请先发送 0x10 03 切换至扩展会话或 0x10 02 编程会话。\n";
  }
  appendAssistantMessage(rep);
}

void CanAiAssistant::onSendQuestion() {
  QString text = m_inputEdit->text().trimmed();
  if (text.isEmpty()) return;

  appendUserMessage(text);
  m_inputEdit->clear();

  // 智能关键词匹配与知识库检索
  QString upper = text.toUpper();
  if (upper.contains("P0100") || upper.contains("U0100") || upper.contains("P0115") || upper.contains("U0121") || upper.contains("U0140")) {
    QString code = "P0100";
    if (upper.contains("U0100")) code = "U0100";
    else if (upper.contains("P0115")) code = "P0115";
    else if (upper.contains("U0121")) code = "U0121";
    else if (upper.contains("U0140")) code = "U0140";
    analyzeDtcList(QStringList() << code);
    return;
  }

  if (upper.contains("0X33") || upper.contains("安全") || upper.contains("0X27")) {
    analyzeNegativeResponse(0x2E, 0x33);
    return;
  }

  if (upper.contains("0X78") || upper.contains("挂起")) {
    appendAssistantMessage("0x78 (ResponsePending) 表示 ECU 已经正确接收了请求，但正在后台执行耗时操作（如擦除Flash），请等待其最终响应，切勿重发请求。");
    return;
  }

  appendAssistantMessage(QString("我已经分析了你的问题：**「%1」**。\n\n"
                                 "建议：\n"
                                 "1. 在当前 CAN 工作台的 **报文跟踪** 视窗中观察总线是否有 ACK 确认或错误帧；\n"
                                 "2. 可以在 **诊断台** 中尝试发送 0x3E TesterPresent 或 0x22 读取 DID；\n"
                                 "3. 若需排查特定 DTC，可输入类似「P0100」或「U0100」让我进行深度分析。").arg(text));
}

void CanAiAssistant::onQuickActionClicked(int type) {
  if (type == 1) {
    analyzeDtcList(QStringList() << "P0100" << "U0100");
  } else if (type == 2) {
    analyzeNegativeResponse(0x34, 0x33);
  } else if (type == 3) {
    appendAssistantMessage("### 📉 总线持续报错误帧 (Error Frame) 排查指引\n\n"
                           "1. **终端电阻检查**：测量 CAN_H 与 CAN_L 间电阻是否严格等于 60Ω（两端各120Ω）；\n"
                           "2. **波特率匹配**：确保发送方与总线采样点、波特率完全一致（注意仲裁域与数据域）；\n"
                           "3. **地线浮地**：检查两端地回路共模电压偏差是否超过 ±12V；\n"
                           "4. **总线支路长度**：CAN FD 高速率下请尽量避免星型拓扑长分支。");
  }
}
