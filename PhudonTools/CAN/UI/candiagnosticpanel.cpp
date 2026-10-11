#include "candiagnosticpanel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDateTime>
#include <QSplitter>

CanDiagnosticPanel::CanDiagnosticPanel(CanInterface *can, QWidget *parent)
    : QWidget(parent), m_can(can) {
  // 构建底层协议栈
  ICanAdapter *adapter = (m_can && m_can->currentAdapter()) ? m_can->currentAdapter() : nullptr;
  m_isoTp = new IsoTpEngine(adapter, this);
  m_uds = new UdsEngine(m_isoTp, this);
  m_knowledge = new KnowledgeEngine(this);
  m_diagService = new DiagnosticService(m_uds, m_knowledge, this);

  setupUi();

  connect(m_diagService, &DiagnosticService::dtcListUpdated, this, &CanDiagnosticPanel::onDtcListUpdated);
  connect(m_diagService, &DiagnosticService::diagnosticMessage, this, &CanDiagnosticPanel::onDiagnosticMessage);
}

CanDiagnosticPanel::~CanDiagnosticPanel() {}

void CanDiagnosticPanel::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(2, 2, 2, 2);
  mainLayout->setSpacing(4);

  // 1. 标准视图头部栏 (与 CanViewPanel 风格一致)
  m_headerWidget = new QWidget(this);
  m_headerWidget->setObjectName("viewHeader");
  m_headerWidget->setFixedHeight(32);

  QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
  headerLayout->setContentsMargins(10, 0, 10, 0);

  m_titleLabel = new QLabel("🩺 UDS 故障诊断台", m_headerWidget);
  m_titleLabel->setObjectName("viewTitle");
  headerLayout->addWidget(m_titleLabel);
  headerLayout->addStretch();

  m_closeBtn = new QPushButton("✕", m_headerWidget);
  m_closeBtn->setObjectName("viewCloseBtn");
  m_closeBtn->setToolTip("关闭诊断台视图");
  m_closeBtn->setFixedSize(22, 22);
  connect(m_closeBtn, &QPushButton::clicked, this, [this]() { emit closeRequested(this); });
  headerLayout->addWidget(m_closeBtn);

  mainLayout->addWidget(m_headerWidget);

  // 2. 顶部基础配置
  QHBoxLayout *configLayout = new QHBoxLayout();
  configLayout->setContentsMargins(4, 2, 4, 2);
  configLayout->addWidget(new QLabel("请求ID (Tx):", this));
  m_editTxId = new QLineEdit("0x7E0", this);
  m_editTxId->setMaximumWidth(80);
  configLayout->addWidget(m_editTxId);

  configLayout->addWidget(new QLabel("应答ID (Rx):", this));
  m_editRxId = new QLineEdit("0x7E8", this);
  m_editRxId->setMaximumWidth(80);
  configLayout->addWidget(m_editRxId);

  m_chkTesterPresent = new QCheckBox("0x3E 会话保持 (TesterPresent)", this);
  configLayout->addWidget(m_chkTesterPresent);

  configLayout->addSpacing(15);
  QPushButton *btnS1 = new QPushButton("默认会话 (0x10 01)", this);
  QPushButton *btnS2 = new QPushButton("编程会话 (0x10 02)", this);
  QPushButton *btnS3 = new QPushButton("扩展会话 (0x10 03)", this);
  configLayout->addWidget(btnS1);
  configLayout->addWidget(btnS2);
  configLayout->addWidget(btnS3);
  configLayout->addStretch();
  mainLayout->addLayout(configLayout);

  connect(btnS1, &QPushButton::clicked, [this]() { onSessionControlClicked(0x01); });
  connect(btnS2, &QPushButton::clicked, [this]() { onSessionControlClicked(0x02); });
  connect(btnS3, &QPushButton::clicked, [this]() { onSessionControlClicked(0x03); });
  connect(m_chkTesterPresent, &QCheckBox::toggled, this, &CanDiagnosticPanel::onTesterPresentToggled);

  // 2. 主体区 (左右分割)
  QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

  // 左侧：DTC 故障诊断与 DID 读取
  QWidget *leftWidget = new QWidget(this);
  QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
  leftLayout->setContentsMargins(0, 0, 0, 0);

  // DTC 模块
  QGroupBox *dtcGroup = new QGroupBox("DTC 故障码诊断", this);
  QVBoxLayout *dtcLayout = new QVBoxLayout(dtcGroup);

  QHBoxLayout *dtcBtnLayout = new QHBoxLayout();
  m_btnReadDtc = new QPushButton("🔍 读取故障码 (0x19)", this);
  m_btnClearDtc = new QPushButton("🧹 一键清除 (0x14)", this);
  m_btnAiReport = new QPushButton("🤖 AI 专家排障分析", this);
  m_btnAiReport->setStyleSheet("font-weight: bold; color: #38bdf8;");

  dtcBtnLayout->addWidget(m_btnReadDtc);
  dtcBtnLayout->addWidget(m_btnClearDtc);
  dtcBtnLayout->addWidget(m_btnAiReport);
  dtcLayout->addLayout(dtcBtnLayout);

  m_dtcTree = new QTreeWidget(this);
  m_dtcTree->setHeaderLabels(QStringList() << "故障码" << "系统分类" << "故障定义描述" << "状态掩码");
  m_dtcTree->setColumnWidth(0, 100);
  m_dtcTree->setColumnWidth(1, 120);
  m_dtcTree->setColumnWidth(2, 220);
  dtcLayout->addWidget(m_dtcTree);
  leftLayout->addWidget(dtcGroup);

  // DID 模块
  QGroupBox *didGroup = new QGroupBox("DID 数据标识符读取 (0x22)", this);
  QHBoxLayout *didLayout = new QHBoxLayout(didGroup);
  m_comboDid = new QComboBox(this);
  m_comboDid->addItem("0xF190 (VIN 车辆识别码)", 0xF190);
  m_comboDid->addItem("0xF188 (软件版本号)", 0xF188);
  m_comboDid->addItem("0x0100 (ECU 运行状态)", 0x0100);
  m_btnReadDid = new QPushButton("读取", this);
  m_editDidResult = new QLineEdit(this);
  m_editDidResult->setReadOnly(true);
  m_editDidResult->setPlaceholderText("读取结果显示...");

  didLayout->addWidget(m_comboDid);
  didLayout->addWidget(m_btnReadDid);
  didLayout->addWidget(m_editDidResult);
  leftLayout->addWidget(didGroup);

  splitter->addWidget(leftWidget);

  // 右侧：自定义 UDS 指令与日志
  QWidget *rightWidget = new QWidget(this);
  QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
  rightLayout->setContentsMargins(0, 0, 0, 0);

  QGroupBox *customGroup = new QGroupBox("自定义 UDS 请求", this);
  QHBoxLayout *customLayout = new QHBoxLayout(customGroup);
  m_editCustomHex = new QLineEdit("22 F1 90", this);
  m_editCustomHex->setPlaceholderText("输入十六进制请求，如: 22 F1 90");
  m_btnSendCustom = new QPushButton("发送指令", this);
  customLayout->addWidget(m_editCustomHex);
  customLayout->addWidget(m_btnSendCustom);
  rightLayout->addWidget(customGroup);

  QGroupBox *logGroup = new QGroupBox("诊断交互详情日志", this);
  QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
  m_logEdit = new QPlainTextEdit(this);
  m_logEdit->setReadOnly(true);
  m_logEdit->setStyleSheet("background-color: #18181b; color: #a1a1aa; font-family: Consolas, monospace;");
  logLayout->addWidget(m_logEdit);
  rightLayout->addWidget(logGroup);

  splitter->addWidget(rightWidget);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);
  mainLayout->addWidget(splitter);

  // 绑定事件
  connect(m_btnReadDtc, &QPushButton::clicked, this, &CanDiagnosticPanel::onReadDtcClicked);
  connect(m_btnClearDtc, &QPushButton::clicked, this, &CanDiagnosticPanel::onClearDtcClicked);
  connect(m_btnReadDid, &QPushButton::clicked, this, &CanDiagnosticPanel::onReadDidClicked);
  connect(m_btnSendCustom, &QPushButton::clicked, this, &CanDiagnosticPanel::onSendCustomUdsClicked);
  connect(m_btnAiReport, &QPushButton::clicked, this, &CanDiagnosticPanel::onAiReportClicked);
}

void CanDiagnosticPanel::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss);
}

void CanDiagnosticPanel::log(const QString &msg) {
  QString time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
  m_logEdit->appendPlainText(QString("[%1] %2").arg(time, msg));
}

void CanDiagnosticPanel::onSessionControlClicked(int type) {
  IsoTpConfig cfg = m_isoTp->config();
  cfg.txId = m_editTxId->text().trimmed().toUInt(nullptr, 0);
  cfg.rxId = m_editRxId->text().trimmed().toUInt(nullptr, 0);
  m_isoTp->setConfig(cfg);

  m_diagService->setSession(static_cast<quint8>(type));
}

void CanDiagnosticPanel::onTesterPresentToggled(bool checked) {
  m_diagService->setTesterPresentEnabled(checked, 2000);
}

void CanDiagnosticPanel::onReadDtcClicked() {
  IsoTpConfig cfg = m_isoTp->config();
  cfg.txId = m_editTxId->text().trimmed().toUInt(nullptr, 0);
  cfg.rxId = m_editRxId->text().trimmed().toUInt(nullptr, 0);
  m_isoTp->setConfig(cfg);

  m_diagService->queryDtcList(0x09);
}

void CanDiagnosticPanel::onClearDtcClicked() {
  m_diagService->clearAllDtc();
}

void CanDiagnosticPanel::onReadDidClicked() {
  quint16 did = static_cast<quint16>(m_comboDid->currentData().toUInt());
  m_diagService->readDid(did);
}

void CanDiagnosticPanel::onSendCustomUdsClicked() {
  QString hex = m_editCustomHex->text().trimmed().remove(' ');
  QByteArray raw = QByteArray::fromHex(hex.toLatin1());
  if (raw.isEmpty()) return;

  quint8 sid = static_cast<quint8>(raw.at(0));
  QByteArray param = raw.mid(1);
  m_uds->sendRequest(sid, param);
}

void CanDiagnosticPanel::onAiReportClicked() {
  emit requestAiDiagnosticReport(m_currentDtcCodes);
}

void CanDiagnosticPanel::onDtcListUpdated(const QList<DiagnosticDtcItem> &dtcs) {
  m_dtcTree->clear();
  m_currentDtcCodes.clear();

  for (const auto &item : dtcs) {
    m_currentDtcCodes.append(item.code);
    QTreeWidgetItem *row = new QTreeWidgetItem(m_dtcTree);
    row->setText(0, item.code);
    row->setText(1, item.category);
    row->setText(2, item.title);
    row->setText(3, QString("0x%1").arg(item.statusMask, 2, 16, QLatin1Char('0')).toUpper());
    row->setForeground(0, QBrush(QColor(239, 68, 68))); // 红色高亮故障代码
  }
}

void CanDiagnosticPanel::onDiagnosticMessage(const QString &msg) {
  log(msg);
  if (msg.contains("DID") && msg.contains("成功")) {
    m_editDidResult->setText(msg.section(':', 1).trimmed());
  }
}
