#include "serialportplot.h"
#include <QDateTime>
#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QMessageBox>
#include <QScrollBar>
#include <QSize>
#include <QSplitter>
#include <QVBoxLayout>

SerialPortPlot::SerialPortPlot(QWidget *parent)
    : QWidget(parent), m_rxCount(0), m_txCount(0), m_xValue(0),
      m_lastPortCount(0), m_scrollPos(0) {
  m_serial = new QSerialPort(this);
  m_autoSendTimer = new QTimer(this);
  m_portCheckTimer = new QTimer(this);
  m_scrollTimer = new QTimer(this);

  m_welcomeText = "   欢迎使用uSmilePro串口示波器V1.0   ";

  setupUi();
  setupChart();
  setupConnections();
  refreshPorts();

  // Start timers
  m_portCheckTimer->start(1000); // Check every second
  m_scrollTimer->start(200);     // Scroll speed
}

SerialPortPlot::~SerialPortPlot() {
  if (m_serial->isOpen())
    m_serial->close();
}

void SerialPortPlot::setupUi() {
  QVBoxLayout *mainLayout =
      new QVBoxLayout(this); // Changed to Vertical for Status Bar
  QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

  // Connect splitter to mainLayout later, but first let's keep the structure
  // The original code did mainLayout->addWidget(splitter). We will do that too.
  // splitter->setStyleSheet("QSplitter::handle { background-color: #cccccc;
  // }");
  // --- Left Panel: Settings ---
  QWidget *leftPanel =
      new QWidget(); // 创建左面板，当前没有指定其父，后面通过
                     // splitter->addWidget(leftPanel);确定QSplitter为其父
  QVBoxLayout *leftLayout = new QVBoxLayout(
      leftPanel); // 垂直布局绑定leftPanel，确定垂直布局的父是leftPanel；

  // 给左侧布局设内边距（面板边缘到控件的距离）
  leftLayout->setContentsMargins(10, 10, 10, 10); // 上下左右各10像素
  // 设置控件之间的间距
  leftLayout->setSpacing(8); // 控件之间隔8像素

  // 给左侧面板加浅灰色背景，和右侧绘图区域区分开
  // Global Stylesheet for GroupBoxes and Background
  this->setStyleSheet("SerialPortPlot { background-color: #f5f5f5; }"
                      "QGroupBox { "
                      "    border: 1px solid #BDBDBD; "
                      "    border-radius: 10px; "
                      "    margin-top: 10px; "
                      "    font-weight: bold; "
                      "}"
                      "QGroupBox::title { "
                      "    subcontrol-origin: margin; "
                      "    subcontrol-position: top left; "
                      "    padding: 0 5px; "
                      "    left: 10px; "
                      "}"
                      "QTextEdit { "
                      "    border: 1px solid #4CAF50; "
                      "    border-radius: 10px; "
                      "    padding: 5px; "
                      "    background-color: #FFFFFF; "
                      "}");

  // 1. Port Settings
  QGroupBox *grpPort = new QGroupBox(
      "串口设置"); // 垂直布局的子对象grpPort（串口设置分组框）
                   // 当前没有指定其父，后面通过leftLayout->addWidget(grpPort);确定其父为leftLayout
  QGridLayout *portLayout = new QGridLayout(
      grpPort); // 串口网格布局，布局绑定grpPort，确定网格布局的父是grpPort

  portLayout->addWidget(new QLabel("端口号:"), 0, 0);
  m_comboPort = new QComboBox();
  portLayout->addWidget(m_comboPort, 0, 1, 1, 2);

  portLayout->addWidget(new QLabel("波特率:"), 1, 0);
  m_comboBaud = new QComboBox();
  m_comboBaud->addItems(
      {"9600", "19200", "38400", "57600", "115200", "921600"});
  m_comboBaud->setCurrentText("115200");
  portLayout->addWidget(m_comboBaud, 1, 1, 1, 2);

  portLayout->addWidget(new QLabel("数据位:"), 2, 0);
  m_comboDataBits = new QComboBox();
  m_comboDataBits->addItems({"5", "6", "7", "8"});
  m_comboDataBits->setCurrentText("8");
  portLayout->addWidget(m_comboDataBits, 2, 1, 1, 2);

  portLayout->addWidget(new QLabel("校验位:"), 3, 0);
  m_comboParity = new QComboBox();
  m_comboParity->addItem("None", QSerialPort::NoParity);
  m_comboParity->addItem("Odd", QSerialPort::OddParity);
  m_comboParity->addItem("Even", QSerialPort::EvenParity);
  portLayout->addWidget(m_comboParity, 3, 1, 1, 2);

  portLayout->addWidget(new QLabel("停止位:"), 4, 0);
  m_comboStopBits = new QComboBox();
  m_comboStopBits->addItem("1", QSerialPort::OneStop);
  m_comboStopBits->addItem("1.5", QSerialPort::OneAndHalfStop);
  m_comboStopBits->addItem("2", QSerialPort::TwoStop);
  portLayout->addWidget(m_comboStopBits, 4, 1, 1, 2);

  QHBoxLayout *portActionLayout = new QHBoxLayout();

  m_btnRefresh = new QPushButton("刷新");
  m_btnRefresh->setMinimumWidth(80);
  m_btnRefresh->setMinimumHeight(40);
  m_btnRefresh->setStyleSheet(getButtonStyle(ButtonType::Refresh));
  portActionLayout->addWidget(m_btnRefresh);

  m_btnOpenClose = new QPushButton("打开串口");
  m_btnOpenClose->setCheckable(true);

  // Make button smaller (compact)
  m_btnOpenClose->setMinimumWidth(80);
  m_btnOpenClose->setMinimumHeight(40);
  m_btnOpenClose->setStyleSheet(getButtonStyle(ButtonType::Open));

  // Status Icon Label
  m_lblStatusIcon = new QLabel();
  m_lblStatusIcon->setPixmap(
      QPixmap(":/icons/ONOFF/OFF5.png")
          .scaled(100, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  m_lblStatusIcon->setAlignment(Qt::AlignCenter);

  portActionLayout->addStretch();
  portActionLayout->addWidget(m_lblStatusIcon);
  portActionLayout->addStretch();
  portActionLayout->addWidget(m_btnOpenClose);
  portActionLayout->addStretch();

  // Add sub-layout to main grid at row 5, spanning 3 cols
  portLayout->addLayout(portActionLayout, 5, 0, 1, 3);

  leftLayout->addWidget(grpPort);

  // 2. Receive Settings
  QGroupBox *grpRx = new QGroupBox("接收设置");
  QVBoxLayout *rxLayout = new QVBoxLayout(grpRx);

  QHBoxLayout *rxModeLayout = new QHBoxLayout();
  m_rbRxAscii = new QRadioButton("ASCII");
  m_rbRxHex = new QRadioButton("HEX");
  m_rbRxAscii->setChecked(true);
  rxModeLayout->addWidget(m_rbRxAscii);
  rxModeLayout->addWidget(m_rbRxHex);
  rxLayout->addLayout(rxModeLayout);

  m_chkRxLog = new QCheckBox("日志模式");
  m_chkRxTime = new QCheckBox("显示时间");
  m_chkRxNewLine = new QCheckBox(
      "自动换行"); // Actually TextEdit does this, maybe "Add Newline"? Let's
                   // assume user means data interpretation or auto-scrolling

  rxLayout->addWidget(m_chkRxLog);
  rxLayout->addWidget(m_chkRxTime);
  // rxLayout->addWidget(m_chkRxNewLine);

  QHBoxLayout *rxBtnLayout = new QHBoxLayout();
  m_btnClearRx = new QPushButton("清空");
  m_btnClearRx->setStyleSheet(getButtonStyle(ButtonType::Normal));
  m_btnStopRx = new QPushButton("暂停显示");
  m_btnStopRx->setStyleSheet(getButtonStyle(ButtonType::Normal));
  m_btnStopRx->setCheckable(true);
  rxBtnLayout->addWidget(m_btnClearRx);
  rxBtnLayout->addWidget(m_btnStopRx);
  rxLayout->addLayout(rxBtnLayout);

  // Waveform Toggle
  m_chkEnableWaveform = new QCheckBox("启用波形显示");
  rxLayout->addWidget(m_chkEnableWaveform);

  // 4. Waveform Settings (Initially Hidden)
  m_grpWaveformSettings = new QGroupBox("波形设置");
  QVBoxLayout *waveLayout = new QVBoxLayout(m_grpWaveformSettings);

  QHBoxLayout *ptLayout = new QHBoxLayout();
  ptLayout->addWidget(new QLabel("点数:"));
  m_spinPoints = new QSpinBox();
  m_spinPoints->setRange(10, 10000);
  m_spinPoints->setValue(100);
  m_spinPoints->setSingleStep(10);
  ptLayout->addWidget(m_spinPoints);
  waveLayout->addLayout(ptLayout);

  m_chkAutoY = new QCheckBox("Y轴自动缩放");
  m_chkAutoY->setChecked(true); // Default to auto
  waveLayout->addWidget(m_chkAutoY);

  QGridLayout *rangeLayout = new QGridLayout();
  rangeLayout->addWidget(new QLabel("Min:"), 0, 0);
  m_spinYMin = new QDoubleSpinBox();
  m_spinYMin->setRange(-99999, 99999);
  m_spinYMin->setValue(0);
  rangeLayout->addWidget(m_spinYMin, 0, 1);

  rangeLayout->addWidget(new QLabel("Max:"), 1, 0);
  m_spinYMax = new QDoubleSpinBox();
  m_spinYMax->setRange(-99999, 99999);
  m_spinYMax->setValue(255);
  rangeLayout->addWidget(m_spinYMax, 1, 1);
  waveLayout->addLayout(rangeLayout);

  m_btnResetChart = new QPushButton("重置图表");
  m_btnResetChart->setStyleSheet(getButtonStyle(ButtonType::Normal));
  waveLayout->addWidget(m_btnResetChart);

  leftLayout->addWidget(m_grpWaveformSettings);
  m_grpWaveformSettings->setVisible(false); // Hidden by default

  leftLayout->addWidget(grpRx);

  // 3. Send Settings
  QGroupBox *grpTx = new QGroupBox("发送设置");
  QVBoxLayout *txLayout = new QVBoxLayout(grpTx);

  QHBoxLayout *txModeLayout = new QHBoxLayout();
  m_rbTxAscii = new QRadioButton("ASCII");
  m_rbTxHex = new QRadioButton("HEX");
  m_rbTxAscii->setChecked(true);
  txModeLayout->addWidget(m_rbTxAscii);
  txModeLayout->addWidget(m_rbTxHex);
  txLayout->addLayout(txModeLayout);

  m_chkTxNewLine = new QCheckBox("发送新行");
  m_chkTxTime = new QCheckBox("显示时间"); // Timestamp in local log
  txLayout->addWidget(m_chkTxNewLine);
  txLayout->addWidget(m_chkTxTime);

  m_chkAutoSend = new QCheckBox("自动发送(ms):");
  m_spinAutoSendInterval = new QSpinBox();
  m_spinAutoSendInterval->setRange(10, 10000);
  m_spinAutoSendInterval->setValue(1000);

  QHBoxLayout *autoSendLayout = new QHBoxLayout();
  autoSendLayout->addWidget(m_chkAutoSend);
  autoSendLayout->addWidget(m_spinAutoSendInterval);
  txLayout->addLayout(autoSendLayout);

  leftLayout->addWidget(grpTx);

  // Status (Counts)
  QGroupBox *grpStatus = new QGroupBox("统计");
  QGridLayout *statusLayout = new QGridLayout(grpStatus);
  statusLayout->addWidget(new QLabel("RX:"), 0, 0);
  m_lblRxCount = new QLabel("0");
  statusLayout->addWidget(m_lblRxCount, 0, 1);
  statusLayout->addWidget(new QLabel("TX:"), 1, 0);
  m_lblTxCount = new QLabel("0");
  statusLayout->addWidget(m_lblTxCount, 1, 1);
  leftLayout->addWidget(grpStatus);

  leftLayout->addStretch();

  splitter->addWidget(leftPanel);

  // --- Right Panel: Data and Waveform ---
  QSplitter *rightSplitter = new QSplitter(Qt::Vertical);

  // Receive Area
  QGroupBox *grpData = new QGroupBox("数据接收");
  QVBoxLayout *dataLayout = new QVBoxLayout(grpData);
  m_textReceive = new QTextEdit();
  m_textReceive->setReadOnly(true);
  dataLayout->addWidget(m_textReceive);
  rightSplitter->addWidget(grpData);

  // Waveform Area
  m_chartView = new QChartView();
  rightSplitter->addWidget(m_chartView);
  // Initially hide chart or just let splitter accept it.
  // Let's keep it visible but maybe collapsed or let user adjust.

  // Send Area
  QGroupBox *grpSend = new QGroupBox("数据发送");
  QVBoxLayout *sendLayout = new QVBoxLayout(grpSend);

  // Text Input
  m_textSend = new QTextEdit();
  m_textSend->setMaximumHeight(60);
  sendLayout->addWidget(m_textSend);

  // Buttons Row
  QHBoxLayout *btnLayout = new QHBoxLayout();
  m_btnSend = new QPushButton("发送");
  m_btnSend->setStyleSheet(getButtonStyle(ButtonType::Normal));
  m_btnSend->setMinimumHeight(40);

  m_btnClearSend = new QPushButton("清除发送");
  m_btnClearSend->setStyleSheet(getButtonStyle(ButtonType::Normal));
  m_btnClearSend->setMinimumHeight(40);

  btnLayout->addWidget(m_btnClearSend);
  btnLayout->addWidget(m_btnSend);
  sendLayout->addLayout(btnLayout);

  // History Row (Moved to bottom)
  QHBoxLayout *histLayout = new QHBoxLayout();
  histLayout->addWidget(new QLabel("发送历史:"));
  m_comboHistory = new QComboBox();
  m_comboHistory->setEditable(false); // Only selectable, input via text box
  histLayout->addWidget(m_comboHistory);
  sendLayout->addLayout(histLayout);

  // Combine Right Side
  QWidget *rightPanel = new QWidget();
  QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
  rightLayout->addWidget(rightSplitter);
  rightLayout->addWidget(grpSend);

  splitter->addWidget(rightPanel);
  splitter->setStretchFactor(1, 1); // Give more space to right side

  mainLayout->addWidget(splitter);

  // --- Status Bar ---
  QHBoxLayout *statusBarLayout = new QHBoxLayout();
  statusBarLayout->setContentsMargins(5, 2, 5, 2);

  // Status Icon/Text
  m_lblPortInfo = new QLabel("串口关闭");
  m_lblPortInfo->setStyleSheet("color: red; font-weight: bold;");

  // Scrolling Message
  m_lblWelcome = new QLabel();
  m_lblWelcome->setStyleSheet("color: blue; font-style: italic;");
  m_lblWelcome->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  statusBarLayout->addWidget(m_lblPortInfo);
  statusBarLayout->addStretch();
  statusBarLayout->addWidget(m_lblWelcome);

  mainLayout->addLayout(statusBarLayout);
}

void SerialPortPlot::setupChart() {
  QChart *chart = new QChart();
  chart->setTitle("串口数据波形");
  chart->legend()->hide();

  m_series = new QLineSeries();
  chart->addSeries(m_series);

  m_axisX = new QValueAxis();
  m_axisX->setLabelFormat("%d");
  m_axisX->setTitleText("Time");
  chart->addAxis(m_axisX, Qt::AlignBottom);
  m_series->attachAxis(m_axisX);

  m_axisY = new QValueAxis();
  m_axisY->setTitleText("Value");
  chart->addAxis(m_axisY, Qt::AlignLeft);
  m_series->attachAxis(m_axisY);

  m_chartView->setChart(chart);
  m_chartView->setRenderHint(QPainter::Antialiasing);

  // Initial Range
  m_axisX->setRange(0, 100);
  m_axisY->setRange(0, 255);

  m_chartView->setVisible(false); // Hidden by default
}

void SerialPortPlot::setupConnections() {
  connect(m_btnRefresh, &QPushButton::clicked, this,
          &SerialPortPlot::refreshPorts);
  connect(m_btnOpenClose, &QPushButton::clicked, this,
          &SerialPortPlot::openClosePort);
  connect(m_serial, &QSerialPort::readyRead, this,
          &SerialPortPlot::onReadyRead);
  connect(m_serial, &QSerialPort::errorOccurred, this,
          &SerialPortPlot::onPortError);

  connect(m_btnClearRx, &QPushButton::clicked, this,
          &SerialPortPlot::clearReceiveArea);
  connect(m_btnSend, &QPushButton::clicked, this, &SerialPortPlot::sendData);

  // Send Area Connections
  connect(m_btnClearSend, &QPushButton::clicked,
          [this]() { m_textSend->clear(); });
  connect(m_comboHistory, QOverload<int>::of(&QComboBox::activated),
          [this](int index) {
            if (index >= 0)
              m_textSend->setText(m_comboHistory->itemText(index));
          });

  connect(m_chkAutoSend, &QCheckBox::toggled, this,
          &SerialPortPlot::toggleAutoSend);
  connect(m_autoSendTimer, &QTimer::timeout, this,
          &SerialPortPlot::onAutoSendTimeout);

  connect(m_chkEnableWaveform, &QCheckBox::toggled, this,
          &SerialPortPlot::onWaveformEnabled);

  connect(m_spinPoints, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &SerialPortPlot::updateChartSettings);
  connect(m_chkAutoY, &QCheckBox::toggled, this,
          &SerialPortPlot::updateChartSettings);
  connect(m_spinYMin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &SerialPortPlot::updateChartSettings);
  connect(m_portCheckTimer, &QTimer::timeout, this,
          &SerialPortPlot::checkPorts);
  connect(m_scrollTimer, &QTimer::timeout, this,
          &SerialPortPlot::scrollWelcomeMessage);

  connect(m_spinYMax, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &SerialPortPlot::updateChartSettings);
  connect(m_btnResetChart, &QPushButton::clicked, [this]() {
    m_series->clear();
    m_xValue = 0;
    updateChartSettings();
  });
}

void SerialPortPlot::refreshPorts() {
  QString currentPort = m_comboPort->currentData().toString();
  m_comboPort->clear();
  const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos) {
    m_comboPort->addItem(info.portName() + " (" + info.description() + ")",
                         info.portName());
  }

  // Restore selection if possible
  int idx = m_comboPort->findData(currentPort);
  if (idx >= 0) {
    m_comboPort->setCurrentIndex(idx);
  }
}

void SerialPortPlot::checkPorts() {
  const auto infos = QSerialPortInfo::availablePorts();
  if (infos.count() != m_lastPortCount) {
    m_lastPortCount = infos.count();
    refreshPorts();
  } else {
    // Double check simple count mismatch might miss same count different ports
    // But for simple usage count check is often enough, or check list names
    bool changed = false;
    if (m_comboPort->count() != infos.count()) {
      changed = true;
    } else {
      for (int i = 0; i < infos.size(); i++) {
        if (m_comboPort->itemData(i).toString() != infos.at(i).portName()) {
          changed = true;
          break;
        }
      }
    }

    if (changed) {
      refreshPorts();
    }
  }
}

void SerialPortPlot::openClosePort() {
  if (m_serial->isOpen()) {
    m_serial->close();
    m_serial->close();
    m_serial->close();
    m_btnOpenClose->setText("打开串口");
    m_btnOpenClose->setChecked(false);
    m_lblStatusIcon->setPixmap(
        QPixmap(":/icons/ONOFF/OFF5.png")
            .scaled(100, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    updateStatusInfo();
    // Enable settings
    m_comboPort->setEnabled(true);
    m_comboBaud->setEnabled(true);
    m_comboDataBits->setEnabled(true);
    m_comboParity->setEnabled(true);
    m_comboStopBits->setEnabled(true);
  } else {
    m_serial->setPortName(m_comboPort->currentData().toString());
    m_serial->setBaudRate(m_comboBaud->currentText().toInt());
    m_serial->setDataBits(static_cast<QSerialPort::DataBits>(
        m_comboDataBits->currentText().toInt()));
    m_serial->setParity(
        static_cast<QSerialPort::Parity>(m_comboParity->currentData().toInt()));
    m_serial->setStopBits(static_cast<QSerialPort::StopBits>(
        m_comboStopBits->currentData().toInt()));

    if (m_serial->open(QIODevice::ReadWrite)) {
      m_btnOpenClose->setText("关闭串口");
      m_btnOpenClose->setChecked(true);
      m_lblStatusIcon->setPixmap(
          QPixmap(":/icons/ONOFF/ON2.png")
              .scaled(100, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      updateStatusInfo();
      // Disable settings
      m_comboPort->setEnabled(false);
      m_comboBaud->setEnabled(false);
      m_comboDataBits->setEnabled(false);
      m_comboParity->setEnabled(false);
      m_comboStopBits->setEnabled(false);
    } else {
      QMessageBox::critical(this, "错误",
                            "无法打开串口:\n" + m_serial->errorString());
      m_btnOpenClose->setChecked(false);
    }
  }
}

void SerialPortPlot::onPortError(QSerialPort::SerialPortError error) {
  if (error == QSerialPort::ResourceError) {
    QMessageBox::critical(this, "严重错误", "串口连接中断！");
    openClosePort(); // Force close UI state
  }
}

void SerialPortPlot::onReadyRead() {
  QByteArray data = m_serial->readAll();
  m_rxCount += data.size();
  m_lblRxCount->setText(QString::number(m_rxCount));

  if (!m_btnStopRx->isChecked()) {
    QString displayStr;

    // Hex vs ASCII
    if (m_rbRxHex->isChecked()) {
      displayStr = data.toHex(' ').toUpper();
    } else {
      displayStr = QString::fromLocal8Bit(data); // Support Local encoding
    }

    if (m_chkRxTime->isChecked()) {
      QString timeStr =
          QDateTime::currentDateTime().toString("[HH:mm:ss.zzz] ");
      if (m_chkRxLog->isChecked()) {
        m_textReceive->append(timeStr + displayStr);
      } else {
        m_textReceive->moveCursor(QTextCursor::End);
        m_textReceive->insertPlainText(timeStr + displayStr);
      }
    } else {
      if (m_chkRxLog->isChecked()) {
        m_textReceive->append(displayStr);
      } else {
        m_textReceive->moveCursor(QTextCursor::End);
        m_textReceive->insertPlainText(displayStr);
      }
    }

    // Auto Scroll
    m_textReceive->verticalScrollBar()->setValue(
        m_textReceive->verticalScrollBar()->maximum());
  }

  if (m_chkEnableWaveform->isChecked()) {
    updateWaveform(data);
  }
}

void SerialPortPlot::onWaveformEnabled(bool checked) {
  m_chartView->setVisible(checked);
  m_grpWaveformSettings->setVisible(checked);
}

void SerialPortPlot::updateChartSettings() {
  // Points count is handled in updateWaveform (trimming)
  // Here we handle Y Axis
  if (m_chkAutoY->isChecked()) {
    m_spinYMin->setEnabled(false);
    m_spinYMax->setEnabled(false);
    // let updateWaveform handle auto-scaling based on data
  } else {
    m_spinYMin->setEnabled(true);
    m_spinYMax->setEnabled(true);
    m_axisY->setRange(m_spinYMin->value(), m_spinYMax->value());
  }
}

void SerialPortPlot::updateWaveform(const QByteArray &data) {
  // Simple parser: treat every number found as a Y value
  // This allows CSV or just '123\n124\n' to work
  QString str = QString::fromLocal8Bit(data);

  // Split by non-digit chars (roughly) or whitespace/comma
  // Improvement: use RegEx to find numbers
  static QString buffer; // Keep buffer for partial numbers across packets
  buffer.append(str);

  // Process full lines or delimiters?
  // Let's trying to find simple float/int patterns
  // We'll iterate and find numbers.

  int processedIndex = 0;
  bool inNumber = false;
  int startPos = 0;

  for (int i = 0; i < buffer.length(); ++i) {
    QChar c = buffer.at(i);
    bool isDigit = c.isDigit() || c == '.' || c == '-';

    if (isDigit && !inNumber) {
      inNumber = true;
      startPos = i;
    } else if (!isDigit && inNumber) {
      // End of number
      QString numStr = buffer.mid(startPos, i - startPos);
      bool ok;
      double val = numStr.toDouble(&ok);
      if (ok) {
        m_series->append(m_xValue++, val);
        int maxPoints = m_spinPoints->value();
        if (m_series->count() > maxPoints) {
          m_series->remove(0, m_series->count() - maxPoints);
        }

        // Auto Scale X
        if (m_series->count() > 0) {
          m_axisX->setRange(m_series->at(0).x(),
                            m_series->at(m_series->count() - 1).x());
        }

        // Auto Scale Y
        if (m_chkAutoY->isChecked() && m_series->count() > 0) {
          double minY = 1e9;
          double maxY = -1e9;
          for (const QPointF &p : m_series->points()) {
            if (p.y() < minY)
              minY = p.y();
            if (p.y() > maxY)
              maxY = p.y();
          }
          // Add some padding
          double padding = (maxY - minY) * 0.1;
          if (padding == 0)
            padding = 1.0;
          m_axisY->setRange(minY - padding, maxY + padding);
        }
      }
      inNumber = false;
    }

    if (!isDigit && !c.isSpace()) {
      // Delimiter reached, safe to say we processed up to here
      // Actually, if we are not in a number, we can discard.
    }
  }

  // Keep unprocessed tail
  if (inNumber) {
    buffer = buffer.mid(startPos);
  } else {
    buffer.clear();
  }
}

void SerialPortPlot::sendData() {
  if (!m_serial->isOpen())
    return;

  QString text = m_textSend->toPlainText();
  if (text.isEmpty())
    return;

  QByteArray idx;
  if (m_rbTxHex->isChecked()) {
    idx = QByteArray::fromHex(text.toUtf8());
  } else {
    idx = text.toLocal8Bit();
  }

  if (m_chkTxNewLine->isChecked()) {
    idx.append("\r\n");
  }

  m_serial->write(idx);
  m_txCount += idx.size();
  m_lblTxCount->setText(QString::number(m_txCount));

  // Add to History (avoid duplicates at top, simple Lru-like)
  if (m_comboHistory->findText(text) == -1) {
    m_comboHistory->insertItem(0, text);
    if (m_comboHistory->count() > 10) {
      m_comboHistory->removeItem(m_comboHistory->count() - 1);
    }
  } else {
    // If exists, move to top? Standard behavior varies.
    // Let's just ensure it's at 0 if we want 'recent' behavior
    int index = m_comboHistory->findText(text);
    m_comboHistory->removeItem(index);
    m_comboHistory->insertItem(0, text);
  }
  m_comboHistory->setCurrentIndex(0);
}

void SerialPortPlot::clearReceiveArea() {
  m_textReceive->clear();
  m_rxCount = 0;
  m_lblRxCount->setText("0");
  m_series->clear();
  m_xValue = 0;
}

void SerialPortPlot::toggleAutoSend(bool checked) {
  if (checked) {
    m_autoSendTimer->start(m_spinAutoSendInterval->value());
    m_spinAutoSendInterval->setEnabled(false);
  } else {
    m_autoSendTimer->stop();
    m_spinAutoSendInterval->setEnabled(true);
  }
}

void SerialPortPlot::onAutoSendTimeout() { sendData(); }

void SerialPortPlot::scrollWelcomeMessage() {
  if (m_welcomeText.isEmpty())
    return;

  // Simple marquee: move first char to end
  // m_scrollPos is not really needed if we just rotate the string
  // But let's keep original const string and rotate display

  QString display =
      m_welcomeText.mid(m_scrollPos) + m_welcomeText.left(m_scrollPos);
  m_lblWelcome->setText(display);

  m_scrollPos++;
  if (m_scrollPos >= m_welcomeText.length()) {
    m_scrollPos = 0;
  }
}

void SerialPortPlot::updateStatusInfo() {
  if (m_serial->isOpen()) {
    QString info =
        QString("串口[%1] 已打开 %2 %3-%4-%5")
            .arg(m_serial->portName())
            .arg(m_serial->baudRate())
            .arg(m_serial->dataBits())
            .arg(m_serial->parity() == QSerialPort::NoParity
                     ? "N"
                     : (m_serial->parity() == QSerialPort::OddParity ? "O"
                                                                     : "E"))
            .arg(m_serial->stopBits() == QSerialPort::OneStop
                     ? "1"
                     : (m_serial->stopBits() == QSerialPort::OneAndHalfStop
                            ? "1.5"
                            : "2"));
    m_lblPortInfo->setText(info);
    m_lblPortInfo->setStyleSheet("color: green; font-weight: bold;");
  }
  else
  {
      QString info =
          QString("串口[%1] 已关闭 %2 %3-%4-%5")
              .arg(m_serial->portName())
              .arg(m_serial->baudRate())
              .arg(m_serial->dataBits())
              .arg(m_serial->parity() == QSerialPort::NoParity
                       ? "N"
                       : (m_serial->parity() == QSerialPort::OddParity ? "O"
                                                                       : "E"))
              .arg(m_serial->stopBits() == QSerialPort::OneStop
                       ? "1"
                       : (m_serial->stopBits() == QSerialPort::OneAndHalfStop
                              ? "1.5"
                              : "2"));
      m_lblPortInfo->setText(info);
      m_lblPortInfo->setStyleSheet("color: red; font-weight: bold;");
  }
}

QString SerialPortPlot::getButtonStyle(ButtonType type) {
  // Base Style with shared settings
  QString baseStyle = "QPushButton { "
                      "    border-radius: 10px; "
                      "    border: 1px solid #90A4AE; "
                      "    padding: 5px; "
                      "    font-family: 'Microsoft YaHei UI'; "
                      "    color: black; "
                      "}"
                      "QPushButton:hover { "
                      "    font-weight: 900; " // Flashy text on hover
                      "    font-size: 10pt; "
                      "}";

  QString gradient;
  switch (type) {
  case ButtonType::Normal:
    // Glossy Blue-Gray
    gradient = "QPushButton { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #CFD8DC, stop:1 #B0BEC5); "
               "}"
               "QPushButton:hover { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #ECEFF1, stop:1 #CFD8DC); "
               "}"
               "QPushButton:pressed { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #B0BEC5, stop:1 #90A4AE); "
               "}";
    break;
  case ButtonType::Refresh:
    // Glossy Light Gray
    gradient = "QPushButton { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #E0E0E0, stop:1 #BDBDBD); "
               "}"
               "QPushButton:hover { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #F5F5F5, stop:1 #E0E0E0); "
               "}"
               "QPushButton:pressed { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #E0E0E0, stop:1 #9E9E9E); "
               "}";
    break;
  case ButtonType::Open:
    // Glossy Green / Red
    gradient = "QPushButton { " // Normal (Open - Green)
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #C8E6C9, stop:1 #81C784); "
               "}"
               "QPushButton:hover { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #E8F5E9, stop:1 #A5D6A7); "
               "}"
               "QPushButton:checked { " // Checked (Close - Red)
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #FFCDD2, stop:1 #E57373); "
               "}"
               "QPushButton:checked:hover { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #FFEBEE, stop:1 #EF9A9A); "
               "}";
    break;
  default:
    break;
  }

  return baseStyle + gradient;
}
