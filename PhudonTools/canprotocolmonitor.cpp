#include "canprotocolmonitor.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

#include "caninterface.h"
#include "candevicedialog.h"
#include "devicestatuswidget.h"

CanProtocolMonitor::CanProtocolMonitor(QWidget *parent)
    : QDialog(parent), m_can(new CanInterface(this)) {
  setWindowTitle(QStringLiteral("设备状态监控面板"));
  setMinimumSize(800, 600);
  resize(960, 700);
  setStyleSheet("QDialog { background-color: #F4F6F9; }");

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(12, 12, 12, 12);
  mainLayout->setSpacing(8);

  // ---- 工具栏 ----
  auto *toolbar = new QHBoxLayout();
  toolbar->setSpacing(8);

  QString btnBase =
      "QPushButton { padding: 6px 14px; border-radius: 4px; font-size: 12px; "
      "font-weight: bold; }";

  m_btnDev = new QPushButton(QStringLiteral("设备管理"));
  m_btnDev->setStyleSheet(btnBase +
      "QPushButton { background-color: #D5F5E3; border: 1px solid #A9DFBF; "
      "color: #1E8449; } QPushButton:hover { background-color: #ABEBC6; }");
  toolbar->addWidget(m_btnDev);

  m_btnConfig = new QPushButton(QStringLiteral("⚙ 配置"));
  m_btnConfig->setStyleSheet(btnBase +
      "QPushButton { background-color: #D6E8FC; border: 1px solid #ADC3E6; "
      "color: #2C3E50; } QPushButton:hover { background-color: #C0DEFC; }");
  toolbar->addWidget(m_btnConfig);

  m_btnImport = new QPushButton(QStringLiteral("导入"));
  m_btnImport->setStyleSheet(btnBase +
      "QPushButton { background-color: #E8ECEF; border: 1px solid #CFD8DC; "
      "color: #555; } QPushButton:hover { background-color: #DFE3E6; }");
  toolbar->addWidget(m_btnImport);

  m_btnExport = new QPushButton(QStringLiteral("导出"));
  m_btnExport->setStyleSheet(btnBase +
      "QPushButton { background-color: #E8ECEF; border: 1px solid #CFD8DC; "
      "color: #555; } QPushButton:hover { background-color: #DFE3E6; }");
  toolbar->addWidget(m_btnExport);

  m_btnClear = new QPushButton(QStringLiteral("清空"));
  m_btnClear->setStyleSheet(btnBase +
      "QPushButton { background-color: #FCE4E4; border: 1px solid #F5B7B1; "
      "color: #922B21; } QPushButton:hover { background-color: #FADBD8; }");
  toolbar->addWidget(m_btnClear);

  toolbar->addStretch();

  // 连接状态指示
  m_connStatus = new QLabel(QStringLiteral("● 未连接"));
  m_connStatus->setStyleSheet(
      "color: #95A5A6; font-size: 12px; font-weight: bold; "
      "padding: 4px 12px; background: #F8FAFC; border-radius: 4px; "
      "border: 1px solid #E2E8F0;");
  toolbar->addWidget(m_connStatus);

  mainLayout->addLayout(toolbar);

  // ---- 设备网格区域 (可滚动) ----
  m_scrollArea = new QScrollArea();
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setStyleSheet(
      "QScrollArea { border: 1px solid #E2E8F0; border-radius: 6px; "
      "background: white; }");

  m_gridContainer = new QWidget();
  m_gridContainer->setStyleSheet("background: white;");
  m_gridLayout = new QGridLayout(m_gridContainer);
  m_gridLayout->setContentsMargins(16, 16, 16, 16);
  m_gridLayout->setSpacing(12);
  m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

  m_scrollArea->setWidget(m_gridContainer);
  mainLayout->addWidget(m_scrollArea, 1);

  // ---- 日志区域 ----
  m_log = new QPlainTextEdit();
  m_log->setReadOnly(true);
  m_log->setMaximumHeight(140);
  m_log->setPlaceholderText(QStringLiteral("告警事件日志…"));
  m_log->setStyleSheet(
      "QPlainTextEdit { border: 1px solid #E2E8F0; border-radius: 4px; "
      "background: #FAFBFC; font-size: 11px; font-family: Consolas, monospace; "
      "padding: 6px; }");
  mainLayout->addWidget(m_log);

  // ---- 信号连接 ----
  m_batchTimer = new QTimer(this);
  m_batchTimer->setInterval(33);
  connect(m_batchTimer, &QTimer::timeout, this, &CanProtocolMonitor::processBatch);
  m_batchTimer->start();

  connect(m_can, &CanInterface::frameReceived, this,
          &CanProtocolMonitor::onFrameReceived);
  connect(m_can, &CanInterface::frameSent, this,
          &CanProtocolMonitor::onFrameReceived);
  connect(m_can, &CanInterface::connected, this,
          &CanProtocolMonitor::onCanConnected);
  connect(m_can, &CanInterface::disconnected, this,
          &CanProtocolMonitor::onCanDisconnected);

  connect(m_btnDev, &QPushButton::clicked, this,
          &CanProtocolMonitor::onDeviceManage);
  connect(m_btnConfig, &QPushButton::clicked, this,
          &CanProtocolMonitor::onConfigClicked);
  connect(m_btnImport, &QPushButton::clicked, this,
          &CanProtocolMonitor::onImportClicked);
  connect(m_btnExport, &QPushButton::clicked, this,
          &CanProtocolMonitor::onExportClicked);
  connect(m_btnClear, &QPushButton::clicked, this,
          &CanProtocolMonitor::onClearClicked);

  // 初始化：加载配置，同步连接状态
  loadConfig();
  if (m_can->isDeviceOpen()) {
    onCanConnected();
  }
}

CanProtocolMonitor::~CanProtocolMonitor() { saveConfig(); }

void CanProtocolMonitor::closeEvent(QCloseEvent *event) {
  saveConfig();
  emit closed();
  QDialog::closeEvent(event);
}

// ========== 设备管理 ==========

void CanProtocolMonitor::onDeviceManage() {
  if (!m_devDialog) {
    m_devDialog = new CanDeviceDialog(m_can, this);
  }
  m_devDialog->exec();
  // 设备管理对话框关闭后刷新连接状态
  if (m_can->isDeviceOpen()) {
    onCanConnected();
  }
}

// ========== 网格重建 ==========

void CanProtocolMonitor::rebuildGrid() {
  // 清除旧 widget
  qDeleteAll(m_deviceWidgets);
  m_deviceWidgets.clear();

  // 清除旧布局
  while (m_gridLayout->count() > 0) {
    QLayoutItem *item = m_gridLayout->takeAt(0);
    delete item;
  }

  // 默认占位提示
  if (m_mappings.isEmpty()) {
    auto *placeholder = new QLabel(
        QStringLiteral("暂无设备配置\n\n"
                       "1. 点击「设备管理」打开 CAN 设备并启动通道\n"
                       "2. 点击「⚙ 配置」添加设备位映射\n"
                       "3. 设备状态将根据 CAN 报文实时更新"));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet(
        "color: #95A5A6; font-size: 15px; padding: 60px; "
        "background: transparent; line-height: 1.8;");
    placeholder->setMinimumHeight(250);
    m_gridLayout->addWidget(placeholder, 0, 0, 1, m_gridCols, Qt::AlignCenter);
    m_gridContainer->updateGeometry();
    return;
  }

  // 创建 DeviceStatusWidget
  for (int i = 0; i < m_mappings.size(); ++i) {
    const auto &mapping = m_mappings[i];

    DeviceStatusWidget::DeviceKind kind = DeviceStatusWidget::Detector;
    if (mapping.deviceType == QStringLiteral("valve")) {
      kind = DeviceStatusWidget::Valve;
    } else if (mapping.deviceType == QStringLiteral("valve_distributor")) {
      kind = DeviceStatusWidget::ValveDistributor;
    } else if (mapping.deviceType == QStringLiteral("valve_zone")) {
      kind = DeviceStatusWidget::ValveZone;
    } else if (mapping.deviceType == QStringLiteral("valve_main_isolation")) {
      kind = DeviceStatusWidget::ValveMainIsolation;
    } else if (mapping.deviceType == QStringLiteral("manual_alarm")) {
      kind = DeviceStatusWidget::ManualAlarm;
    } else if (mapping.deviceType == QStringLiteral("gas_cylinder")) {
      kind = DeviceStatusWidget::GasCylinder;
    } else if (mapping.deviceType == QStringLiteral("water_pump")) {
      kind = DeviceStatusWidget::WaterPump;
    } else if (mapping.deviceType == QStringLiteral("pressure_switch")) {
      kind = DeviceStatusWidget::PressureSwitch;
    } else if (mapping.deviceType == QStringLiteral("mobile_spray_gun")) {
      kind = DeviceStatusWidget::MobileSprayGun;
    }

    auto *widget =
        new DeviceStatusWidget(mapping.deviceId, kind, mapping.label, mapping.canId);
    widget->setDefaultVal(mapping.defaultVal);
    widget->setStatus(mapping.defaultVal == 1);
    connect(widget, &DeviceStatusWidget::editRequested, this,
            &CanProtocolMonitor::onConfigClicked);

    int row = i / m_gridCols;
    int col = i % m_gridCols;
    m_gridLayout->addWidget(widget, row, col, Qt::AlignCenter);

    m_deviceWidgets[mapping.deviceId] = widget;
  }

  m_gridContainer->updateGeometry();
}

// ========== CAN 帧处理 ==========

void CanProtocolMonitor::onFrameReceived(const CanFrame &frame) {
  if (m_mappings.isEmpty()) return;
  m_ringBuffer.push(frame);
}

void CanProtocolMonitor::processBatch() {
  if (m_ringBuffer.isEmpty()) return;

  std::vector<CanFrame> batch;
  m_ringBuffer.pop_batch(batch, 4096);

  for (const auto &frame : batch) {
    for (const auto &mapping : m_mappings) {
      if (frame.id != mapping.canId) continue;
      if (mapping.canChannel != -1 && frame.channel != mapping.canChannel) continue;
      if (mapping.byteIndex >= frame.data.size()) continue;

      const quint8 byteVal =
          static_cast<quint8>(frame.data[mapping.byteIndex]);
      const bool bitVal = (byteVal >> mapping.bitIndex) & 0x01;

      auto *widget = m_deviceWidgets.value(mapping.deviceId, nullptr);
      if (!widget) continue;

      const bool prev = widget->status();
      widget->setStatus(bitVal);

      // 状态变化时记录日志
      if (bitVal != prev) {
        QString direction =
            bitVal ? QStringLiteral("→ 报警") : QStringLiteral("→ 恢复正常");
        if (mapping.deviceType == QStringLiteral("valve")) {
          direction =
              bitVal ? QStringLiteral("→ 开启") : QStringLiteral("→ 关闭");
        }
        const QString time =
            QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        appendLog(
            QStringLiteral("[%1] CAN ID=0x%2 字节%3.位%4 %5 %6")
                .arg(time)
                .arg(frame.id, 0, 16)
                .arg(mapping.byteIndex)
                .arg(mapping.bitIndex)
                .arg(mapping.label)
                .arg(direction),
            bitVal);
      }
    }
  }
}

// ========== 连接状态 ==========

void CanProtocolMonitor::onCanConnected() {
  m_connStatus->setText(QStringLiteral("● 已连接"));
  m_connStatus->setStyleSheet(
      "color: #27AE60; font-size: 12px; font-weight: bold; "
      "padding: 4px 12px; background: #E8F8F5; border-radius: 4px; "
      "border: 1px solid #A9DFBF;");
}

void CanProtocolMonitor::onCanDisconnected() {
  m_connStatus->setText(QStringLiteral("● 未连接"));
  m_connStatus->setStyleSheet(
      "color: #95A5A6; font-size: 12px; font-weight: bold; "
      "padding: 4px 12px; background: #F8FAFC; border-radius: 4px; "
      "border: 1px solid #E2E8F0;");
}

// ========== 配置按钮 ==========

void CanProtocolMonitor::onConfigClicked() {
  CanProtocolConfigDialog dlg(this);
  dlg.setMappings(m_mappings);

  if (dlg.exec() == QDialog::Accepted) {
    m_mappings = dlg.mappings();
    rebuildGrid();
    saveConfig();
    appendLog(QStringLiteral("配置已更新 (%1 个设备)").arg(m_mappings.size()));
  }
}

void CanProtocolMonitor::onImportClicked() {
  QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("导入设备配置"), QString(),
      QStringLiteral("JSON 文件 (*.json)"));
  if (path.isEmpty()) return;

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, QStringLiteral("导入失败"),
                          QStringLiteral("无法读取文件"));
    return;
  }

  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
  file.close();

  if (err.error != QJsonParseError::NoError || !doc.isArray()) {
    QMessageBox::critical(this, QStringLiteral("格式错误"),
                          QStringLiteral("JSON 解析失败"));
    return;
  }

  m_mappings.clear();
  for (const QJsonValue &val : doc.array()) {
    QJsonObject obj = val.toObject();
    DeviceBitMapping m;
    m.deviceId = obj.value("deviceId").toInt(1);
    m.label = obj.value("label").toString(QStringLiteral("未知"));
    m.deviceType =
        obj.value("deviceType").toString(QStringLiteral("detector"));
    m.canId = static_cast<quint32>(obj.value("canId").toInt(0x100));
    m.byteIndex = obj.value("byteIndex").toInt(0);
    m.bitIndex = obj.value("bitIndex").toInt(0);
    m_mappings.append(m);
  }

  rebuildGrid();
  saveConfig();
  appendLog(QStringLiteral("已导入 %1 个设备配置").arg(m_mappings.size()));
}

void CanProtocolMonitor::onExportClicked() {
  QJsonArray arr;
  for (const auto &m : m_mappings) {
    QJsonObject obj;
    obj["deviceId"] = m.deviceId;
    obj["label"] = m.label;
    obj["deviceType"] = m.deviceType;
    obj["canId"] = static_cast<int>(m.canId);
    obj["byteIndex"] = m.byteIndex;
    obj["bitIndex"] = m.bitIndex;
    arr.append(obj);
  }

  QString path = QFileDialog::getSaveFileName(
      this, QStringLiteral("导出设备配置"),
      QStringLiteral("device_config.json"),
      QStringLiteral("JSON 文件 (*.json)"));
  if (path.isEmpty()) return;

  QFile file(path);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    file.close();
  }
}

void CanProtocolMonitor::onClearClicked() {
  // 将所有设备状态重置为 0
  for (auto *widget : m_deviceWidgets) {
    widget->setStatus(false);
  }
  m_log->clear();
  appendLog(QStringLiteral("所有设备状态已重置"));
}

// ========== 持久化 (JSON) ==========

void CanProtocolMonitor::saveConfig() {
  QJsonArray arr;
  for (const auto &m : m_mappings) {
    QJsonObject obj;
    obj["deviceId"] = m.deviceId;
    obj["label"] = m.label;
    obj["deviceType"] = m.deviceType;
    obj["canId"] = static_cast<int>(m.canId);
    obj["byteIndex"] = m.byteIndex;
    obj["bitIndex"] = m.bitIndex;
    arr.append(obj);
  }

  QFile file(QStringLiteral("device_config.json"));
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    file.close();
  }
}

void CanProtocolMonitor::loadConfig() {
  QFile file(QStringLiteral("device_config.json"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
  file.close();

  if (err.error != QJsonParseError::NoError || !doc.isArray()) return;

  m_mappings.clear();
  for (const QJsonValue &val : doc.array()) {
    QJsonObject obj = val.toObject();
    DeviceBitMapping m;
    m.deviceId = obj.value("deviceId").toInt(1);
    m.label = obj.value("label").toString(QStringLiteral("未知"));
    m.deviceType =
        obj.value("deviceType").toString(QStringLiteral("detector"));
    m.canId = static_cast<quint32>(obj.value("canId").toInt(0x100));
    m.byteIndex = obj.value("byteIndex").toInt(0);
    m.bitIndex = obj.value("bitIndex").toInt(0);
    m_mappings.append(m);
  }

  rebuildGrid();
}

// ========== 日志 ==========

void CanProtocolMonitor::appendLog(const QString &text, bool isAlarm) {
  m_log->setUpdatesEnabled(false);
  if (isAlarm) {
    m_log->appendHtml(
        QStringLiteral(
            "<span style='color:#E74C3C;font-weight:bold;'>%1</span>")
            .arg(text.toHtmlEscaped()));
  } else {
    m_log->appendPlainText(text);
  }

  // 限制日志行数（单次高效率裁剪）
  QTextDocument *doc = m_log->document();
  int extraBlocks = doc->blockCount() - 500;
  if (extraBlocks > 0) {
    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, extraBlocks);
    cursor.removeSelectedText();
  }

  // 自动滚动到底部
  m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
  m_log->setUpdatesEnabled(true);
}
