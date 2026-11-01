#include "cansignalplotpanel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDateTime>
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>

CanSignalPlotPanel::CanSignalPlotPanel(CanInterface *can, QWidget *parent)
    : QWidget(parent), m_can(can) {
  m_dbc = new DbcParserEngine(this);
  m_startTimeMs = QDateTime::currentMSecsSinceEpoch();

  setupUi();

  // 默认内置一套虚拟仿真 DBC，让用户离线仿真时直接有信号可以勾选！
  QString demoDbc =
      "BO_ 256 EngineData: 8 Vector__XXX\n"
      " SG_ RPM : 0|16@0+ (1,0) [0|8000] \"rpm\" Vector__XXX\n"
      " SG_ WaterTemp : 16|8@1+ (1,-40) [-40|150] \"degC\" Vector__XXX\n"
      " SG_ Throttle : 24|8@1+ (1,0) [0|100] \"%\" Vector__XXX\n"
      "BO_ 640 BodyData: 8 Vector__XXX\n"
      " SG_ Speed : 0|16@0+ (0.1,0) [0|250] \"km/h\" Vector__XXX\n"
      " SG_ Gear : 16|8@1+ (1,0) [0|8] \"\" Vector__XXX\n"
      "BO_ 800 BatteryData: 8 Vector__XXX\n"
      " SG_ Voltage : 0|16@0+ (0.1,0) [0|500] \"V\" Vector__XXX\n"
      " SG_ Current : 16|16@0- (0.1,0) [-200|200] \"A\" Vector__XXX\n"
      " SG_ SOC : 32|8@1+ (1,0) [0|100] \"%\" Vector__XXX\n";
  m_dbc->loadDbcContent(demoDbc);

  // 刷新信号列表
  for (const auto &msg : m_dbc->allMessages()) {
    for (const auto &sig : msg.signalList) {
      QListWidgetItem *item = new QListWidgetItem(QString("%1 (%2)").arg(sig.name, sig.unit));
      item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
      item->setCheckState(Qt::Unchecked);
      item->setData(Qt::UserRole, sig.name);
      item->setData(Qt::UserRole + 1, msg.id);
      m_signalListWidget->addItem(item);
      m_signalIdMap[sig.name] = msg.id;
    }
  }

  // 默认勾选前 2 个信号便于观察
  if (m_signalListWidget->count() > 0) {
    m_signalListWidget->item(0)->setCheckState(Qt::Checked);
  }
  if (m_signalListWidget->count() > 1) {
    m_signalListWidget->item(1)->setCheckState(Qt::Checked);
  }

  if (m_can) {
    connect(m_can, &CanInterface::frameReceived, this, &CanSignalPlotPanel::onFrameReceived);
  }
}

CanSignalPlotPanel::~CanSignalPlotPanel() {}

void CanSignalPlotPanel::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(2, 2, 2, 2);
  mainLayout->setSpacing(4);

  // 1. 标准视图头部栏 (与 CanViewPanel 风格一致)
  m_headerWidget = new QWidget(this);
  m_headerWidget->setObjectName("viewHeader");
  m_headerWidget->setFixedHeight(32);

  QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
  headerLayout->setContentsMargins(10, 0, 10, 0);

  m_titleLabel = new QLabel("📈 信号曲线监视视图", m_headerWidget);
  m_titleLabel->setObjectName("viewTitle");
  headerLayout->addWidget(m_titleLabel);
  headerLayout->addStretch();

  m_closeBtn = new QPushButton("✕", m_headerWidget);
  m_closeBtn->setObjectName("viewCloseBtn");
  m_closeBtn->setToolTip("关闭信号曲线视图");
  m_closeBtn->setFixedSize(22, 22);
  connect(m_closeBtn, &QPushButton::clicked, this, [this]() { emit closeRequested(this); });
  headerLayout->addWidget(m_closeBtn);

  mainLayout->addWidget(m_headerWidget);

  // 顶部工具栏
  QHBoxLayout *topLayout = new QHBoxLayout();
  m_btnLoadDbc = new QPushButton("📂 加载 DBC 文件", this);
  m_btnPause = new QPushButton("⏸️ 暂停曲线", this);
  m_btnClear = new QPushButton("🗑️ 清空数据", this);
  m_statusLabel = new QLabel("信号曲线监视已就绪 (已内置发动机/车速/电池仿真信号)", this);

  topLayout->addWidget(m_btnLoadDbc);
  topLayout->addWidget(m_btnPause);
  topLayout->addWidget(m_btnClear);
  topLayout->addWidget(m_statusLabel);
  topLayout->addStretch();
  mainLayout->addLayout(topLayout);

  // 分割窗口
  QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

  // 左侧信号勾选面板
  QWidget *leftWidget = new QWidget(this);
  QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
  leftLayout->setContentsMargins(0, 0, 0, 0);
  leftLayout->addWidget(new QLabel("<b>可监视信号列表 (勾选绘制):</b>", this));
  m_signalListWidget = new QListWidget(this);
  leftLayout->addWidget(m_signalListWidget);
  leftWidget->setMaximumWidth(260);
  splitter->addWidget(leftWidget);

  // 右侧波形图
  m_plot = new QCustomPlot(this);
  m_plot->setBackground(QBrush(QColor(24, 24, 27)));
  m_plot->xAxis->setBasePen(QPen(QColor(180, 180, 180)));
  m_plot->yAxis->setBasePen(QPen(QColor(180, 180, 180)));
  m_plot->xAxis->setTickPen(QPen(QColor(180, 180, 180)));
  m_plot->yAxis->setTickPen(QPen(QColor(180, 180, 180)));
  m_plot->xAxis->setSubTickPen(QPen(QColor(100, 100, 100)));
  m_plot->yAxis->setSubTickPen(QPen(QColor(100, 100, 100)));
  m_plot->xAxis->setTickLabelColor(QColor(200, 200, 200));
  m_plot->yAxis->setTickLabelColor(QColor(200, 200, 200));
  m_plot->xAxis->grid()->setPen(QPen(QColor(50, 50, 60), 1, Qt::DotLine));
  m_plot->yAxis->grid()->setPen(QPen(QColor(50, 50, 60), 1, Qt::DotLine));
  m_plot->xAxis->setLabel("时间 (秒)");
  m_plot->xAxis->setLabelColor(QColor(200, 200, 200));
  m_plot->yAxis->setLabel("物理量数值");
  m_plot->yAxis->setLabelColor(QColor(200, 200, 200));
  m_plot->legend->setVisible(true);
  m_plot->legend->setBrush(QBrush(QColor(30, 30, 35, 200)));
  m_plot->legend->setTextColor(QColor(220, 220, 220));
  m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

  splitter->addWidget(m_plot);
  splitter->setStretchFactor(1, 4);
  mainLayout->addWidget(splitter);

  // 信号槽
  connect(m_btnLoadDbc, &QPushButton::clicked, this, &CanSignalPlotPanel::onLoadDbcClicked);
  connect(m_btnPause, &QPushButton::clicked, this, &CanSignalPlotPanel::onPauseResumeClicked);
  connect(m_btnClear, &QPushButton::clicked, this, &CanSignalPlotPanel::onClearDataClicked);
  connect(m_signalListWidget, &QListWidget::itemChanged, this, &CanSignalPlotPanel::onSignalSelectionChanged);
}

void CanSignalPlotPanel::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss);
}

void CanSignalPlotPanel::onLoadDbcClicked() {
  QString file = QFileDialog::getOpenFileName(this, "打开 DBC 文件", "", "DBC 文件 (*.dbc)");
  if (file.isEmpty()) return;

  if (m_dbc->loadDbcFile(file)) {
    m_signalListWidget->clear();
    m_signalIdMap.clear();
    for (const auto &msg : m_dbc->allMessages()) {
      for (const auto &sig : msg.signalList) {
        QListWidgetItem *item = new QListWidgetItem(QString("%1 (%2)").arg(sig.name, sig.unit));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, sig.name);
        item->setData(Qt::UserRole + 1, msg.id);
        m_signalListWidget->addItem(item);
        m_signalIdMap[sig.name] = msg.id;
      }
    }
    m_statusLabel->setText(QString("成功加载 DBC: %1 (共 %2 个报文)").arg(QFileInfo(file).fileName()).arg(m_dbc->allMessages().size()));
  }
}

void CanSignalPlotPanel::onSignalSelectionChanged() {
  static const QList<QColor> colors = {
      QColor(56, 189, 248),  // Sky Blue
      QColor(52, 211, 153),  // Emerald
      QColor(251, 191, 36),  // Amber
      QColor(244, 114, 182), // Pink
      QColor(167, 139, 250), // Purple
      QColor(239, 68, 68)    // Red
  };

  for (int i = 0; i < m_signalListWidget->count(); ++i) {
    QListWidgetItem *item = m_signalListWidget->item(i);
    QString sigName = item->data(Qt::UserRole).toString();

    if (item->checkState() == Qt::Checked) {
      if (!m_graphs.contains(sigName)) {
        QCPGraph *graph = m_plot->addGraph();
        graph->setName(sigName);
        QColor c = colors[m_graphs.size() % colors.size()];
        graph->setPen(QPen(c, 2));
        m_graphs[sigName] = graph;
      }
    } else {
      if (m_graphs.contains(sigName)) {
        m_plot->removeGraph(m_graphs.take(sigName));
      }
    }
  }
  m_plot->replot();
}

void CanSignalPlotPanel::onPauseResumeClicked() {
  m_paused = !m_paused;
  m_btnPause->setText(m_paused ? "▶️ 继续曲线" : "⏸️ 暂停曲线");
}

void CanSignalPlotPanel::onClearDataClicked() {
  for (QCPGraph *g : m_graphs) {
    g->data()->clear();
  }
  m_startTimeMs = QDateTime::currentMSecsSinceEpoch();
  m_plot->replot();
}

void CanSignalPlotPanel::onFrameReceived(const CanFrame &frame) {
  if (m_paused || !m_dbc->hasMessage(frame.id)) return;

  CanRawFrame rf;
  rf.id = frame.id;
  rf.data = frame.data;
  rf.channel = frame.channel;

  QList<DecodedSignal> decoded = m_dbc->decodeFrame(rf);
  double timeSec = (QDateTime::currentMSecsSinceEpoch() - m_startTimeMs) / 1000.0;

  bool replotNeeded = false;
  for (const auto &ds : decoded) {
    if (m_graphs.contains(ds.name)) {
      QCPGraph *g = m_graphs.value(ds.name);
      g->addData(timeSec, ds.physicalValue);
      replotNeeded = true;
    }
  }

  if (replotNeeded) {
    m_plot->xAxis->setRange(qMax(0.0, timeSec - 10.0), timeSec + 0.5);
    m_plot->rescaleAxes(true);
    m_plot->replot(QCustomPlot::rpQueuedReplot);
  }
}
