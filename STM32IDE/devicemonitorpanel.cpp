#include "devicemonitorpanel.h"

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBlock>
#include <QTextCursor>
#include <QVBoxLayout>

#include "caninterface.h"
#include "canprotocolconfigdialog.h"
#include "devicestatuswidget.h"

// ========== 科技风顶部标题栏 (自定义绘制) ==========

class MonitorHeader : public QWidget {
public:
  explicit MonitorHeader(QWidget *parent = nullptr) : QWidget(parent) {
    setFixedHeight(52);
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 渐变背景
    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0, QColor(0x14, 0x1C, 0x2C));
    bg.setColorAt(1, QColor(0x0D, 0x12, 0x1E));
    p.fillRect(rect(), bg);

    // 底部发光分割线
    QLinearGradient line(0, height() - 1, width(), height() - 1);
    line.setColorAt(0, QColor(0x00, 0xD4, 0xFF, 0));
    line.setColorAt(0.3, QColor(0x00, 0xD4, 0xFF, 80));
    line.setColorAt(0.5, QColor(0x00, 0xD4, 0xFF, 120));
    line.setColorAt(0.7, QColor(0x00, 0xD4, 0xFF, 80));
    line.setColorAt(1, QColor(0x00, 0xD4, 0xFF, 0));
    p.setPen(QPen(line, 1));
    p.drawLine(0, height() - 1, width(), height() - 1);

    // 左侧装饰竖线
    p.fillRect(0, 0, 3, height(), QColor(0x00, 0xD4, 0xFF));

    // 标题文字
    p.setPen(QColor(0xE2, 0xE8, 0xF0));
    QFont f("Microsoft YaHei", 12, QFont::Bold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    p.setFont(f);
    p.drawText(14, 4, width() - 28, height() - 8,
               Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("设备状态监控"));

    // 右侧状态点 + 文字
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x10, 0xB9, 0x81));
    p.drawEllipse(QPoint(width() - 60, height() / 2), 4, 4);

    p.setPen(QColor(0x7C, 0x87, 0x9A));
    p.setFont(QFont("Consolas", 8));
    p.drawText(width() - 130, 4, 110, height() - 8,
               Qt::AlignVCenter | Qt::AlignRight,
               QStringLiteral("MONITOR  v1.0"));
  }
};

// ================================================================

DeviceMonitorPanel::DeviceMonitorPanel(CanInterface *can, QWidget *parent)
    : QWidget(parent), m_can(can) {
  setupUi();

  // 连接 CAN 帧接收信号
  connect(m_can, &CanInterface::frameReceived, this,
          &DeviceMonitorPanel::onFrameReceived);

  // 加载持久化配置
  loadConfig();

  // 启动 WebSocket 服务器
  m_wsServer = new QWebSocketServer(QStringLiteral("StatusMonitorServer"), QWebSocketServer::NonSecureMode, this);
  if (m_wsServer->listen(QHostAddress::Any, 12345)) {
    connect(m_wsServer, &QWebSocketServer::newConnection, this, &DeviceMonitorPanel::onNewConnection);
    appendLog(QStringLiteral("[WebSocket] 服务器已启动，监听端口 12345"), false);
  } else {
    appendLog(QStringLiteral("[WebSocket] 服务器启动失败，端口 12345 被占用"), true);
  }
}

DeviceMonitorPanel::~DeviceMonitorPanel() {
  saveConfig();
  if (m_wsServer) {
    m_wsServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
  }
}

void DeviceMonitorPanel::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // ---- 科技风顶部标题栏 ----
  auto *header = new MonitorHeader(this);
  mainLayout->addWidget(header);

  // ---- 工具栏 ----
  auto *toolbar = new QWidget();
  toolbar->setFixedHeight(42);
  toolbar->setStyleSheet(
      "QWidget { background-color: #111827; border-bottom: 1px solid #1E293E; }");

  auto *tbLayout = new QHBoxLayout(toolbar);
  tbLayout->setContentsMargins(10, 5, 10, 5);
  tbLayout->setSpacing(8);

  const QString techBtn =
      "QPushButton { padding: 5px 14px; border-radius: 3px; font-size: 11px; "
      "font-weight: bold; font-family: 'Microsoft YaHei'; }";

  m_btnConfig = new QPushButton(QStringLiteral("⚙ 设备配置"));
  m_btnConfig->setStyleSheet(techBtn +
      "QPushButton { color: #00D4FF; background: #1A2740; "
      "border: 1px solid #1E3A5F; } "
      "QPushButton:hover { background: #1E3A5F; border-color: #00D4FF; }");
  tbLayout->addWidget(m_btnConfig);

  m_btnImport = new QPushButton(QStringLiteral("导入"));
  m_btnImport->setStyleSheet(techBtn +
      "QPushButton { color: #7C879A; background: #1A2235; "
      "border: 1px solid #1E293E; } "
      "QPushButton:hover { color: #E2E8F0; border-color: #3B82F6; }");
  tbLayout->addWidget(m_btnImport);

  m_btnExport = new QPushButton(QStringLiteral("导出"));
  m_btnExport->setStyleSheet(techBtn +
      "QPushButton { color: #7C879A; background: #1A2235; "
      "border: 1px solid #1E293E; } "
      "QPushButton:hover { color: #E2E8F0; border-color: #3B82F6; }");
  tbLayout->addWidget(m_btnExport);

  m_btnReset = new QPushButton(QStringLiteral("重置"));
  m_btnReset->setStyleSheet(techBtn +
      "QPushButton { color: #F87171; background: #271A1A; "
      "border: 1px solid #3E1E1E; } "
      "QPushButton:hover { background: #3E1E1E; border-color: #EF4444; }");
  tbLayout->addWidget(m_btnReset);

  tbLayout->addStretch();

  auto *statusLabel = new QLabel(QStringLiteral("CAN 2.0B Protocol Monitor"));
  statusLabel->setStyleSheet(
      "color: #475569; font-size: 10px; "
      "font-family: 'Consolas', monospace; padding-right: 4px;");
  tbLayout->addWidget(statusLabel);

  mainLayout->addWidget(toolbar);

  // ---- 页面主体 (采用分割器以支持日志高度拖动调节) ----
  QSplitter *splitter = new QSplitter(Qt::Vertical, this);
  splitter->setChildrenCollapsible(false);
  splitter->setStyleSheet("QSplitter::handle { background-color: #1E293E; height: 3px; }");

  // ---- 设备网格区域 (可滚动) ----
  m_scrollArea = new QScrollArea();
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setStyleSheet(
      "QScrollArea { background-color: #0D1117; border: none; }"
      "QScrollBar:vertical { background: #0D1117; width: 8px; }"
      "QScrollBar::handle:vertical { background: #1E293E; border-radius: 4px; "
      "min-height: 30px; }"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
      "height: 0px; }");

  m_gridContainer = new QWidget();
  m_gridContainer->setStyleSheet("background-color: #0D1117;");
  m_gridLayout = new QGridLayout(m_gridContainer);
  m_gridLayout->setContentsMargins(20, 20, 20, 20);
  m_gridLayout->setSpacing(18);
  m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

  m_scrollArea->setWidget(m_gridContainer);
  splitter->addWidget(m_scrollArea);

  // ---- 事件日志 ----
  m_log = new QPlainTextEdit();
  m_log->setReadOnly(true);
  m_log->setFrameShape(QFrame::NoFrame);
  m_log->setStyleSheet(
      "QPlainTextEdit { background-color: #0A0E14; color: #7C879A; "
      "font-size: 11px; font-family: 'Microsoft YaHei', 'Consolas', monospace; "
      "padding: 6px; border-top: 1px solid #1E293E; }");
  m_log->setPlaceholderText(QStringLiteral("告警事件日志…"));
  splitter->addWidget(m_log);

  // 设置分割器初始拉伸比例
  splitter->setStretchFactor(0, 5); // 监控区域占5份
  splitter->setStretchFactor(1, 1); // 日志区域占1份

  mainLayout->addWidget(splitter, 1);

  // ---- 信号连接 ----
  connect(m_btnConfig, &QPushButton::clicked, this,
          &DeviceMonitorPanel::onConfigClicked);
  connect(m_btnImport, &QPushButton::clicked, this,
          &DeviceMonitorPanel::onImportClicked);
  connect(m_btnExport, &QPushButton::clicked, this,
          &DeviceMonitorPanel::onExportClicked);
  connect(m_btnReset, &QPushButton::clicked, this,
          &DeviceMonitorPanel::onResetClicked);

  // 初始占位
  rebuildGrid();
}

// ========== CAN 帧处理 ==========

void DeviceMonitorPanel::onFrameReceived(const CanFrame &frame) {
  if (m_mappings.isEmpty()) return;
  m_frameCount++;

  for (const auto &mapping : m_mappings) {
    if (frame.id != mapping.canId) continue;
    if (mapping.byteIndex >= frame.data.size()) continue;

    const quint8 byteVal =
        static_cast<quint8>(frame.data[mapping.byteIndex]);
    const bool bitVal = (byteVal >> mapping.bitIndex) & 0x01;

    auto *widget = m_deviceWidgets.value(mapping.deviceId, nullptr);
    if (!widget) continue;

    const bool prev = widget->status();
    widget->setStatus(bitVal);

    if (bitVal != prev) {
      QString dir = bitVal ? QStringLiteral(">> 报警") : QStringLiteral("<< 恢复");
      if (mapping.deviceType == QStringLiteral("valve")) {
        dir = bitVal ? QStringLiteral(">> 开启") : QStringLiteral("<< 关闭");
      }
      appendLog(QStringLiteral("%1  ID=0x%2 B%3.b%4  %5")
                    .arg(mapping.label)
                    .arg(frame.id, 3, 16, QChar('0'))
                    .arg(mapping.byteIndex)
                    .arg(mapping.bitIndex)
                    .arg(dir),
                bitVal);

      // 向所有连接的 WebSocket 客户端广播状态更新
      QJsonObject updateObj;
      updateObj[QStringLiteral("type")] = QStringLiteral("update");
      updateObj[QStringLiteral("deviceId")] = mapping.deviceId;
      updateObj[QStringLiteral("status")] = bitVal;
      updateObj[QStringLiteral("prevStatus")] = prev;
      updateObj[QStringLiteral("label")] = mapping.label;
      updateObj[QStringLiteral("deviceType")] = mapping.deviceType;
      updateObj[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
      broadcastMessage(updateObj);
    }
  }
}

// ========== 网格重建 ==========

void DeviceMonitorPanel::rebuildGrid() {
  if (m_mappings.isEmpty()) {
    qDeleteAll(m_deviceWidgets);
    m_deviceWidgets.clear();
    while (m_gridLayout->count() > 0) {
      QLayoutItem *item = m_gridLayout->takeAt(0);
      delete item;
    }
    auto *ph = new QLabel(
        QStringLiteral("  ⚊  暂无设备配置  ⚊\n\n"
                       "点击「⚙ 设备配置」添加 CAN 设备位映射\n"
                       "收到匹配的 CAN 帧后将自动更新设备状态"));
    ph->setAlignment(Qt::AlignCenter);
    ph->setStyleSheet(
        "color: #374151; font-size: 15px; padding: 80px; "
        "background: transparent; "
        "font-family: 'Microsoft YaHei';");
    ph->setMinimumHeight(300);
    m_gridLayout->addWidget(ph, 0, 0, 1, m_gridCols, Qt::AlignCenter);
  } else {
    // 如果配置数量发生变化，则重新创建控件
    if (m_deviceWidgets.size() != m_mappings.size()) {
      qDeleteAll(m_deviceWidgets);
      m_deviceWidgets.clear();
      while (m_gridLayout->count() > 0) {
        QLayoutItem *item = m_gridLayout->takeAt(0);
        delete item;
      }
      for (int i = 0; i < m_mappings.size(); ++i) {
        const auto &m = m_mappings[i];
        DeviceStatusWidget::DeviceKind kind = DeviceStatusWidget::Detector;
        if (m.deviceType == QStringLiteral("valve")) {
          kind = DeviceStatusWidget::Valve;
        } else if (m.deviceType == QStringLiteral("manual_alarm")) {
          kind = DeviceStatusWidget::ManualAlarm;
        } else if (m.deviceType == QStringLiteral("gas_cylinder")) {
          kind = DeviceStatusWidget::GasCylinder;
        } else if (m.deviceType == QStringLiteral("water_pump")) {
          kind = DeviceStatusWidget::WaterPump;
        } else if (m.deviceType == QStringLiteral("pressure_switch")) {
          kind = DeviceStatusWidget::PressureSwitch;
        } else if (m.deviceType == QStringLiteral("mobile_spray_gun")) {
          kind = DeviceStatusWidget::MobileSprayGun;
        }
        auto *w = new DeviceStatusWidget(m.deviceId, kind, m.label, m.canId);
        w->setStatus(m.defaultVal);
        m_deviceWidgets[m.deviceId] = w;
      }
    }

    // 重新排列已有控件坐标（避免重新创建导致的状态丢失）
    while (m_gridLayout->count() > 0) {
      m_gridLayout->takeAt(0);
    }

    for (int i = 0; i < m_mappings.size(); ++i) {
      const auto &m = m_mappings[i];
      auto *w = m_deviceWidgets.value(m.deviceId, nullptr);
      if (w) {
        int row = i / m_gridCols;
        int col = i % m_gridCols;
        m_gridLayout->addWidget(w, row, col);
      }
    }
  }

  // 广播配置更新给所有已连接的 WebSocket 客户端
  for (auto *client : m_clients) {
    sendConfigToClient(client);
  }
}

void DeviceMonitorPanel::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  if (!m_scrollArea || m_mappings.isEmpty()) return;

  // 根据视口宽度动态计算列数 (卡片宽度约150px)
  int areaW = m_scrollArea->viewport()->width();
  int cols = qMax(1, areaW / 150);
  if (cols != m_gridCols) {
    m_gridCols = cols;
    rebuildGrid();
  }
}

void DeviceMonitorPanel::sendConfigToClient(QWebSocket *client) {
  QJsonObject configObj;
  configObj[QStringLiteral("type")] = QStringLiteral("config");

  QJsonArray mappingsArr;
  for (const auto &mapping : m_mappings) {
    QJsonObject item;
    item[QStringLiteral("deviceId")] = mapping.deviceId;
    item[QStringLiteral("label")] = mapping.label;
    item[QStringLiteral("deviceType")] = mapping.deviceType;
    item[QStringLiteral("canId")] = static_cast<int>(mapping.canId);
    item[QStringLiteral("byteIndex")] = mapping.byteIndex;
    item[QStringLiteral("bitIndex")] = mapping.bitIndex;

    // 获取当前状态
    bool currentStatus = false;
    auto *widget = m_deviceWidgets.value(mapping.deviceId, nullptr);
    if (widget) {
      currentStatus = widget->status();
    }
    item[QStringLiteral("status")] = currentStatus;

    mappingsArr.append(item);
  }
  configObj[QStringLiteral("mappings")] = mappingsArr;

  QJsonDocument doc(configObj);
  client->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void DeviceMonitorPanel::broadcastMessage(const QJsonObject &json) {
  QJsonDocument doc(json);
  QString msg = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
  for (auto *client : m_clients) {
    client->sendTextMessage(msg);
  }
}

void DeviceMonitorPanel::onNewConnection() {
  QWebSocket *client = m_wsServer->nextPendingConnection();
  connect(client, &QWebSocket::disconnected, this, &DeviceMonitorPanel::onClientDisconnected);
  m_clients.append(client);

  appendLog(QStringLiteral("[WebSocket] 客户端已连接：%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort()), false);

  // 连接建立后，主动发送当前配置和最新设备状态
  sendConfigToClient(client);
}

void DeviceMonitorPanel::onClientDisconnected() {
  QWebSocket *client = qobject_cast<QWebSocket *>(sender());
  if (client) {
    m_clients.removeAll(client);
    client->deleteLater();
    appendLog(QStringLiteral("[WebSocket] 客户端已断开"), false);
  }
}

// ========== 按钮操作 ==========

void DeviceMonitorPanel::onConfigClicked() {
  CanProtocolConfigDialog dlg(this);

  // 获取 CANTool 窗口的当前样式以应用主题
  QWidget *canTool = this;
  while (canTool && !canTool->inherits("CANTool")) {
    canTool = canTool->parentWidget();
  }
  if (canTool) {
    dlg.applyThemeStyle(canTool->styleSheet());
  }

  dlg.setMappings(m_mappings);
  if (dlg.exec() == QDialog::Accepted) {
    m_mappings = dlg.mappings();
    rebuildGrid();
    saveConfig();
    appendLog(
        QStringLiteral("✓ 配置已更新 — %1 个设备").arg(m_mappings.size()));
  }
}

void DeviceMonitorPanel::onImportClicked() {
  QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("导入配置"), QString(),
      QStringLiteral("JSON (*.json)"));
  if (path.isEmpty()) return;

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) return;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  f.close();

  if (!doc.isArray()) return;
  m_mappings.clear();
  for (const auto &v : doc.array()) {
    auto o = v.toObject();
    DeviceBitMapping m;
    m.deviceId = o["deviceId"].toInt(1);
    m.label = o["label"].toString(QStringLiteral("设备"));
    m.deviceType =
        o["deviceType"].toString(QStringLiteral("detector"));
    m.canId = static_cast<quint32>(o["canId"].toInt(0x100));
    m.byteIndex = o["byteIndex"].toInt(0);
    m.bitIndex = o["bitIndex"].toInt(0);
    m.defaultVal = o["defaultVal"].toInt(0);
    m_mappings.append(m);
  }
  rebuildGrid();
  saveConfig();
  appendLog(QStringLiteral("✓ 已导入 %1 个设备").arg(m_mappings.size()));
}

void DeviceMonitorPanel::onExportClicked() {
  QJsonArray arr;
  for (const auto &m : m_mappings) {
    QJsonObject o;
    o["deviceId"] = m.deviceId;
    o["label"] = m.label;
    o["deviceType"] = m.deviceType;
    o["canId"] = static_cast<int>(m.canId);
    o["byteIndex"] = m.byteIndex;
    o["bitIndex"] = m.bitIndex;
    o["defaultVal"] = m.defaultVal;
    arr.append(o);
  }
  QString path = QFileDialog::getSaveFileName(
      this, QStringLiteral("导出"), QStringLiteral("device_config.json"),
      QStringLiteral("JSON (*.json)"));
  if (path.isEmpty()) return;
  QFile f(path);
  if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
  }
}

void DeviceMonitorPanel::onResetClicked() {
  for (const auto &m : m_mappings) {
    auto *w = m_deviceWidgets.value(m.deviceId, nullptr);
    if (w) w->setStatus(m.defaultVal);
  }
  m_log->clear();
  m_frameCount = 0;
  appendLog(QStringLiteral("↻ 全部设备状态已重置为默认配置值"));
}

// ========== 持久化 ==========

void DeviceMonitorPanel::saveConfig() {
  QJsonArray arr;
  for (const auto &m : m_mappings) {
    QJsonObject o;
    o["deviceId"] = m.deviceId;
    o["label"] = m.label;
    o["deviceType"] = m.deviceType;
    o["canId"] = static_cast<int>(m.canId);
    o["byteIndex"] = m.byteIndex;
    o["bitIndex"] = m.bitIndex;
    o["defaultVal"] = m.defaultVal;
    arr.append(o);
  }
  QFile f(QStringLiteral("device_config.json"));
  if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
  }
}

void DeviceMonitorPanel::loadConfig() {
  QFile f(QStringLiteral("device_config.json"));
  if (!f.open(QIODevice::ReadOnly)) return;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  f.close();
  if (!doc.isArray()) return;
  for (const auto &v : doc.array()) {
    auto o = v.toObject();
    DeviceBitMapping m;
    m.deviceId = o["deviceId"].toInt(1);
    m.label = o["label"].toString(QStringLiteral("设备"));
    m.deviceType =
        o["deviceType"].toString(QStringLiteral("detector"));
    m.canId = static_cast<quint32>(o["canId"].toInt(0x100));
    m.byteIndex = o["byteIndex"].toInt(0);
    m.bitIndex = o["bitIndex"].toInt(0);
    m.defaultVal = o["defaultVal"].toInt(0);
    m_mappings.append(m);
  }
  rebuildGrid();
}

// ========== 日志 ==========

void DeviceMonitorPanel::appendLog(const QString &text, bool isAlarm) {
  const QString ts =
      QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  if (isAlarm) {
    m_log->appendHtml(
        QStringLiteral("<span style='color:#EF4444;'>[%1] %2</span>")
            .arg(ts, text.toHtmlEscaped()));
  } else {
    m_log->appendPlainText(
        QStringLiteral("[%1] %2").arg(ts, text));
  }
  // 裁剪旧日志
  auto *doc = m_log->document();
  while (doc->blockCount() > 300) {
    QTextCursor c(doc->firstBlock());
    c.select(QTextCursor::BlockUnderCursor);
    c.removeSelectedText();
    c.deleteChar();
  }
  m_log->verticalScrollBar()->setValue(
      m_log->verticalScrollBar()->maximum());
}
