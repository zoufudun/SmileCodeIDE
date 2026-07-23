#include "canopenviewpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFontDatabase>
#include <QMessageBox>
#include <QDateTime>

CanOpenViewPanel::CanOpenViewPanel(CanOpenMaster *co, CanInterface *can, int viewId, QWidget *parent)
    : QWidget(parent), m_co(co), m_can(can), m_viewId(viewId) {
  setupUi();

  if (m_co) {
    connect(m_co, &CanOpenMaster::heartbeatReceived, this, &CanOpenViewPanel::onHeartbeat);
    connect(m_co, &CanOpenMaster::emcyReceived, this, &CanOpenViewPanel::onEmcy);
    connect(m_co, &CanOpenMaster::pdoReceived, this, &CanOpenViewPanel::onPdo);
    connect(m_co, &CanOpenMaster::sdoReadFinished, this, &CanOpenViewPanel::onSdoReadFinished);
    connect(m_co, &CanOpenMaster::sdoWriteFinished, this, &CanOpenViewPanel::onSdoWriteFinished);
    connect(m_co, &CanOpenMaster::logMessage, this, &CanOpenViewPanel::onCanOpenLog);
  }
}

CanOpenViewPanel::~CanOpenViewPanel() = default;

void CanOpenViewPanel::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(2, 2, 2, 2);
  mainLayout->setSpacing(4);

  // 1. Header with title and close button
  m_headerWidget = new QWidget();
  m_headerWidget->setObjectName("viewHeader");
  m_headerWidget->setStyleSheet("QWidget#viewHeader { background-color: #2f343f; border-radius: 4px; }");
  m_headerWidget->setFixedHeight(32);

  QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
  headerLayout->setContentsMargins(10, 0, 10, 0);

  m_titleLabel = new QLabel(QString("视图 %1: CANopen 测试").arg(m_viewId));
  m_titleLabel->setStyleSheet("color: #ffffff; font-weight: bold; font-size: 13px;");
  headerLayout->addWidget(m_titleLabel);
  headerLayout->addStretch();

  m_closeBtn = new QPushButton("✕");
  m_closeBtn->setFixedSize(20, 20);
  m_closeBtn->setStyleSheet("QPushButton { background-color: transparent; color: #abb2bf; border: none; font-weight: bold; font-size: 12px; }"
                            "QPushButton:hover { background-color: #e06c75; color: white; border-radius: 3px; }");
  connect(m_closeBtn, &QPushButton::clicked, this, [this]() { emit closeRequested(this); });
  headerLayout->addWidget(m_closeBtn);

  mainLayout->addWidget(m_headerWidget);

  // 2. NMT
  QGroupBox *nmtGroup = new QGroupBox("节点管理 (NMT)");
  QHBoxLayout *nmtLayout = new QHBoxLayout(nmtGroup);
  m_nodeIdSpin = new QSpinBox();
  m_nodeIdSpin->setRange(0, 127);
  m_nodeIdSpin->setValue(1);
  m_nodeIdSpin->setPrefix("节点 ");
  
  // Set local state change or sync if needed
  m_nmtCommandCombo = new QComboBox();
  m_nmtCommandCombo->addItem("启动节点 (Start)", 0x01);
  m_nmtCommandCombo->addItem("停止节点 (Stop)", 0x02);
  m_nmtCommandCombo->addItem("进入预运行 (Pre-op)", 0x80);
  m_nmtCommandCombo->addItem("复位节点 (Reset Node)", 0x81);
  m_nmtCommandCombo->addItem("复位通信 (Reset Comm)", 0x82);
  m_nmtSendButton = new QPushButton("发送 NMT");
  m_syncButton = new QPushButton("发送 SYNC");
  m_nmtSendButton->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 12px; }");
  m_syncButton->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 12px; }");

  connect(m_nmtSendButton, &QPushButton::clicked, this, &CanOpenViewPanel::onNmtSend);
  connect(m_syncButton, &QPushButton::clicked, this, &CanOpenViewPanel::onSyncSend);

  nmtLayout->addWidget(m_nodeIdSpin);
  nmtLayout->addWidget(m_nmtCommandCombo, 1);
  nmtLayout->addWidget(m_nmtSendButton);
  nmtLayout->addWidget(m_syncButton);
  mainLayout->addWidget(nmtGroup);

  // 3. SDO
  QGroupBox *sdoGroup = new QGroupBox("SDO 服务数据对象 (快速传输)");
  QGridLayout *sdoLayout = new QGridLayout(sdoGroup);
  m_sdoIndexEdit = new QLineEdit("6040");
  m_sdoSubIndexEdit = new QLineEdit("00");
  m_sdoValueEdit = new QLineEdit("000F");
  m_sdoSizeCombo = new QComboBox();
  m_sdoSizeCombo->addItem("1 字节", 1);
  m_sdoSizeCombo->addItem("2 字节", 2);
  m_sdoSizeCombo->addItem("3 字节", 3);
  m_sdoSizeCombo->addItem("4 字节", 4);
  m_sdoSizeCombo->setCurrentIndex(1);
  m_sdoReadButton = new QPushButton("读取 (Upload)");
  m_sdoWriteButton = new QPushButton("写入 (Download)");
  m_sdoReadButton->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 12px; }");
  m_sdoWriteButton->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 12px; }");

  connect(m_sdoReadButton, &QPushButton::clicked, this, &CanOpenViewPanel::onSdoRead);
  connect(m_sdoWriteButton, &QPushButton::clicked, this, &CanOpenViewPanel::onSdoWrite);

  sdoLayout->addWidget(new QLabel("索引(Hex):"), 0, 0);
  sdoLayout->addWidget(m_sdoIndexEdit, 0, 1);
  sdoLayout->addWidget(new QLabel("子索引(Hex):"), 0, 2);
  sdoLayout->addWidget(m_sdoSubIndexEdit, 0, 3);
  sdoLayout->addWidget(new QLabel("数值(Hex):"), 1, 0);
  sdoLayout->addWidget(m_sdoValueEdit, 1, 1);
  sdoLayout->addWidget(new QLabel("长度:"), 1, 2);
  sdoLayout->addWidget(m_sdoSizeCombo, 1, 3);
  sdoLayout->addWidget(m_sdoReadButton, 2, 0, 1, 2);
  sdoLayout->addWidget(m_sdoWriteButton, 2, 2, 1, 2);
  mainLayout->addWidget(sdoGroup);

  // 4. Node Monitor & Log
  QGroupBox *nodeGroup = new QGroupBox("节点状态 / 心跳监控");
  QVBoxLayout *nodeLayout = new QVBoxLayout(nodeGroup);
  m_nodeTreeWidget = new QTreeWidget();
  m_nodeTreeWidget->setHeaderLabels({"节点ID", "状态", "最近更新"});
  m_nodeTreeWidget->setColumnWidth(0, 80);
  m_nodeTreeWidget->setColumnWidth(1, 120);
  m_nodeTreeWidget->setRootIsDecorated(false);
  nodeLayout->addWidget(m_nodeTreeWidget);

  QGroupBox *logGroup = new QGroupBox("CANopen 日志");
  QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
  m_canOpenLog = new QPlainTextEdit();
  m_canOpenLog->setReadOnly(true);
  m_canOpenLog->setMaximumBlockCount(500);
  m_canOpenLog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  logLayout->addWidget(m_canOpenLog);

  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->addWidget(nodeGroup, 1);
  bottomLayout->addWidget(logGroup, 1);

  mainLayout->addLayout(bottomLayout, 1);
}

void CanOpenViewPanel::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss);
  m_headerWidget->setStyleSheet("QWidget#viewHeader { background-color: #2f343f; border-radius: 4px; }");
}

void CanOpenViewPanel::onNmtSend() {
  if (!m_co) return;
  const NmtCommand cmd = static_cast<NmtCommand>(m_nmtCommandCombo->currentData().toInt());
  m_co->sendNmt(cmd, static_cast<quint8>(m_nodeIdSpin->value()));
}

void CanOpenViewPanel::onSyncSend() {
  if (!m_co) return;
  m_co->sendSync();
}

void CanOpenViewPanel::onSdoRead() {
  if (!m_co) return;
  bool ok1 = false, ok2 = false;
  const quint16 index = static_cast<quint16>(m_sdoIndexEdit->text().trimmed().toUInt(&ok1, 16));
  const quint8 sub = static_cast<quint8>(m_sdoSubIndexEdit->text().trimmed().toUInt(&ok2, 16));
  if (!ok1 || !ok2) {
    QMessageBox::warning(this, "提示", "索引/子索引格式错误。");
    return;
  }
  m_co->setNodeId(static_cast<quint8>(m_nodeIdSpin->value()));
  m_co->sdoRead(index, sub);
}

void CanOpenViewPanel::onSdoWrite() {
  if (!m_co) return;
  bool ok1 = false, ok2 = false, ok3 = false;
  const quint16 index = static_cast<quint16>(m_sdoIndexEdit->text().trimmed().toUInt(&ok1, 16));
  const quint8 sub = static_cast<quint8>(m_sdoSubIndexEdit->text().trimmed().toUInt(&ok2, 16));
  const quint32 value = m_sdoValueEdit->text().trimmed().toUInt(&ok3, 16);
  if (!ok1 || !ok2 || !ok3) {
    QMessageBox::warning(this, "提示", "索引/子索引/数值格式错误。");
    return;
  }
  m_co->setNodeId(static_cast<quint8>(m_nodeIdSpin->value()));
  m_co->sdoWrite(index, sub, value, m_sdoSizeCombo->currentData().toInt());
}

void CanOpenViewPanel::onHeartbeat(quint8 nodeId, NmtState state) {
  const QString idText = QString::number(nodeId);
  QTreeWidgetItem *item = nullptr;
  for (int i = 0; i < m_nodeTreeWidget->topLevelItemCount(); ++i) {
    if (m_nodeTreeWidget->topLevelItem(i)->text(0) == idText) {
      item = m_nodeTreeWidget->topLevelItem(i);
      break;
    }
  }
  if (!item) {
    item = new QTreeWidgetItem(QStringList{idText});
    m_nodeTreeWidget->addTopLevelItem(item);
  }
  item->setText(1, CanOpenMaster::stateText(state));
  item->setText(2, QDateTime::currentDateTime().toString("HH:mm:ss"));
}

void CanOpenViewPanel::onEmcy(quint8 nodeId, quint16 errorCode, quint8 errorRegister, const QByteArray &manufacturer) {
  appendCanOpenLog(QStringLiteral("[EMCY] 节点 %1 错误码=0x%2 错误寄存器=0x%3 厂商=%4")
                       .arg(nodeId)
                       .arg(errorCode, 4, 16, QChar('0'))
                       .arg(errorRegister, 2, 16, QChar('0'))
                       .arg(QString::fromLatin1(manufacturer.toHex(' ').toUpper())));
}

void CanOpenViewPanel::onPdo(quint8 nodeId, int pdoNumber, bool isTpdo, const QByteArray &data) {
  appendCanOpenLog(QStringLiteral("[%1PDO%2] 节点 %3 数据=%4")
                       .arg(isTpdo ? "T" : "R")
                       .arg(pdoNumber)
                       .arg(nodeId)
                       .arg(QString::fromLatin1(data.toHex(' ').toUpper())));
}

void CanOpenViewPanel::onSdoReadFinished(bool success, quint16 index, quint8 subIndex, quint32 value, quint32 abortCode) {
  if (success) {
    appendCanOpenLog(QStringLiteral("[SDO 读] [%1:%2] = 0x%3 (%4)")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex)
                         .arg(value, 0, 16)
                         .arg(value));
  } else {
    appendCanOpenLog(QStringLiteral("[SDO 读失败] [%1:%2] %3")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex)
                         .arg(CanOpenMaster::abortText(abortCode)));
  }
}

void CanOpenViewPanel::onSdoWriteFinished(bool success, quint16 index, quint8 subIndex, quint32 abortCode) {
  if (success) {
    appendCanOpenLog(QStringLiteral("[SDO 写] [%1:%2] 成功")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex));
  } else {
    appendCanOpenLog(QStringLiteral("[SDO 写失败] [%1:%2] %3")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex)
                         .arg(CanOpenMaster::abortText(abortCode)));
  }
}

void CanOpenViewPanel::onCanOpenLog(const QString &message) {
  appendCanOpenLog(message);
}

void CanOpenViewPanel::appendCanOpenLog(const QString &text) {
  const QString time = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  m_canOpenLog->appendPlainText(QStringLiteral("%1  %2").arg(time, text));
}
