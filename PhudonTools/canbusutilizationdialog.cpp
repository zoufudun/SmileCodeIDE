#include "canbusutilizationdialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>

CanBusUtilizationDialog::CanBusUtilizationDialog(CanInterface *can, QWidget *parent)
    : QDialog(parent), m_can(can) {
  setupUi();
  
  m_timer = new QTimer(this);
  m_timer->setInterval(200); // 默认 200 ms
  connect(m_timer, &QTimer::timeout, this, &CanBusUtilizationDialog::onTimerTimeout);

  if (m_can) {
    connect(m_can, &CanInterface::frameReceived, this, &CanBusUtilizationDialog::onFrameReceived);
    connect(m_can, &CanInterface::frameSent, this, &CanBusUtilizationDialog::onFrameSent);
    connect(m_can, &CanInterface::connected, this, &CanBusUtilizationDialog::refreshChannels);
    connect(m_can, &CanInterface::disconnected, this, &CanBusUtilizationDialog::refreshChannels);
  }

  refreshChannels();

  m_elapsedTimer.start();
  m_timer->start();
}

CanBusUtilizationDialog::~CanBusUtilizationDialog() {
  if (m_timer) {
    m_timer->stop();
  }
}

void CanBusUtilizationDialog::setupUi() {
  setWindowTitle("CAN 总线利用率");
  setMinimumSize(850, 500);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(15, 15, 15, 15);
  mainLayout->setSpacing(15);

  QHBoxLayout *chartsLayout = new QHBoxLayout();
  chartsLayout->setSpacing(15);

  // --- Left Plot Panel ---
  QVBoxLayout *leftLayout = new QVBoxLayout();
  QHBoxLayout *leftHeader = new QHBoxLayout();
  leftHeader->addWidget(new QLabel("选择通道:"));
  m_leftChannelCombo = new QComboBox();
  leftHeader->addWidget(m_leftChannelCombo);
  leftHeader->addSpacing(10);
  m_leftRateLabel = new QLabel("当前速率: 0 帧/秒");
  leftHeader->addWidget(m_leftRateLabel);
  leftHeader->addSpacing(10);
  m_leftUsageLabel = new QLabel("当前利用率: 0.0000 %");
  leftHeader->addWidget(m_leftUsageLabel);
  leftHeader->addStretch();
  leftLayout->addLayout(leftHeader);

  m_leftPlot = new QCustomPlot(this);
  setupPlot(m_leftPlot);
  leftLayout->addWidget(m_leftPlot);
  chartsLayout->addLayout(leftLayout);

  // --- Right Plot Panel ---
  QVBoxLayout *rightLayout = new QVBoxLayout();
  QHBoxLayout *rightHeader = new QHBoxLayout();
  rightHeader->addWidget(new QLabel("选择通道:"));
  m_rightChannelCombo = new QComboBox();
  rightHeader->addWidget(m_rightChannelCombo);
  rightHeader->addSpacing(10);
  m_rightRateLabel = new QLabel("当前速率: 0 帧/秒");
  rightHeader->addWidget(m_rightRateLabel);
  rightHeader->addSpacing(10);
  m_rightUsageLabel = new QLabel("当前利用率: 0.0000 %");
  rightHeader->addWidget(m_rightUsageLabel);
  rightHeader->addStretch();
  rightLayout->addLayout(rightHeader);

  m_rightPlot = new QCustomPlot(this);
  setupPlot(m_rightPlot);
  rightLayout->addWidget(m_rightPlot);
  chartsLayout->addLayout(rightLayout);

  mainLayout->addLayout(chartsLayout);

  // --- Bottom settings row ---
  QHBoxLayout *bottomLayout = new QHBoxLayout();
  m_saveBtn = new QPushButton("实时保存");
  m_saveBtn->setStyleSheet("QPushButton { padding: 6px 20px; font-weight: bold; }");
  connect(m_saveBtn, &QPushButton::clicked, this, &CanBusUtilizationDialog::onSaveClicked);
  bottomLayout->addWidget(m_saveBtn);

  bottomLayout->addStretch();

  bottomLayout->addWidget(new QLabel("刷新时间:"));
  m_refreshCombo = new QComboBox();
  m_refreshCombo->addItems({"200 ms", "500 ms", "1000 ms"});
  connect(m_refreshCombo, &QComboBox::currentTextChanged, this, &CanBusUtilizationDialog::onRefreshPeriodChanged);
  bottomLayout->addWidget(m_refreshCombo);

  mainLayout->addLayout(bottomLayout);

  // Initialize data vectors
  for (int i = 0; i <= 100; ++i) {
    m_leftX.append(i);
    m_leftY.append(0.0);
    m_rightX.append(i);
    m_rightY.append(0.0);
  }
}

void CanBusUtilizationDialog::setupPlot(QCustomPlot *plot) {
  QColor baseColor = palette().color(QPalette::Base);
  QColor textColor = palette().color(QPalette::Text);
  QColor gridColor = palette().color(QPalette::Mid);

  plot->setBackground(QBrush(baseColor));
  plot->xAxis->setLabel("点数 (个)");
  plot->yAxis->setLabel("通道利用率 (%)");

  plot->xAxis->setLabelColor(textColor);
  plot->xAxis->setTickLabelColor(textColor);
  plot->xAxis->setBasePen(QPen(textColor));
  plot->xAxis->setTickPen(QPen(textColor));
  plot->xAxis->setSubTickPen(QPen(textColor));
  plot->xAxis->grid()->setPen(QPen(gridColor, 1, Qt::DotLine));

  plot->yAxis->setLabelColor(textColor);
  plot->yAxis->setTickLabelColor(textColor);
  plot->yAxis->setBasePen(QPen(textColor));
  plot->yAxis->setTickPen(QPen(textColor));
  plot->yAxis->setSubTickPen(QPen(textColor));
  plot->yAxis->grid()->setPen(QPen(gridColor, 1, Qt::DotLine));

  plot->xAxis->setRange(0, 100);
  plot->yAxis->setRange(0, 100);

  // Set line styling
  QCPGraph *graph = plot->addGraph();
  if (plot == m_leftPlot) {
    graph->setPen(QPen(QColor(97, 175, 239), 2)); // Modern light blue
  } else {
    graph->setPen(QPen(QColor(255, 87, 34), 2)); // Modern orange-red
  }
}

void CanBusUtilizationDialog::refreshChannels() {
  int prevLeft = m_leftChannelCombo->currentIndex();
  int prevRight = m_rightChannelCombo->currentIndex();

  m_leftChannelCombo->clear();
  m_rightChannelCombo->clear();

  QString hw, fw, dr, lib, serial, typeStr;
  int canNum = 2;
  if (m_can && m_can->isDeviceOpen()) {
    m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &canNum, &serial, &typeStr);
    if (typeStr.isEmpty()) {
      typeStr = "USBCANFD-200U";
    }
    int devIdx = m_can->deviceIndex();
    for (int i = 0; i < canNum; ++i) {
      QString label = QString("%1 设备%2 通道%3").arg(typeStr).arg(devIdx).arg(i);
      m_leftChannelCombo->addItem(label, i);
      m_rightChannelCombo->addItem(label, i);
    }
  } else {
    m_leftChannelCombo->addItem("未打开设备", -1);
    m_rightChannelCombo->addItem("未打开设备", -1);
  }

  if (prevLeft >= 0 && prevLeft < m_leftChannelCombo->count()) {
    m_leftChannelCombo->setCurrentIndex(prevLeft);
  } else if (m_leftChannelCombo->count() > 0) {
    m_leftChannelCombo->setCurrentIndex(0);
  }

  if (prevRight >= 0 && prevRight < m_rightChannelCombo->count()) {
    m_rightChannelCombo->setCurrentIndex(prevRight);
  } else if (m_rightChannelCombo->count() > 1) {
    m_rightChannelCombo->setCurrentIndex(1); // Default right to channel 1 if available
  } else if (m_rightChannelCombo->count() > 0) {
    m_rightChannelCombo->setCurrentIndex(0);
  }
}

void CanBusUtilizationDialog::onFrameReceived(const CanFrame &frame) {
  m_frameCount[frame.channel]++;
  int bits = frame.extended ? 131 : 111;
  if (frame.fd) {
    bits = frame.extended ? 180 : 150;
  }
  bits += frame.data.size() * 8;
  m_bitCount[frame.channel] += bits;
}

void CanBusUtilizationDialog::onFrameSent(const CanFrame &frame) {
  m_frameCount[frame.channel]++;
  int bits = frame.extended ? 131 : 111;
  if (frame.fd) {
    bits = frame.extended ? 180 : 150;
  }
  bits += frame.data.size() * 8;
  m_bitCount[frame.channel] += bits;
}

void CanBusUtilizationDialog::onRefreshPeriodChanged(const QString &text) {
  int period = 200;
  if (text == "500 ms") {
    period = 500;
  } else if (text == "1000 ms") {
    period = 1000;
  }
  m_timer->setInterval(period);
}

void CanBusUtilizationDialog::onTimerTimeout() {
  double elapsedSec = m_elapsedTimer.restart() / 1000.0;
  if (elapsedSec <= 0.0) return;

  m_pointCounter++;

  // 1. Process Left Plot
  int leftCh = m_leftChannelCombo->currentData().toInt();
  double leftFps = 0.0;
  double leftUsage = 0.0;
  if (leftCh >= 0) {
    leftFps = m_frameCount[leftCh] / elapsedSec;
    int baud = 500000;
    if (m_can) {
      baud = m_can->channelConfig(leftCh).abitBaud;
      if (baud <= 0) baud = 500000;
    }
    leftUsage = ((m_bitCount[leftCh] / elapsedSec) / baud) * 100.0;
    if (leftUsage > 100.0) leftUsage = 100.0;

    m_frameCount[leftCh] = 0;
    m_bitCount[leftCh] = 0;
  }
  m_leftRateLabel->setText(QString("当前速率: %1 帧/秒").arg(leftCh >= 0 ? QString::number(qRound(leftFps)) : "0"));
  m_leftUsageLabel->setText(QString("当前利用率: %1 %").arg(leftUsage, 0, 'f', 4));

  m_leftY.removeFirst();
  m_leftY.append(leftUsage);
  m_leftPlot->graph(0)->setData(m_leftX, m_leftY);
  m_leftPlot->replot();

  // 2. Process Right Plot
  int rightCh = m_rightChannelCombo->currentData().toInt();
  double rightFps = 0.0;
  double rightUsage = 0.0;
  if (rightCh >= 0) {
    rightFps = m_frameCount[rightCh] / elapsedSec;
    int baud = 500000;
    if (m_can) {
      baud = m_can->channelConfig(rightCh).abitBaud;
      if (baud <= 0) baud = 500000;
    }
    rightUsage = ((m_bitCount[rightCh] / elapsedSec) / baud) * 100.0;
    if (rightUsage > 100.0) rightUsage = 100.0;

    m_frameCount[rightCh] = 0;
    m_bitCount[rightCh] = 0;
  }
  m_rightRateLabel->setText(QString("当前速率: %1 帧/秒").arg(rightCh >= 0 ? QString::number(qRound(rightFps)) : "0"));
  m_rightUsageLabel->setText(QString("当前利用率: %1 %").arg(rightUsage, 0, 'f', 4));

  m_rightY.removeFirst();
  m_rightY.append(rightUsage);
  m_rightPlot->graph(0)->setData(m_rightX, m_rightY);
  m_rightPlot->replot();

  // Record history
  ExportData data;
  data.pointIndex = m_pointCounter;
  data.leftCh = leftCh;
  data.leftVal = leftUsage;
  data.rightCh = rightCh;
  data.rightVal = rightUsage;
  m_exportHistory.append(data);

  // Keep cache reasonable
  if (m_exportHistory.size() > 5000) {
    m_exportHistory.removeFirst();
  }
}

void CanBusUtilizationDialog::onSaveClicked() {
  QString filename = QFileDialog::getSaveFileName(this, "保存总线利用率数据", 
                                                    QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + "_utilization.csv",
                                                    "CSV Files (*.csv)");
  if (filename.isEmpty()) return;

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "错误", "无法写入文件！");
    return;
  }

  QTextStream out(&file);
  out.setCodec("UTF-8");
  out << QString::fromUtf8("序号,时间戳,左侧通道ID,左侧利用率(%),右侧通道ID,右侧利用率(%)\n");

  QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
  for (const auto &item : m_exportHistory) {
    out << item.pointIndex << ","
        << timeStr << ","
        << item.leftCh << ","
        << QString::number(item.leftVal, 'f', 4) << ","
        << item.rightCh << ","
        << QString::number(item.rightVal, 'f', 4) << "\n";
  }

  file.close();
  QMessageBox::information(this, "提示", "实时总线利用率历史记录已成功保存！");
}
