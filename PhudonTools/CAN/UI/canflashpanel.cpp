#include "canflashpanel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDateTime>
#include <QSplitter>

CanFlashPanel::CanFlashPanel(CanInterface *can, QWidget *parent)
    : QWidget(parent), m_can(can) {
  ICanAdapter *adapter = (m_can && m_can->currentAdapter()) ? m_can->currentAdapter() : nullptr;
  m_isoTp = new IsoTpEngine(adapter, this);
  m_uds = new UdsEngine(m_isoTp, this);
  m_flashService = new FlashService(m_uds, this);

  setupUi();

  connect(m_flashService, &FlashService::stepChanged, this, &CanFlashPanel::onStepChanged);
  connect(m_flashService, &FlashService::progressChanged, this, &CanFlashPanel::onProgressChanged);
  connect(m_flashService, &FlashService::flashFinished, this, &CanFlashPanel::onFlashFinished);
  connect(m_flashService, &FlashService::logMessage, this, &CanFlashPanel::onLogMessage);
}

CanFlashPanel::~CanFlashPanel() {}

void CanFlashPanel::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(2, 2, 2, 2);
  mainLayout->setSpacing(4);

  // 1. 标准视图头部栏 (与 CanViewPanel 风格一致)
  m_headerWidget = new QWidget(this);
  m_headerWidget->setObjectName("viewHeader");
  m_headerWidget->setFixedHeight(32);

  QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
  headerLayout->setContentsMargins(10, 0, 10, 0);

  m_titleLabel = new QLabel("⚡ UDS 固件刷写工程台", m_headerWidget);
  m_titleLabel->setObjectName("viewTitle");
  headerLayout->addWidget(m_titleLabel);
  headerLayout->addStretch();

  m_closeBtn = new QPushButton("✕", m_headerWidget);
  m_closeBtn->setObjectName("viewCloseBtn");
  m_closeBtn->setToolTip("关闭刷写台视图");
  m_closeBtn->setFixedSize(22, 22);
  connect(m_closeBtn, &QPushButton::clicked, this, [this]() { emit closeRequested(this); });
  headerLayout->addWidget(m_closeBtn);

  mainLayout->addWidget(m_headerWidget);

  // 2. 固件文件选择
  QGroupBox *fileGroup = new QGroupBox("固件工程文件 (支持 Bin / Hex / S19)", this);
  QHBoxLayout *fileLayout = new QHBoxLayout(fileGroup);
  m_editFilePath = new QLineEdit(this);
  m_editFilePath->setPlaceholderText("选择或拖入固件文件...");
  m_btnBrowse = new QPushButton("📂 浏览固件...", this);
  fileLayout->addWidget(m_editFilePath);
  fileLayout->addWidget(m_btnBrowse);
  mainLayout->addWidget(fileGroup);

  // 2. 刷写参数与段信息
  QSplitter *midSplitter = new QSplitter(Qt::Horizontal, this);

  QWidget *paramWidget = new QWidget(this);
  QVBoxLayout *paramLayout = new QVBoxLayout(paramWidget);
  paramLayout->setContentsMargins(0, 0, 0, 0);

  QGroupBox *cfgGroup = new QGroupBox("UDS 刷写协议配置", this);
  QVBoxLayout *cfgLayout = new QVBoxLayout(cfgGroup);

  QHBoxLayout *row1 = new QHBoxLayout();
  row1->addWidget(new QLabel("Flash 基地址:", this));
  m_editBaseAddr = new QLineEdit("0x08008000", this);
  row1->addWidget(m_editBaseAddr);
  row1->addWidget(new QLabel("传输分块大小:", this));
  m_editBlockSize = new QLineEdit("512", this);
  row1->addWidget(m_editBlockSize);
  cfgLayout->addLayout(row1);

  QHBoxLayout *row2 = new QHBoxLayout();
  row2->addWidget(new QLabel("安全访问密钥:", this));
  m_editSecurityKey = new QLineEdit("00 00 00 00", this);
  row2->addWidget(m_editSecurityKey);
  cfgLayout->addLayout(row2);

  paramLayout->addWidget(cfgGroup);

  // 控制按钮
  QHBoxLayout *btnLayout = new QHBoxLayout();
  m_btnStartFlash = new QPushButton("🚀 开始刷写", this);
  m_btnStartFlash->setStyleSheet("background-color: #2563eb; color: white; font-weight: bold; padding: 6px 16px;");
  m_btnAbortFlash = new QPushButton("⏹️ 终止", this);
  m_btnAbortFlash->setEnabled(false);
  btnLayout->addWidget(m_btnStartFlash);
  btnLayout->addWidget(m_btnAbortFlash);
  paramLayout->addLayout(btnLayout);

  midSplitter->addWidget(paramWidget);

  // 段列表
  QGroupBox *segGroup = new QGroupBox("固件内存分布段", this);
  QVBoxLayout *segLayout = new QVBoxLayout(segGroup);
  m_segmentTree = new QTreeWidget(this);
  m_segmentTree->setHeaderLabels(QStringList() << "段索引" << "起始物理地址" << "段长度 (字节)");
  segLayout->addWidget(m_segmentTree);
  midSplitter->addWidget(segGroup);

  midSplitter->setStretchFactor(0, 2);
  midSplitter->setStretchFactor(1, 3);
  mainLayout->addWidget(midSplitter);

  // 3. 进度与状态
  QGroupBox *progressGroup = new QGroupBox("刷写进度与状态指示", this);
  QVBoxLayout *progLayout = new QVBoxLayout(progressGroup);

  QHBoxLayout *statusRow = new QHBoxLayout();
  m_stepLabel = new QLabel("状态: 空闲就绪", this);
  m_stepLabel->setStyleSheet("font-weight: bold; color: #38bdf8;");
  m_speedLabel = new QLabel("速率: 0.0 KB/s", this);
  statusRow->addWidget(m_stepLabel);
  statusRow->addStretch();
  statusRow->addWidget(m_speedLabel);
  progLayout->addLayout(statusRow);

  m_progressBar = new QProgressBar(this);
  m_progressBar->setRange(0, 100);
  m_progressBar->setValue(0);
  m_progressBar->setTextVisible(true);
  progLayout->addWidget(m_progressBar);
  mainLayout->addWidget(progressGroup);

  // 4. 详细日志
  QGroupBox *logGroup = new QGroupBox("刷写流程执行日志", this);
  QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
  m_logEdit = new QPlainTextEdit(this);
  m_logEdit->setReadOnly(true);
  m_logEdit->setStyleSheet("background-color: #18181b; color: #a1a1aa; font-family: Consolas, monospace;");
  logLayout->addWidget(m_logEdit);
  mainLayout->addWidget(logGroup);
  mainLayout->setStretchFactor(logGroup, 2);

  // 信号槽
  connect(m_btnBrowse, &QPushButton::clicked, this, &CanFlashPanel::onBrowseFileClicked);
  connect(m_btnStartFlash, &QPushButton::clicked, this, &CanFlashPanel::onStartFlashClicked);
  connect(m_btnAbortFlash, &QPushButton::clicked, this, &CanFlashPanel::onAbortFlashClicked);
}

void CanFlashPanel::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss);
}

void CanFlashPanel::log(const QString &msg) {
  QString time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
  m_logEdit->appendPlainText(QString("[%1] %2").arg(time, msg));
}

void CanFlashPanel::onBrowseFileClicked() {
  QString file = QFileDialog::getOpenFileName(this, "选择固件文件", "", "固件文件 (*.bin *.hex *.s19 *.srec);;所有文件 (*.*)");
  if (file.isEmpty()) return;

  m_editFilePath->setText(file);
  if (m_flashService->loadFirmwareFile(file)) {
    m_segmentTree->clear();
    int idx = 0;
    for (const auto &seg : m_flashService->segments()) {
      QTreeWidgetItem *item = new QTreeWidgetItem(m_segmentTree);
      item->setText(0, QString("Segment #%1").arg(idx + 1));
      item->setText(1, QString("0x%1").arg(seg.address, 8, 16, QLatin1Char('0')).toUpper());
      item->setText(2, QString("%1 字节").arg(seg.data.size()));
      idx++;
    }
  }
}

void CanFlashPanel::onStartFlashClicked() {
  if (m_flashService->segments().isEmpty()) {
    // 若未选文件，自动生成一段 4KB 虚拟固件供用户在离线仿真模式下体验完整刷写！
    QByteArray demoFirmware;
    demoFirmware.resize(4096);
    demoFirmware.fill(0x5A);
    quint32 base = m_editBaseAddr->text().toUInt(nullptr, 0);
    m_flashService->setFirmwareData(base, demoFirmware);

    m_segmentTree->clear();
    QTreeWidgetItem *item = new QTreeWidgetItem(m_segmentTree);
    item->setText(0, "虚拟固件 #1");
    item->setText(1, QString("0x%1").arg(base, 8, 16, QLatin1Char('0')).toUpper());
    item->setText(2, "4096 字节");
    log("提示: 未选择物理固件，已自动装载 4KB 仿真固件镜像供离线测试");
  }

  FlashConfig cfg;
  cfg.targetAddress = m_editBaseAddr->text().toUInt(nullptr, 0);
  cfg.blockSize = m_editBlockSize->text().toInt();
  if (cfg.blockSize <= 0) cfg.blockSize = 512;
  cfg.securityKey = QByteArray::fromHex(m_editSecurityKey->text().remove(' ').toLatin1());
  m_flashService->setConfig(cfg);

  m_btnStartFlash->setEnabled(false);
  m_btnAbortFlash->setEnabled(true);
  m_progressBar->setValue(0);

  m_flashService->startFlashing();
}

void CanFlashPanel::onAbortFlashClicked() {
  m_flashService->abortFlashing();
}

void CanFlashPanel::onStepChanged(FlashStep step, const QString &stepDesc) {
  Q_UNUSED(step);
  m_stepLabel->setText(QString("状态: %1").arg(stepDesc));
}

void CanFlashPanel::onProgressChanged(int percent, double speedKBs, int bytesTransferred, int totalBytes) {
  m_progressBar->setValue(percent);
  m_speedLabel->setText(QString("速率: %1 KB/s (%2 / %3 字节)").arg(speedKBs, 0, 'f', 1).arg(bytesTransferred).arg(totalBytes));
}

void CanFlashPanel::onFlashFinished(bool success, const QString &message) {
  m_btnStartFlash->setEnabled(true);
  m_btnAbortFlash->setEnabled(false);
  m_stepLabel->setText(QString("状态: %1").arg(success ? "刷写完成" : "刷写异常终止"));
  if (success) m_progressBar->setValue(100);
}

void CanFlashPanel::onLogMessage(const QString &msg) {
  log(msg);
}
