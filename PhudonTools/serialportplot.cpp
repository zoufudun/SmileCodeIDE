#include "serialportplot.h"
#include "TOOLS/CIconFont.h"
#include "curvesettings.h"
#include "customwidget.h"
#include "mainwindow.h"
#include "toastwidget.h"
#include "verticaltabwidget.h"
#include "widgetdesigner.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QSignalBlocker>
#include <QSize>
#include <QSplitter>
#include <QTextCodec>
#include <QTextStream>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>


namespace {
QString unifiedToolTipStyleSheet() {
  return QStringLiteral("QToolTip {"
                        " color: #263238;"
                        " background-color: #FFF8E1;"
                        " border: 1px solid #FFCC80;"
                        " padding: 4px 8px;"
                        "}");
}

void ensureUnifiedToolTipStyle() {
  if (!qApp) {
    return;
  }

  const QString tooltipStyle = unifiedToolTipStyleSheet();
  QString appStyle = qApp->styleSheet();
  if (!appStyle.contains(QStringLiteral("QToolTip {"))) {
    qApp->setStyleSheet(appStyle + tooltipStyle);
  }

  QFont toolTipFont = qApp->font();
  if (toolTipFont.pointSize() < 9) {
    toolTipFont.setPointSize(9);
  }
  QToolTip::setFont(toolTipFont);
}

QString neutralDataDockStyleSheet() {
  return QStringLiteral(R"(
    QDockWidget#dataDock {
        background: #FFFFFF;
        border: 1px solid #D8DEE6;
        border-radius: 14px;
    }
    QDockWidget#dataDock::title {
        background: #F3F4F6;
        padding-top: 6px;
        padding-bottom: 6px;
        padding-left: 12px;
        padding-right: 80px;
        font-weight: bold;
        font-size: 11pt;
        color: #374151;
        border: none;
        border-bottom: 1px solid #E5E7EB;
        border-top-left-radius: 14px;
        border-top-right-radius: 14px;
        min-height: 32px;
    }
    QDockWidget#dataDock QWidget#dockContentWidget {
        background: #FFFFFF;
        border: none;
        border-bottom-left-radius: 14px;
        border-bottom-right-radius: 14px;
    }
    QDockWidget#dataDock::close-button,
    QDockWidget#dataDock::float-button {
        background: transparent;
        border: none;
        border-radius: 4px;
        padding: 2px;
        icon-size: 16px;
        subcontrol-position: center right;
        subcontrol-origin: margin;
        width: 24px;
        height: 24px;
    }
    QDockWidget#dataDock::close-button {
        right: 6px;
    }
    QDockWidget#dataDock::float-button {
        right: 34px;
    }
    QDockWidget#dataDock::close-button:hover,
    QDockWidget#dataDock::float-button:hover {
        background: rgba(148, 163, 184, 0.18);
    }
    QDockWidget#dataDock::close-button:pressed,
    QDockWidget#dataDock::float-button:pressed {
        background: rgba(100, 116, 139, 0.24);
    }
  )");
}

class ScaledAxisTicker : public QCPAxisTicker {
public:
  explicit ScaledAxisTicker(double divisor, int fixedPrecision = -1)
      : m_divisor(divisor > 0.0 ? divisor : 1.0),
        m_fixedPrecision(fixedPrecision) {}

  QString getTickLabel(double tick, const QLocale &locale, QChar formatChar,
                       int precision) Q_DECL_OVERRIDE {
    const QChar fmt = formatChar.isNull() ? QChar('f') : formatChar;
    const int p = (m_fixedPrecision >= 0) ? m_fixedPrecision : precision;
    return locale.toString(tick / m_divisor, fmt.toLatin1(), p);
  }

private:
  double m_divisor;
  int m_fixedPrecision;
};

QByteArray parseFrameMarker(const QString &text) {
  const QString trimmed = text.trimmed();
  if (trimmed.startsWith(QStringLiteral("hex:"), Qt::CaseInsensitive)) {
    return QByteArray::fromHex(trimmed.mid(4).toLatin1());
  }

  QByteArray marker;
  for (int index = 0; index < text.size(); ++index) {
    const QChar current = text.at(index);
    if (current != QLatin1Char('\\') || index + 1 >= text.size()) {
      marker.append(QString(current).toUtf8());
      continue;
    }

    const QChar escaped = text.at(++index);
    if (escaped == QLatin1Char('n')) {
      marker.append('\n');
    } else if (escaped == QLatin1Char('r')) {
      marker.append('\r');
    } else if (escaped == QLatin1Char('t')) {
      marker.append('\t');
    } else if (escaped == QLatin1Char('\\')) {
      marker.append('\\');
    } else if (escaped == QLatin1Char('x') && index + 2 < text.size()) {
      const QByteArray hex = text.mid(index + 1, 2).toLatin1();
      bool validHex = false;
      const int value = hex.toInt(&validHex, 16);
      if (validHex) {
        marker.append(static_cast<char>(value));
        index += 2;
      } else {
        marker.append("\\x");
      }
    } else {
      marker.append(QString(escaped).toUtf8());
    }
  }
  return marker;
}
} // namespace

// ============================================================================
// LineNumberWidget — 接收框行号显示组件（独立列，不属于文本区）
// ============================================================================
class LineNumberWidget : public QWidget {
  QTextEdit *m_edit = nullptr;

public:
  explicit LineNumberWidget(QWidget *parent = nullptr)
      : QWidget(parent) {
    setFixedWidth(40);
    setVisible(false);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  }

  void bindTo(QTextEdit *edit) {
    m_edit = edit;
    if (edit) {
      setFont(edit->font());
    }
  }

  QSize sizeHint() const override {
    int digits = 1;
    int max = m_edit ? qMax(1, m_edit->document()->blockCount()) : 1;
    while (max >= 10) { max /= 10; ++digits; }
    int w = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 8;
    return QSize(w, 0);
  }

  void updateWidth() {
    QSize s = sizeHint();
    setFixedWidth(s.width());
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.fillRect(rect(), QColor("#ECEFF1"));
    if (!m_edit) return;

    QPen textPen(QColor("#90A4AE"));
    p.setPen(textPen);
    p.setFont(m_edit->font());

    // 对齐 QTextEdit 文本内容的垂直偏移
    QAbstractTextDocumentLayout *layout =
        m_edit->document()->documentLayout();
    int scrollY = m_edit->verticalScrollBar()->value();
    // QTextEdit 内容区顶部有 ~4px 的 margin
    int contentOffset = qRound(m_edit->document()->documentMargin());
    int w = width() - 6;

    for (QTextBlock block = m_edit->document()->begin(); block.isValid();
         block = block.next()) {
      QRectF br = layout->blockBoundingRect(block);
      int top = qRound(br.top()) - scrollY + contentOffset;
      int h = qRound(br.height());
      if (top + h < 0) continue;
      if (top > height()) break;
      p.drawText(0, top, w, h, Qt::AlignRight | Qt::AlignVCenter,
                 QString::number(block.blockNumber() + 1));
    }
  }
};

SerialSession::SerialSession(QWidget *parent)
    : QWidget(parent), m_lastPortCount(0), m_rxCount(0), m_txCount(0),
      m_xValue(0), m_xAxisScale(1.0), m_viewWidthPoints(100),
      m_widgetDesigner(nullptr), m_widgetToolbox(nullptr),
      m_leftTabWidget(nullptr), m_converterState(nullptr),
      m_lineNumberWidget(nullptr) {
  ensureUnifiedToolTipStyle();
  m_serial = new QSerialPort(this);
  m_autoSendTimer = new QTimer(this);
  m_portCheckTimer = new QTimer(this);

  // Multi-send: init with 1 page of 20 empty items
  m_multiPage = 0;
  m_multiLoopIndex = 0;
  m_multiLoopTimer = new QTimer(this);
  m_multiPages.append(QVector<MultiSendItem>(MULTI_PER_PAGE));

  m_welcomeText = "   欢迎使用uSmilePro串口示波器V1.0   ";

  // Render Throttling Setup
  m_needsReplot = false;
  m_replotTimer = new QTimer(this);
  m_replotTimer->setInterval(33); // ~30 FPS limit for QCustomPlot

  m_frameTail = "\n";

  setupUi();
  setupChart();
  setupConnections();
  refreshMultiPage(); // init multi-send page display
  refreshPorts();

  // Start timers
  m_portCheckTimer->start(1000);
  m_replotTimer->start();
}

SerialSession::~SerialSession() {
  if (m_serial->isOpen())
    m_serial->close();
  delete m_converterState;
  m_converterState = nullptr;
}

void SerialSession::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  m_innerMainWindow = new QMainWindow(this);
  m_innerMainWindow->setWindowFlags(Qt::Widget);
  m_innerMainWindow->setDockOptions(QMainWindow::AnimatedDocks |
                                    QMainWindow::AllowNestedDocks);
  m_innerMainWindow->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);

  // 先创建 central widget，再添加 Dock，避免 Dock 被错误放置到顶部区域
  m_mainHorizSplitter = new QSplitter(Qt::Horizontal);
  m_innerMainWindow->setCentralWidget(m_mainHorizSplitter);

  // === 左侧垂直标签页 ===
  m_leftTabWidget = new VerticalTabWidget();
  m_leftTabWidget->setMinimumWidth(400);
  m_leftTabWidget->setMaximumWidth(600);
  m_mainHorizSplitter->addWidget(m_leftTabWidget);

  // 左侧数据区（接收/发送）由独立的 Dock Host 托管，保证真正支持浮动/拖动/关闭
  QMainWindow *dataDockHost = new QMainWindow();
  dataDockHost->setWindowFlags(Qt::Widget);
  dataDockHost->setDockOptions(QMainWindow::AnimatedDocks |
                               QMainWindow::AllowNestedDocks |
                               QMainWindow::AllowTabbedDocks);
  dataDockHost->setStyleSheet(
      "QMainWindow { background: transparent; border: none; }");
  QWidget *dataDockPlaceholder = new QWidget(dataDockHost);
  dataDockPlaceholder->setMinimumSize(0, 0);
  dataDockPlaceholder->setSizePolicy(QSizePolicy::Ignored,
                                     QSizePolicy::Ignored);
  dataDockPlaceholder->hide();
  dataDockHost->setCentralWidget(dataDockPlaceholder);
  m_mainHorizSplitter->addWidget(dataDockHost);
  m_dataSplitter = dataDockHost;

  // Global Stylesheet for Custom Cards
  this->setStyleSheet("SerialPortPlot { background-color: #f5f5f5; }"
                      "QWidget#cardWidget { "
                      "    background-color: #FFFFFF; "
                      "    border: 1px solid #E0E0E0; "
                      "    border-radius: 12px; "
                      "}"
                      "QLabel#cardTitle { "
                      "    font-weight: bold; "
                      "    font-size: 13px; "
                      "    color: #333333; "
                      "    padding-bottom: 5px; "
                      "}"
                      "QTextEdit { "
                      "    border: 1px solid #4CAF50; "
                      "    border-radius: 10px; "
                      "    padding: 5px; "
                      "    background-color: #FAFAFA; "
                      "}");

  // 1. Port Settings
  QWidget *grpPort = new QWidget();
  grpPort->setObjectName("cardWidget");
  QVBoxLayout *portMainLayout = new QVBoxLayout(grpPort);
  portMainLayout->setContentsMargins(10, 10, 10, 10);

  QGridLayout *portLayout = new QGridLayout();
  portLayout->setContentsMargins(0, 0, 0, 0);
  portMainLayout->addLayout(portLayout);

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

  // 刷新按钮：纯图标样式，无按钮边框
  m_btnRefresh = new QPushButton(QChar(0xE84D));
  m_btnRefresh->setFont(CIconFont::instance()->getIconFont(50));
  m_btnRefresh->setToolTip("刷新端口");
  m_btnRefresh->setMinimumWidth(52);
  m_btnRefresh->setMinimumHeight(44);
  m_btnRefresh->setFlat(true);
  m_btnRefresh->setStyleSheet(
      "QPushButton { background: transparent; border: none; border-radius: 8px;"
      "  color: #1565C0; padding: 4px; }"
      "QPushButton:hover { background: rgba(21,101,192,40); }"
      "QPushButton:pressed { background: rgba(21,101,192,80); }");

  // m_btnRefresh->setStyleSheet(
  //     "QPushButton { color: #555555; background: #E3F2FD; border: 1px solid "
  //     "#BBDEFB; border-radius: 16px; }"
  //     "QPushButton:hover { background: #BBDEFB; color: #1976D2; }"
  //     "QPushButton:checked { background: #C8E6C9; color: #388E3C; "
  //     "border-color: #A5D6A7; }");
  portActionLayout->addWidget(m_btnRefresh);

  // 打开串口按钮：带背景的按钮样式
  m_btnOpenClose = new QPushButton(QChar(0xe84e));
  m_btnOpenClose->setFont(CIconFont::instance()->getIconFont(50));
  m_btnOpenClose->setCheckable(true);
  m_btnOpenClose->setToolTip("打开串口");
  m_btnOpenClose->setMinimumWidth(52);
  m_btnOpenClose->setMinimumHeight(44);
  m_btnOpenClose->setFlat(true);
  m_btnOpenClose->setStyleSheet(
      "QPushButton { background: transparent; border: none; border-radius: 8px;"
      "  color: #1565C0; padding: 4px; }"
      "QPushButton:hover { background: rgba(21,101,192,40); }"
      "QPushButton:pressed { background: rgba(21,101,192,80); }"
      "QPushButton:checked { color: #C62828; }");

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

  // 将串口设置添加到垂直标签页（第一个标签）
  m_leftTabWidget->addTab(grpPort, "串口设置", QString(QChar(0xe890)));

  // 2. Receive Settings
  QWidget *grpRx = new QWidget();
  grpRx->setObjectName("cardWidget");
  QVBoxLayout *rxMainLayout = new QVBoxLayout(grpRx);
  rxMainLayout->setContentsMargins(10, 10, 10, 10);

  QVBoxLayout *rxLayout = new QVBoxLayout();
  rxLayout->setContentsMargins(0, 0, 0, 0);
  rxMainLayout->addLayout(rxLayout);

  QHBoxLayout *rxModeLayout = new QHBoxLayout();
  m_rbRxAscii = new QRadioButton("ASCII");
  m_rbRxHex = new QRadioButton("HEX");
  m_rbRxAscii->setChecked(true);
  rxModeLayout->addWidget(m_rbRxAscii);
  rxModeLayout->addWidget(m_rbRxHex);
  rxLayout->addLayout(rxModeLayout);

  m_chkRxLog = new QCheckBox("日志模式");
  m_chkRxTime = new QCheckBox("显示时间");

  rxLayout->addWidget(m_chkRxLog);
  rxLayout->addWidget(m_chkRxTime);

  // Instantiate the logical buttons that are used by the Rx panel signals.
  // They are no longer added to the layout directly (replaced by floating
  // controls), but their logical state (isChecked, clicked) is required by
  // onReadyRead and toggles.
  m_btnStopRx = new QPushButton(this);
  m_btnStopRx->setCheckable(true);
  m_btnStopRx->setVisible(false);
  m_btnClearRx = new QPushButton(this);
  m_btnClearRx->setVisible(false);

  // Floating controls will replace the inline buttons later, but keeping for
  // now or remove if redundant

  m_dockRx = new QDockWidget("接收设置", m_innerMainWindow);
  m_dockRx->setFeatures(QDockWidget::DockWidgetMovable |
                        QDockWidget::DockWidgetFloatable |
                        QDockWidget::DockWidgetClosable);
  m_dockRx->setAllowedAreas(Qt::LeftDockWidgetArea);
  m_dockRx->setWidget(grpRx);
  m_innerMainWindow->addDockWidget(Qt::LeftDockWidgetArea, m_dockRx);
  m_dockRx->hide(); // Requirement 1: Hide by default

  // 3. Send Settings
  QWidget *grpTx = new QWidget();
  grpTx->setObjectName("cardWidget");
  QVBoxLayout *txMainLayout = new QVBoxLayout(grpTx);
  txMainLayout->setContentsMargins(10, 10, 10, 10);

  QVBoxLayout *txLayout = new QVBoxLayout();
  txLayout->setContentsMargins(0, 0, 0, 0);
  txMainLayout->addLayout(txLayout);

  QHBoxLayout *txModeLayout = new QHBoxLayout();
  m_rbTxAscii = new QRadioButton("ASCII");
  m_rbTxHex = new QRadioButton("HEX");
  m_rbTxAscii->setChecked(true);
  txModeLayout->addWidget(m_rbTxAscii);
  txModeLayout->addWidget(m_rbTxHex);
  txLayout->addLayout(txModeLayout);

  // Dock-side controls are kept as mirrors of the floating controls below.
  QCheckBox *dockChkTxNewLine = new QCheckBox("发送新行");
  m_chkTxTime = new QCheckBox("显示时间"); // Timestamp in local log
  txLayout->addWidget(dockChkTxNewLine);
  txLayout->addWidget(m_chkTxTime);

  QCheckBox *dockChkAutoSend = new QCheckBox("自动发送(ms):");
  dockChkAutoSend->setEnabled(false); // Disabled until port is open
  QSpinBox *dockSpinAutoSendInterval = new QSpinBox();
  dockSpinAutoSendInterval->setRange(10, 10000);
  dockSpinAutoSendInterval->setValue(1000);
  dockSpinAutoSendInterval->setEnabled(false); // Disabled until port is open

  QHBoxLayout *autoSendLayout = new QHBoxLayout();
  autoSendLayout->addWidget(dockChkAutoSend);
  autoSendLayout->addWidget(dockSpinAutoSendInterval);
  txLayout->addLayout(autoSendLayout);

  m_dockTx = new QDockWidget("发送设置", m_innerMainWindow);
  m_dockTx->setFeatures(QDockWidget::DockWidgetMovable |
                        QDockWidget::DockWidgetFloatable |
                        QDockWidget::DockWidgetClosable);
  m_dockTx->setAllowedAreas(Qt::LeftDockWidgetArea);
  m_dockTx->setWidget(grpTx);
  m_innerMainWindow->addDockWidget(Qt::LeftDockWidgetArea, m_dockTx);
  m_dockTx->hide(); // Requirement 1: Hide by default

  // === 新增：示波器设置 Card ===
  QWidget *grpScope = new QWidget();
  grpScope->setObjectName("cardWidget");
  QVBoxLayout *grpScopeMainLayout = new QVBoxLayout(grpScope);
  grpScopeMainLayout->setContentsMargins(10, 10, 10, 10);

  m_settingsLayout = grpScopeMainLayout;

  const QFont scopeIconFont = CIconFont::instance()->getIconFont(50);
  auto setupScopeIconCheck = [&scopeIconFont](QCheckBox *check,
                                              const QString &icon,
                                              const QString &tooltip) {
    check->setFont(scopeIconFont);
    check->setText(icon);
    check->setToolTip(tooltip);
    check->setCursor(Qt::PointingHandCursor);
    check->setStyleSheet("QCheckBox {"
                         "  background: transparent;"
                         "  border: none;"
                         "  color: #546E7A;"
                         "  font-size: 50px;"
                         "  padding: 0px;"
                         "  margin: 0px;"
                         "}"
                         "QCheckBox:hover { color: #42A5F5; }"
                         "QCheckBox:checked { color: #1976D2; }"
                         "QCheckBox::indicator { width: 0px; height: 0px; }");
    check->setFixedSize(62, 62);
  };

  auto setupScopeIconButton =
      [&scopeIconFont](QPushButton *button, const QString &icon,
                       const QString &tooltip, bool checkable) {
        button->setFont(scopeIconFont);
        button->setText(icon);
        button->setToolTip(tooltip);
        button->setCheckable(checkable);
        button->setCursor(Qt::PointingHandCursor);
        button->setFlat(true);
        button->setFixedSize(62, 62);
        button->setStyleSheet("QPushButton {"
                              "  background: transparent;"
                              "  border: none;"
                              "  color: #546E7A;"
                              "  font-size: 50px;"
                              "  padding: 0px;"
                              "  margin: 0px;"
                              "}"
                              "QPushButton:hover { color: #42A5F5; }"
                              "QPushButton:pressed { color: #1E88E5; }"
                              "QPushButton:checked { color: #1976D2; }");
      };

  m_chkEnableWaveform = new QCheckBox();
  setupScopeIconCheck(m_chkEnableWaveform, "\ue86e", "启用/关闭波形显示");

  m_chkShowGrid = new QCheckBox();
  setupScopeIconCheck(m_chkShowGrid, "\ue866", "显示/隐藏网格");
  m_chkShowGrid->setChecked(true);

  m_chkHideRxTx = new QCheckBox();
  setupScopeIconCheck(m_chkHideRxTx, "\ue9db", "隐藏/显示收发区");

  m_chkHideRxData = new QCheckBox();
  setupScopeIconCheck(m_chkHideRxData, "\ue883", "不显示接收数据");

  m_chkShowRawData = new QCheckBox();
  setupScopeIconCheck(m_chkShowRawData, "\ue881", "显示原始数据");

  m_btnAutoScale = new QPushButton();
  setupScopeIconButton(m_btnAutoScale, "\ue879", "自动缩放开关", true);
  m_btnAutoScale->setChecked(true);

  m_btnClearWaveform = new QPushButton();
  setupScopeIconButton(m_btnClearWaveform, "\ue604", "清空波形数据", false);

  m_btnResetChart = new QPushButton();
  setupScopeIconButton(m_btnResetChart, "\ue85d", "重置图表设置", false);

  m_btnStopWaveform = new QPushButton();
  setupScopeIconButton(m_btnStopWaveform, "\ue87c", "暂停/继续波形显示", true);

  m_btnCurveSettings = new QPushButton();
  setupScopeIconButton(m_btnCurveSettings, "\ue872", "查看/修改曲线样式",
                       false);

  QGridLayout *grpScopeLayout = new QGridLayout();
  grpScopeLayout->setContentsMargins(0, 0, 0, 0);
  grpScopeLayout->setHorizontalSpacing(12);
  grpScopeLayout->setVerticalSpacing(10);
  grpScopeMainLayout->addLayout(grpScopeLayout);

  // 顶部快捷图标栏（两行四列）
  QWidget *quickPanel = new QWidget();
  quickPanel->setObjectName("scopeQuickPanel");
  QGridLayout *quickLayout = new QGridLayout(quickPanel);
  quickLayout->setContentsMargins(10, 8, 10, 8);
  quickLayout->setSpacing(10);
  quickPanel->setStyleSheet("QWidget#scopeQuickPanel {"
                            "  background: #F7F9FB;"
                            "  border: 1px solid #E3E8EE;"
                            "  border-radius: 10px;"
                            "}");

  quickLayout->addWidget(m_chkEnableWaveform, 0, 0, Qt::AlignCenter);
  quickLayout->addWidget(m_chkShowGrid, 0, 1, Qt::AlignCenter);
  quickLayout->addWidget(m_btnAutoScale, 0, 2, Qt::AlignCenter);
  quickLayout->addWidget(m_btnCurveSettings, 0, 3, Qt::AlignCenter);
  quickLayout->addWidget(m_btnStopWaveform, 1, 0, Qt::AlignCenter);
  quickLayout->addWidget(m_btnClearWaveform, 1, 1, Qt::AlignCenter);
  quickLayout->addWidget(m_btnResetChart, 1, 2, Qt::AlignCenter);
  quickLayout->addWidget(m_chkShowRawData, 1, 3, Qt::AlignCenter);
  quickLayout->addWidget(m_chkHideRxTx, 2, 0, Qt::AlignCenter);
  quickLayout->addWidget(m_chkHideRxData, 2, 1, Qt::AlignCenter);

  for (int c = 0; c < 4; ++c) {
    quickLayout->setColumnStretch(c, 1);
  }

  grpScopeLayout->addWidget(quickPanel, 0, 0, 1, 2);

  // 左列：绘图参数 + Y 轴
  m_groupPlotParams = new QGroupBox("绘图参数");
  QGridLayout *paramsLayout = new QGridLayout(m_groupPlotParams);
  paramsLayout->setContentsMargins(10, 8, 10, 10);
  paramsLayout->setHorizontalSpacing(12);
  paramsLayout->setVerticalSpacing(8);

  paramsLayout->addWidget(new QLabel("视窗宽度(∆t):"), 0, 0);
  m_spinPoints = new QDoubleSpinBox();
  m_spinPoints->setKeyboardTracking(false);
  paramsLayout->addWidget(m_spinPoints, 0, 1);

  paramsLayout->addWidget(new QLabel("缓冲区上限:"), 1, 0);
  m_spinBufferLimit = new QSpinBox();
  m_spinBufferLimit->setRange(100, 1000000);
  m_spinBufferLimit->setValue(10000);
  paramsLayout->addWidget(m_spinBufferLimit, 1, 1);

  paramsLayout->addWidget(new QLabel("X 轴单位:"), 2, 0);
  m_comboTimeUnit = new QComboBox();
  m_comboTimeUnit->addItems({"点数 (Points)", "毫秒 (ms)", "秒 (s)"});
  paramsLayout->addWidget(m_comboTimeUnit, 2, 1);

  paramsLayout->addWidget(new QLabel("采样周期:"), 3, 0);
  m_spinSampleInterval = new QDoubleSpinBox();
  m_spinSampleInterval->setRange(0.001, 60000.0);
  m_spinSampleInterval->setDecimals(3);
  m_spinSampleInterval->setSingleStep(0.1);
  m_spinSampleInterval->setValue(1.0);
  m_spinSampleInterval->setSuffix(" ms/点");
  m_spinSampleInterval->setToolTip("设置每个采样点对应的真实时间");
  paramsLayout->addWidget(m_spinSampleInterval, 3, 1);

  paramsLayout->addWidget(new QLabel("波形主题:"), 4, 0);
  m_comboChartTheme = new QComboBox();
  m_comboChartTheme->addItems({"亮色主题", "暗黑炫光", "科幻示波器"});
  m_comboChartTheme->setCurrentIndex(1);
  paramsLayout->addWidget(m_comboChartTheme, 4, 1);

  m_groupPlotParams->setStyleSheet(
      "QGroupBox { border: 1px solid #E3E8EE; border-radius: 10px; margin-top: "
      "8px; background: #FFFFFF; }"
      "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: "
      "top left; padding: 0 8px; color: #1B5E20; font-weight: 700; }");

  m_groupYAxis = new QGroupBox("Y 轴控制");
  QGridLayout *yLayout = new QGridLayout(m_groupYAxis);
  yLayout->setContentsMargins(10, 8, 10, 10);
  yLayout->setHorizontalSpacing(12);
  yLayout->setVerticalSpacing(8);

  yLayout->addWidget(new QLabel("最小值:"), 0, 0);
  m_spinYMin = new QDoubleSpinBox();
  m_spinYMin->setRange(-99999, 99999);
  m_spinYMin->setEnabled(false);
  yLayout->addWidget(m_spinYMin, 0, 1);

  yLayout->addWidget(new QLabel("最大值:"), 1, 0);
  m_spinYMax = new QDoubleSpinBox();
  m_spinYMax->setRange(-99999, 99999);
  m_spinYMax->setEnabled(false);
  yLayout->addWidget(m_spinYMax, 1, 1);

  yLayout->addWidget(new QLabel("刻度:"), 2, 0);
  m_spinYTick = new QDoubleSpinBox();
  m_spinYTick->setRange(0, 99999);
  m_spinYTick->setEnabled(false);
  yLayout->addWidget(m_spinYTick, 2, 1);

  m_groupYAxis->setStyleSheet(
      "QGroupBox { border: 1px solid #E3E8EE; border-radius: 10px; margin-top: "
      "8px; background: #FFFFFF; }"
      "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: "
      "top left; padding: 0 8px; color: #1B5E20; font-weight: 700; }");

  // 右列：数据帧配置 + 通道管理 (构造卡片组件)
  m_groupFrameConfig = new QGroupBox("数据帧配置");
  QGridLayout *frameLayout = new QGridLayout(m_groupFrameConfig);
  frameLayout->setContentsMargins(12, 12, 12, 12);
  frameLayout->setHorizontalSpacing(10);
  frameLayout->setVerticalSpacing(10);

  const QString frameFieldStyle =
      "QLineEdit {"
      "  border: 1px solid #CFD8DC;"
      "  border-radius: 6px;"
      "  padding: 4px 8px;"
      "  min-height: 26px;"
      "  background: #FFFFFF;"
      "  color: #263238;"
      "  font-size: 12px;"
      "  font-family: Consolas, 'Courier New', monospace;"
      "}"
      "QLineEdit:focus { border: 1px solid #43A047; background: #FAFAFA; }";
  const QString frameLabelStyle =
      "QLabel { font-weight: 600; font-size: 12px; color: #37474F; background: transparent; }";

  QLabel *lblFrameHeader = new QLabel("帧头标示:");
  lblFrameHeader->setStyleSheet(frameLabelStyle);
  frameLayout->addWidget(lblFrameHeader, 0, 0);

  m_editFrameHeader = new QLineEdit();
  m_editFrameHeader->setPlaceholderText("可选，如 $ 或 hex:AA 55");
  m_editFrameHeader->setStyleSheet(frameFieldStyle);
  m_editFrameHeader->setToolTip(
      "支持普通文本、\\n /\\r /\\t /\\xNN 转义，或 hex:AA 55 形式的十六进制");
  frameLayout->addWidget(m_editFrameHeader, 0, 1);

  QLabel *lblFrameTail = new QLabel("帧尾标示:");
  lblFrameTail->setStyleSheet(frameLabelStyle);
  frameLayout->addWidget(lblFrameTail, 1, 0);

  m_editFrameTail = new QLineEdit("\\n");
  m_editFrameTail->setPlaceholderText("必填，如 ; 或 \\n 或 hex:0D 0A");
  m_editFrameTail->setStyleSheet(frameFieldStyle);
  m_editFrameTail->setToolTip(
      "帧尾不能为空；支持普通文本、\\n /\\r /\\t /\\xNN 转义，或 hex:0D 0A");
  frameLayout->addWidget(m_editFrameTail, 1, 1);

  m_chkStrictFrame = new QCheckBox("严格帧匹配 (仅完整数据帧绘图)");
  m_chkStrictFrame->setChecked(true);
  m_chkStrictFrame->setEnabled(false);
  m_chkStrictFrame->setToolTip(
      "已强制启用：只有同时匹配所配置帧头与帧尾的完整数据帧才会显示波形，"
      "不完整或不匹配的数据会被丢弃。");
  m_chkStrictFrame->setStyleSheet(
      "QCheckBox { font-weight: 600; font-size: 12px; color: #1B5E20; background: transparent; padding: 2px 0px; }"
      "QCheckBox::indicator { width: 14px; height: 14px; }");
  frameLayout->addWidget(m_chkStrictFrame, 2, 0, 1, 2);

  m_lblFramePreview = new QLabel();
  m_lblFramePreview->setAlignment(Qt::AlignCenter);
  m_lblFramePreview->setWordWrap(true);
  m_lblFramePreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  m_lblFramePreview->setMinimumHeight(48);
  frameLayout->addWidget(m_lblFramePreview, 3, 0, 1, 2);

  frameLayout->setColumnStretch(1, 1);

  m_groupFrameConfig->setStyleSheet(
      "QGroupBox { border: 1px solid #C8E6C9; border-radius: 10px; margin-top: "
      "8px; background: #FAFFF9; }"
      "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: "
      "top left; padding: 0 10px; color: #1B5E20; font-weight: 700; font-size: 13px; }");

  m_groupChannelConfig = new QGroupBox("通道管理");
  QVBoxLayout *channelOuterLayout = new QVBoxLayout(m_groupChannelConfig);
  channelOuterLayout->setContentsMargins(10, 8, 10, 10);
  channelOuterLayout->setSpacing(6);

  m_lblChannelHint = new QLabel("等待符合帧格式的数据，通道将自动创建");
  m_lblChannelHint->setAlignment(Qt::AlignCenter);
  m_lblChannelHint->setWordWrap(true);
  m_lblChannelHint->setStyleSheet(
      "QLabel {"
      "  color: #90A4AE;"
      "  background: #FAFBFC;"
      "  border: 1px dashed #CFD8DC;"
      "  border-radius: 8px;"
      "  padding: 10px 8px;"
      "}");
  channelOuterLayout->addWidget(m_lblChannelHint);

  QScrollArea *channelScroll = new QScrollArea();
  channelScroll->setWidgetResizable(true);
  channelScroll->setFrameShape(QFrame::NoFrame);
  channelScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  channelScroll->setStyleSheet("QScrollArea { background: transparent; }");

  QWidget *channelHost = new QWidget();
  channelHost->setStyleSheet("background: transparent;");
  m_channelsLayout = new QVBoxLayout(channelHost);
  m_channelsLayout->setContentsMargins(0, 2, 0, 2);
  m_channelsLayout->setSpacing(6);
  m_channelsLayout->addStretch();

  channelScroll->setWidget(channelHost);
  channelOuterLayout->addWidget(channelScroll, 1);

  m_groupChannelConfig->setStyleSheet(
      "QGroupBox { border: 1px solid #E3E8EE; border-radius: 10px; margin-top: "
      "8px; background: #FFFFFF; }"
      "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: "
      "top left; padding: 0 8px; color: #1B5E20; font-weight: 700; }");

  // 左列：绘图参数 + Y 轴 (使用垂直 QSplitter 支持上下拖动调节)
  QSplitter *leftSplitter = new QSplitter(Qt::Vertical);
  leftSplitter->setObjectName("scopeLeftSplitter");
  leftSplitter->setChildrenCollapsible(false);
  leftSplitter->setHandleWidth(6);
  leftSplitter->setStyleSheet(
      "QSplitter#scopeLeftSplitter::handle {"
      "  background: #E3E8EE;"
      "  border-radius: 2px;"
      "  margin: 2px 0px;"
      "}"
      "QSplitter#scopeLeftSplitter::handle:hover {"
      "  background: #42A5F5;"
      "}");
  leftSplitter->addWidget(m_groupPlotParams);
  leftSplitter->addWidget(m_groupYAxis);
  leftSplitter->setStretchFactor(0, 1);
  leftSplitter->setStretchFactor(1, 1);

  // 右列：数据帧配置 + 通道管理 (使用垂直 QSplitter 支持上下拖动调节)
  QSplitter *rightSplitter = new QSplitter(Qt::Vertical);
  rightSplitter->setObjectName("scopeRightSplitter");
  rightSplitter->setChildrenCollapsible(false);
  rightSplitter->setHandleWidth(6);
  rightSplitter->setStyleSheet(
      "QSplitter#scopeRightSplitter::handle {"
      "  background: #E3E8EE;"
      "  border-radius: 2px;"
      "  margin: 2px 0px;"
      "}"
      "QSplitter#scopeRightSplitter::handle:hover {"
      "  background: #42A5F5;"
      "}");
  rightSplitter->addWidget(m_groupFrameConfig);
  rightSplitter->addWidget(m_groupChannelConfig);
  rightSplitter->setStretchFactor(0, 0);
  rightSplitter->setStretchFactor(1, 1);

  grpScopeLayout->addWidget(leftSplitter, 1, 0);
  grpScopeLayout->addWidget(rightSplitter, 1, 1);

  grpScopeLayout->setRowStretch(1, 1);
  grpScopeLayout->setColumnStretch(0, 1);
  grpScopeLayout->setColumnStretch(1, 1);

  // 将示波器设置添加到垂直标签页（第二个标签）
  m_leftTabWidget->addTab(grpScope, "示波器设置", QString(QChar(0xe86d)));

  m_lblRxCount = new QLabel("0");
  m_lblTxCount = new QLabel("0");

  // === 控件设计器区域 - 垂直分割（左：控件库，右：实例化区）===
  QSplitter *designerSplitter = new QSplitter(Qt::Horizontal);
  designerSplitter->setObjectName("designerSplitter");
  designerSplitter->setChildrenCollapsible(false);
  designerSplitter->setHandleWidth(6);
  designerSplitter->setStyleSheet("QSplitter#designerSplitter::handle {"
                                  "  background: #DADCE0;"
                                  "}"
                                  "QSplitter#designerSplitter::handle:hover {"
                                  "  background: #BDC1C6;"
                                  "}");

  // 左侧：工具箱（控件库）
  m_widgetToolbox = new WidgetToolbox(designerSplitter);
  m_widgetToolbox->setMinimumWidth(220);
  m_widgetToolbox->setMaximumWidth(360);

  // 右侧：设计器区域（控件放置区）
  m_widgetDesigner = new WidgetDesignerArea(designerSplitter);

  designerSplitter->addWidget(m_widgetToolbox);
  designerSplitter->addWidget(m_widgetDesigner);
  designerSplitter->setStretchFactor(0, 0);
  designerSplitter->setStretchFactor(1, 1);
  designerSplitter->setSizes({240, 600});

  // 将控件设计器添加到垂直标签页（第三个标签）
  m_designerTabIndex = m_leftTabWidget->addTab(designerSplitter, "控件设计器",
                                               QString(QChar(0xe88b)));

  // 连接工具箱按钮信号到设计器
  connect(m_widgetToolbox, &WidgetToolbox::addProtocolButton, m_widgetDesigner,
          [this]() { m_widgetDesigner->addProtocolButton(QPoint()); });
  connect(m_widgetToolbox, &WidgetToolbox::addScopeWidget, m_widgetDesigner,
          [this]() { m_widgetDesigner->addScopeWidget(QPoint()); });
  connect(m_widgetToolbox, &WidgetToolbox::addLedWidget, m_widgetDesigner,
          [this]() { m_widgetDesigner->addLedWidget(QPoint()); });

  // 连接设计器的发送数据信号
  connect(m_widgetDesigner, &WidgetDesignerArea::sendData, this,
          [this](const QByteArray &data) {
            if (m_serial && m_serial->isOpen()) {
              m_serial->write(data);
              m_txCount += data.size();
              updateStatusInfo();
            }
          });

  // Receive Area - 改为DockWidget
  QWidget *grpData = new QWidget();
  grpData->setObjectName("dockContentWidget");
  QVBoxLayout *dataMainLayout = new QVBoxLayout(grpData);
  dataMainLayout->setContentsMargins(10, 10, 10, 10);

  QVBoxLayout *dataLayout = new QVBoxLayout();
  dataLayout->setContentsMargins(0, 0, 0, 0);
  dataMainLayout->addLayout(dataLayout);

  // ===== 接收区：行号列 + 文本框 水平排列 =====
  QWidget *rxContainer = new QWidget();
  QHBoxLayout *rxContainerLayout = new QHBoxLayout(rxContainer);
  rxContainerLayout->setContentsMargins(0, 0, 0, 0);
  rxContainerLayout->setSpacing(0);

  // 行号组件（独立列，默认隐藏）
  m_lineNumberWidget = new LineNumberWidget();
  rxContainerLayout->addWidget(m_lineNumberWidget);

  m_textReceive = new QTextEdit();
  m_textReceive->document()->setMaximumBlockCount(1000);
  m_textReceive->setReadOnly(true);
  m_textReceive->installEventFilter(this);
  rxContainerLayout->addWidget(m_textReceive, 1);

  // 绑定行号组件到文本框
  m_lineNumberWidget->bindTo(m_textReceive);

  // 行号更新信号
  connect(m_textReceive->document(), &QTextDocument::blockCountChanged, this,
          [this]() {
            if (m_showLineNumbers)
              updateLineNumberDisplay();
          });
  connect(m_textReceive->verticalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() {
            if (m_showLineNumbers)
              m_lineNumberWidget->update();
          });

  // Floating container（悬浮在 m_textReceive 上方）
  QWidget *rxFloatWidget = new QWidget(m_textReceive);
  rxFloatWidget->setObjectName("rxFloat");
  rxFloatWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
  rxFloatWidget->setStyleSheet("background: transparent; border: none;");
  rxFloatWidget->raise();

  QHBoxLayout *rxFloatLayout = new QHBoxLayout(rxFloatWidget);
  rxFloatLayout->setContentsMargins(0, 0, 0, 0);
  rxFloatLayout->setSpacing(8);

  auto createFloatBtn = [](QChar iconCode) {
    QToolButton *btn = new QToolButton();
    btn->setFont(CIconFont::instance()->getIconFont(25));
    btn->setText(iconCode);
    btn->setFixedSize(32, 32);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setCheckable(true);
    btn->setStyleSheet(
        "QToolButton { color: #555555; background: #E3F2FD; border: 1px solid "
        "#BBDEFB; border-radius: 16px; }"
        "QToolButton:hover { background: #BBDEFB; color: #1976D2; }"
        "QToolButton:checked { background: #C8E6C9; color: #388E3C; "
        "border-color: #A5D6A7; }"
        "QToolTip { color: #263238; background: #FFF8E1; border: 1px solid "
        "#FFCC80; padding: 4px 8px; }");
    return btn;
  };

  // 行号图标 0xe89c（悬浮最左侧）
  m_btnRxLineNumber = createFloatBtn(QChar(0xe89c));
  m_btnRxLineNumber->setToolTip("显示/隐藏行号");
  connect(m_btnRxLineNumber, &QToolButton::toggled, this, [this](bool on) {
    m_showLineNumbers = on;
    updateLineNumberDisplay();
  });

  // 编码图标 0xf9d5（悬浮在行号图标右侧）
  m_btnRxEncoding = createFloatBtn(QChar(0xf9d5));
  m_btnRxEncoding->setToolTip("选择文本编码");
  m_btnRxEncoding->setCheckable(false);

  m_btnRxHexToggle = createFloatBtn(QChar(0xEBBC));
  m_btnRxTimeToggle = createFloatBtn(QChar(0xE676));
  m_btnRxPauseToggle = createFloatBtn(QChar(0xE617));
  m_btnRxClear = createFloatBtn(QChar(0xE621));
  m_btnRxClear->setCheckable(false);

  m_btnRxHexToggle->setToolTip("HEX显示");
  m_btnRxTimeToggle->setToolTip("显示时间");
  m_btnRxPauseToggle->setToolTip("暂停接收");
  m_btnRxClear->setToolTip("清空接收区");

  rxFloatLayout->addWidget(m_btnRxLineNumber);
  rxFloatLayout->addWidget(m_btnRxEncoding);
  rxFloatLayout->addWidget(m_btnRxHexToggle);
  rxFloatLayout->addWidget(m_btnRxTimeToggle);
  rxFloatLayout->addWidget(m_btnRxPauseToggle);
  rxFloatLayout->addWidget(m_btnRxClear);

  // 编码图标弹出菜单：列出全部可用编码
  connect(m_btnRxEncoding, &QToolButton::clicked, this, [this]() {
    QMenu menu(m_btnRxEncoding);
    menu.setStyleSheet(
        "QMenu { background: #FFFFFF; border: 1px solid #D0D7DE; "
        "border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 6px 32px 6px 16px; border-radius: 4px; }"
        "QMenu::item:selected { background: #E8EDF5; color: #1565C0; }"
        "QMenu::separator { height: 1px; background: #E5E7EB; margin: 4px 8px; }");

    // 自动检测（始终第一项，data 为空表示自动）
    QAction *actAuto = menu.addAction("自动检测（UTF-8 / GBK）");
    actAuto->setCheckable(true);
    actAuto->setChecked(m_selectedCodecName.isEmpty());
    actAuto->setData(QString());

    // 常用编码（显示名与 data 分离）
    QAction *actUtf8 = menu.addAction("UTF-8");
    actUtf8->setCheckable(true);
    actUtf8->setChecked(m_selectedCodecName == "UTF-8");
    actUtf8->setData("UTF-8");

    QAction *actGbk = menu.addAction("GBK / GB2312");
    actGbk->setCheckable(true);
    actGbk->setChecked(m_selectedCodecName == "GBK" ||
                       m_selectedCodecName == "GB2312");
    actGbk->setData("GBK");

    menu.addSeparator();

    // 列出系统所有可用编码
    const auto allCodecs = QTextCodec::availableCodecs();
    QSet<QString> seen;
    seen.insert("UTF-8");
    seen.insert("GBK");
    seen.insert("GB2312");
    for (const QByteArray &name : allCodecs) {
      QString codecName = QString::fromUtf8(name);
      if (codecName.isEmpty() || seen.contains(codecName)) continue;
      seen.insert(codecName);
      QAction *act = menu.addAction(codecName);
      act->setCheckable(true);
      act->setChecked(m_selectedCodecName == codecName);
      act->setData(codecName);
    }

    QAction *chosen = menu.exec(m_btnRxEncoding->mapToGlobal(
        QPoint(0, m_btnRxEncoding->height())));

    if (!chosen) return;
    // 使用 data() 获取实际编码名（而非显示文字）
    m_selectedCodecName = chosen->data().toString();
    // 编码变化时重置解码器状态
    delete m_converterState;
    m_converterState = nullptr;
    m_lastCodecName.clear();
  });

  // 初始化浮动控件位置
  rxFloatWidget->adjustSize();
  rxFloatWidget->move(
      qMax(4, m_textReceive->width() - rxFloatWidget->width() - 35),
      qMax(4, m_textReceive->height() - rxFloatWidget->height() - 10));

  dataLayout->addWidget(rxContainer);

  // 将接收区域包装为DockWidget（不显示标题文字）
  m_dockReceive = new QDockWidget(QString(), dataDockHost);
  m_dockReceive->setObjectName("dataDock");
  m_dockReceive->setFeatures(QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable |
                             QDockWidget::DockWidgetClosable);
  m_dockReceive->setAllowedAreas(Qt::AllDockWidgetAreas);
  m_dockReceive->setWidget(grpData);
  m_dockReceive->setStyleSheet(neutralDataDockStyleSheet());
  dataDockHost->addDockWidget(Qt::TopDockWidgetArea, m_dockReceive);

  // --- Send Area Container - 改为DockWidget ---
  QWidget *grpSend = new QWidget();
  grpSend->setObjectName("dockContentWidget");
  QVBoxLayout *grpSendMainLayout = new QVBoxLayout(grpSend);
  grpSendMainLayout->setContentsMargins(10, 10, 10, 10);

  QVBoxLayout *grpSendLayout = new QVBoxLayout();
  grpSendLayout->setContentsMargins(0, 0, 0, 0);
  grpSendMainLayout->addLayout(grpSendLayout);

  // --- Send Area: 4-tab QTabWidget ---
  m_sendTabWidget = new QTabWidget();
  m_sendTabWidget->setDocumentMode(false);
  // 消除标签页面板左上角遮挡条框
  m_sendTabWidget->setStyleSheet(R"(
    QTabWidget::pane {
        border: 1px solid #C8C8C8;
        border-top: none;
        background: transparent;
    }
    QTabWidget::left-corner {
        background: transparent;
        border: none;
        width: 0px;
        height: 0px;
    }
    QTabWidget::right-corner {
        background: transparent;
        border: none;
        width: 0px;
        height: 0px;
    }
    QTabWidget::tab-bar { left: 0px; }
    QTabBar::tab {
        background: #F0F0F0;
        border: 1px solid #C8C8C8;
        border-bottom: none;
        border-top-left-radius: 6px;
        border-top-right-radius: 6px;
        min-width: 70px;
        padding: 4px 12px;
        margin-right: 1px;
    }
    QTabBar::tab:selected {
        background: #FFFFFF;
        font-weight: bold;
        color: #1565C0;
    }
    QTabBar::tab:hover:!selected { background: #E8EDF5; }
    QTabBar::tab:first { margin-left: 0px; }
  )");

  // ========== Tab 0: 单条发送 ==========
  QWidget *tabSingle = new QWidget();
  QVBoxLayout *singleLayout = new QVBoxLayout(tabSingle);
  singleLayout->setContentsMargins(6, 6, 6, 6);

  // We need the Tx icons floating ABOVE the Send text box.
  QWidget *txContainer = new QWidget();
  QVBoxLayout *txContainerLayout = new QVBoxLayout(txContainer);
  txContainerLayout->setContentsMargins(0, 0, 0, 0);

  m_textSend = new QTextEdit();
  m_textSend->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_textSend->setPlaceholderText("输入要发送的数据...");
  m_textSend->installEventFilter(this);

  // Floating widget for right corner (icons)
  QWidget *txFloatRightWidget = new QWidget(m_textSend);
  txFloatRightWidget->setObjectName("txFloatRight");
  txFloatRightWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
  txFloatRightWidget->setStyleSheet("background: transparent; border: none;");
  txFloatRightWidget->raise(); // 确保浮动控件在最上层
  QHBoxLayout *txFloatRightLayout = new QHBoxLayout(txFloatRightWidget);
  txFloatRightLayout->setContentsMargins(0, 0, 0, 0);
  txFloatRightLayout->setSpacing(10);

  m_btnSend = createFloatBtn(QChar(0xE651));
  m_btnSend->setCheckable(false);
  m_btnTxHexToggle = createFloatBtn(QChar(0xEBBC));
  m_btnTxTimeToggle = createFloatBtn(QChar(0xE676));
  m_btnTxClear = createFloatBtn(QChar(0xE621));
  m_btnTxClear->setCheckable(false);

  m_btnTxHexToggle->setToolTip("HEX显示");
  m_btnTxTimeToggle->setToolTip("显示时间");
  m_btnSend->setToolTip("发送数据");
  m_btnTxClear->setToolTip("清空发送区");

  // 悬浮在右下角
  txFloatRightLayout->addWidget(m_btnTxHexToggle);
  txFloatRightLayout->addWidget(m_btnTxTimeToggle);
  txFloatRightLayout->addWidget(m_btnSend);
  txFloatRightLayout->addWidget(m_btnTxClear);

  // Floating widget for left corner (Auto send, new line)
  QWidget *txFloatLeftWidget = new QWidget(m_textSend);
  txFloatLeftWidget->setObjectName("txFloatLeft");
  txFloatLeftWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
  txFloatLeftWidget->setStyleSheet("background: transparent; border: none;");
  txFloatLeftWidget->raise(); // 确保浮动控件在最上层
  QHBoxLayout *txFloatLeftLayout = new QHBoxLayout(txFloatLeftWidget);
  txFloatLeftLayout->setContentsMargins(6, 3, 6, 3);
  txFloatLeftLayout->setSpacing(8);

  m_chkAutoSend = createFloatBtn(QChar(0xE886));
  m_chkAutoSend->setToolTip("自动发送");
  m_chkAutoSend->setEnabled(false);
  m_spinAutoSendInterval = new QSpinBox();
  m_spinAutoSendInterval->setRange(10, 10000);
  m_spinAutoSendInterval->setValue(1000);
  m_spinAutoSendInterval->setSuffix(" ms");
  m_spinAutoSendInterval->setEnabled(false);
  m_spinAutoSendInterval->setStyleSheet(
      "QSpinBox { color: #263238; background: rgba(255,255,255,240);"
      " border: 1px solid #90A4AE; border-radius: 4px; padding: 0 4px; }"
      "QSpinBox:disabled { color: #546E7A; }");
  m_spinAutoSendInterval->hide(); // Only show when checked

  m_chkTxNewLine = createFloatBtn(QChar(0xE888));
  m_chkTxNewLine->setToolTip("发送新行");

  // Keep dock-side and floating controls in sync without overriding members.
  connect(dockChkTxNewLine, &QCheckBox::toggled, m_chkTxNewLine,
          &QToolButton::setChecked);
  connect(m_chkTxNewLine, &QToolButton::toggled, dockChkTxNewLine,
          &QCheckBox::setChecked);
  connect(dockChkAutoSend, &QCheckBox::toggled, m_chkAutoSend,
          &QToolButton::setChecked);
  connect(m_chkAutoSend, &QToolButton::toggled, dockChkAutoSend,
          &QCheckBox::setChecked);
  connect(dockSpinAutoSendInterval, QOverload<int>::of(&QSpinBox::valueChanged),
          m_spinAutoSendInterval, &QSpinBox::setValue);
  connect(m_spinAutoSendInterval, QOverload<int>::of(&QSpinBox::valueChanged),
          dockSpinAutoSendInterval, &QSpinBox::setValue);
  connect(m_btnOpenClose, &QPushButton::toggled, dockChkAutoSend,
          &QCheckBox::setEnabled);
  connect(m_btnOpenClose, &QPushButton::toggled, dockSpinAutoSendInterval,
          &QSpinBox::setEnabled);
  connect(m_btnOpenClose, &QPushButton::toggled, m_chkAutoSend,
          &QToolButton::setEnabled);
  connect(m_btnOpenClose, &QPushButton::toggled, m_spinAutoSendInterval,
          &QSpinBox::setEnabled);

  txFloatLeftLayout->addWidget(m_chkAutoSend);
  txFloatLeftLayout->addWidget(m_chkTxNewLine);
  txFloatLeftLayout->addWidget(m_spinAutoSendInterval);

  // 初始化浮动控件位置（防止被滚动条遮挡，右边距设为 35）
  txFloatRightWidget->adjustSize();
  txFloatRightWidget->move(
      qMax(4, m_textSend->width() - txFloatRightWidget->width() - 35),
      qMax(4, m_textSend->height() - txFloatRightWidget->height() - 10));
  txFloatLeftWidget->adjustSize();
  txFloatLeftWidget->move(
      10, qMax(4, m_textSend->height() - txFloatLeftWidget->height() - 10));

  connect(m_chkAutoSend, &QToolButton::toggled, this, [this](bool checked) {
    m_spinAutoSendInterval->setVisible(checked);
    if (!m_textSend)
      return;

    QWidget *txFloatLeft = m_textSend->findChild<QWidget *>("txFloatLeft");
    if (txFloatLeft) {
      txFloatLeft->adjustSize();
      txFloatLeft->move(
          10, qMax(4, m_textSend->height() - txFloatLeft->height() - 10));
    }
  });

  txContainerLayout->addWidget(m_textSend);

  singleLayout->addWidget(txContainer);

  // Bottom Area (under text box) Layout
  QGridLayout *txBottomLayout = new QGridLayout();
  txBottomLayout->setSpacing(8);

  m_comboHistory = new QComboBox();
  m_comboHistory->setEditable(false);
  m_comboHistory->setMinimumWidth(150);

  // Row 1 of Bottom: History (History ComboBox)
  QHBoxLayout *txHistory = new QHBoxLayout();
  txHistory->addWidget(new QLabel("历史记录:"));
  txHistory->addWidget(m_comboHistory);
  txHistory->addStretch();

  txBottomLayout->addLayout(txHistory, 0, 0);

  singleLayout->addLayout(txBottomLayout);

  m_sendTabWidget->addTab(tabSingle, "单条发送");

  // ========== Tab 1: 多条发送 ==========
  QWidget *tabMulti = new QWidget();
  QVBoxLayout *multiLayout = new QVBoxLayout(tabMulti);
  multiLayout->setContentsMargins(4, 4, 4, 4);
  multiLayout->setSpacing(3);

  // 双列 x 10行 网格 (共20条)
  QGridLayout *multiGrid = new QGridLayout();
  multiGrid->setSpacing(2);
  for (int i = 0; i < MULTI_PER_PAGE; i++) {
    int col = i / MULTI_ROWS;
    int row = i % MULTI_ROWS;
    m_chkMultiItem[i] = new QCheckBox();
    m_chkMultiItem[i]->setFixedWidth(20);
    m_leMultiItem[i] = new QLineEdit();
    m_leMultiItem[i]->setPlaceholderText(QString("条目 %1").arg(i + 1));
    m_leMultiItem[i]->setMinimumWidth(80);
    QHBoxLayout *cell = new QHBoxLayout();
    cell->setContentsMargins(0, 0, 0, 0);
    cell->setSpacing(2);
    cell->addWidget(m_chkMultiItem[i]);
    cell->addWidget(m_leMultiItem[i]);
    QWidget *cellW = new QWidget();
    cellW->setLayout(cell);
    multiGrid->addWidget(cellW, row, col);
  }
  multiLayout->addLayout(multiGrid);

  // ================= 底部工具栏布局 =================
  QVBoxLayout *bottomLayout = new QVBoxLayout();
  bottomLayout->setContentsMargins(0, 4, 0, 0);
  bottomLayout->setSpacing(6);

  // 第一排：选项 与 导入导出/全选
  QHBoxLayout *row1 = new QHBoxLayout();
  m_chkMultiNewLine = new QCheckBox("发送新行");
  m_chkMultiHex = new QCheckBox("16进制");
  m_chkMultiLoop = new QCheckBox("自动循环(ms):");
  m_spinMultiLoopInterval = new QSpinBox();
  m_spinMultiLoopInterval->setRange(10, 60000);
  m_spinMultiLoopInterval->setValue(1000);
  m_spinMultiLoopInterval->setEnabled(false);

  row1->addWidget(m_chkMultiNewLine);
  row1->addWidget(m_chkMultiHex);
  row1->addWidget(m_chkMultiLoop);
  row1->addWidget(m_spinMultiLoopInterval);
  row1->addStretch();

  QPushButton *btnSelAll = new QPushButton("全选");
  QPushButton *btnDeselAll = new QPushButton("全不选");
  m_btnMultiImport = new QPushButton("导入");
  m_btnMultiExport = new QPushButton("导出");
  for (auto *b : {btnSelAll, btnDeselAll, m_btnMultiImport, m_btnMultiExport}) {
    b->setStyleSheet(getButtonStyle(ButtonType::Normal));
    b->setMinimumHeight(28);
    row1->addWidget(b);
  }
  bottomLayout->addLayout(row1);

  // 第二排：分页导航 与 发送按钮
  QHBoxLayout *row2 = new QHBoxLayout();
  m_btnMultiFirst = new QPushButton("|<");
  m_btnMultiPrev = new QPushButton("<");
  m_btnMultiNext = new QPushButton(">");
  m_btnMultiLast = new QPushButton(">|");
  for (auto *b :
       {m_btnMultiFirst, m_btnMultiPrev, m_btnMultiNext, m_btnMultiLast}) {
    b->setFixedWidth(32);
    b->setStyleSheet(getButtonStyle(ButtonType::Normal));
  }
  m_lblMultiPage = new QLabel("第 1 / 1 页");
  m_lblMultiPage->setAlignment(Qt::AlignCenter);

  m_btnMultiAddPage = new QPushButton("+页");
  m_btnMultiDelPage = new QPushButton("-页");
  for (auto *b : {m_btnMultiAddPage, m_btnMultiDelPage}) {
    b->setFixedWidth(40);
    b->setStyleSheet(getButtonStyle(ButtonType::Normal));
  }

  m_spinJumpPage = new QSpinBox();
  m_spinJumpPage->setRange(1, 1);
  m_spinJumpPage->setPrefix("页码: ");
  m_spinJumpPage->setFixedWidth(80);
  QPushButton *btnJump = new QPushButton("GO");
  btnJump->setStyleSheet(getButtonStyle(ButtonType::Normal));
  btnJump->setFixedWidth(36);

  row2->addWidget(m_btnMultiFirst);
  row2->addWidget(m_btnMultiPrev);
  row2->addWidget(m_lblMultiPage);
  row2->addWidget(m_btnMultiNext);
  row2->addWidget(m_btnMultiLast);
  row2->addSpacing(10);
  row2->addWidget(m_btnMultiAddPage);
  row2->addWidget(m_btnMultiDelPage);
  row2->addSpacing(10);
  row2->addWidget(m_spinJumpPage);
  row2->addWidget(btnJump);
  row2->addStretch();

  // 突出发送选中按钮
  m_btnSendAll = new QPushButton("发送选中");
  QString sendStyle = getButtonStyle(ButtonType::Normal);
  // 可选：在基础样式上加一点微调让发送按钮更显眼
  sendStyle += "font-weight: bold;";
  m_btnSendAll->setStyleSheet(sendStyle);
  m_btnSendAll->setMinimumHeight(32);
  m_btnSendAll->setMinimumWidth(100);
  row2->addWidget(m_btnSendAll);

  bottomLayout->addLayout(row2);
  multiLayout->addLayout(bottomLayout);

  connect(btnSelAll, &QPushButton::clicked, [this]() {
    for (int i = 0; i < MULTI_PER_PAGE; i++)
      m_chkMultiItem[i]->setChecked(true);
  });
  connect(btnDeselAll, &QPushButton::clicked, [this]() {
    for (int i = 0; i < MULTI_PER_PAGE; i++)
      m_chkMultiItem[i]->setChecked(false);
  });
  connect(btnJump, &QPushButton::clicked,
          [this]() { onMultiPageChanged(m_spinJumpPage->value() - 1); });

  m_sendTabWidget->addTab(tabMulti, "多条发送");

  // ========== Tab 2: 协议传送 ==========
  QWidget *tabProtocol = new QWidget();
  QVBoxLayout *protoLayout = new QVBoxLayout(tabProtocol);
  protoLayout->setContentsMargins(6, 6, 6, 6);
  QGridLayout *protoGrid = new QGridLayout();
  protoGrid->addWidget(new QLabel("协议名:"), 0, 0);
  protoGrid->addWidget(new QComboBox(), 0, 1);
  protoGrid->addWidget(new QLabel("起始帧:"), 1, 0);
  QLineEdit *leStartFrame = new QLineEdit("AA");
  protoGrid->addWidget(leStartFrame, 1, 1);
  protoGrid->addWidget(new QLabel("数据域:"), 2, 0);
  QLineEdit *leData = new QLineEdit();
  leData->setPlaceholderText("Hex 数据...");
  protoGrid->addWidget(leData, 2, 1);
  protoGrid->addWidget(new QLabel("校验:"), 3, 0);
  QComboBox *cbChecksum = new QComboBox();
  cbChecksum->addItems({"无", "Sum8", "CRC8", "CRC16"});
  protoGrid->addWidget(cbChecksum, 3, 1);
  protoLayout->addLayout(protoGrid);
  QPushButton *btnProtoSend = new QPushButton("发送协议帧");
  btnProtoSend->setStyleSheet(getButtonStyle(ButtonType::Normal));
  btnProtoSend->setMinimumHeight(36);
  protoLayout->addWidget(btnProtoSend);
  protoLayout->addStretch();
  m_sendTabWidget->addTab(tabProtocol, "协议传送");

  // ========== Tab 3: 自定义发送 ==========
  QWidget *tabCustom = new QWidget();
  QVBoxLayout *customLayout = new QVBoxLayout(tabCustom);
  customLayout->setContentsMargins(6, 6, 6, 6);
  QPlainTextEdit *editScript = new QPlainTextEdit();
  editScript->setPlaceholderText(
      "// 自定义发送脚本（预留扩展）\n// 例如：循环发送、条件发送等");
  customLayout->addWidget(editScript);
  QPushButton *btnRunScript = new QPushButton("执行脚本（开发中）");
  btnRunScript->setStyleSheet(getButtonStyle(ButtonType::Normal));
  btnRunScript->setEnabled(false);
  customLayout->addWidget(btnRunScript);
  m_sendTabWidget->addTab(tabCustom, "自定义发送");

  grpSendLayout->addWidget(m_sendTabWidget);

  // 将发送区域包装为DockWidget
  m_dockSend = new QDockWidget(QString(), dataDockHost);
  m_dockSend->setObjectName("dataDock");
  m_dockSend->setFeatures(QDockWidget::DockWidgetMovable |
                          QDockWidget::DockWidgetFloatable |
                          QDockWidget::DockWidgetClosable);
  m_dockSend->setAllowedAreas(Qt::AllDockWidgetAreas);
  m_dockSend->setWidget(grpSend);
  m_dockSend->setStyleSheet(neutralDataDockStyleSheet());
  dataDockHost->addDockWidget(Qt::TopDockWidgetArea, m_dockSend);
  dataDockHost->splitDockWidget(m_dockReceive, m_dockSend, Qt::Vertical);
  dataDockHost->resizeDocks({m_dockReceive, m_dockSend}, {3, 2}, Qt::Vertical);

  // Lastly, add our main internal window to the actual top-level layout
  mainLayout->addWidget(m_innerMainWindow);

  // --- Extended Waveform Page ---
  // We use a QMainWindow to easily support Dock Widgets
  m_waveformPage = new QMainWindow();
  m_waveformPage->setWindowFlags(Qt::Widget); // Embeddable
  m_waveformPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // -- Chart and Scrollbar Container (Central Widget) --
  QWidget *chartContainer = new QWidget();
  QVBoxLayout *chartLayout = new QVBoxLayout(chartContainer);
  chartLayout->setContentsMargins(0, 0, 0, 0);
  chartLayout->setSpacing(0);

  m_customPlot = new QCustomPlot();
  m_scrollbarWaveform = new QScrollBar(Qt::Horizontal);
  m_scrollbarWaveform->setMinimum(0);
  m_scrollbarWaveform->setMaximum(0);

  // Modern Glowing Sci-Fi Scrollbar QSS
  QString scrollbarStyle = R"(
    QScrollBar:horizontal {
        background: transparent;
        height: 12px;
        margin: 2px 0 2px 0;
    }
    QScrollBar::handle:horizontal {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 212, 255, 180), stop:1 rgba(9, 9, 121, 180));
        border-radius: 4px;
        min-width: 40px;
    }
    QScrollBar::handle:horizontal:hover {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 255, 255, 255), stop:1 rgba(0, 150, 255, 255));
    }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
        width: 0px;
        background: none;
    }
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
        background: rgba(30, 30, 40, 100);
        border-radius: 4px;
    }
  )";
  m_scrollbarWaveform->setStyleSheet(scrollbarStyle);

  chartLayout->addWidget(m_customPlot, 1);
  chartLayout->addWidget(m_scrollbarWaveform, 0);

  m_waveformPage->setCentralWidget(chartContainer);

  // m_dockSettings merged into m_dockScopeSettings above.

  m_mainHorizSplitter->addWidget(
      m_waveformPage); // Append to main layout vertically

  // Hide initially
  m_waveformPage->setVisible(false); // Give waveform plenty of space

  // --- Layout Integration ---
  // Directly add content to Tab Widget without Navigation Bar

  // 移除了内部多余的 m_mainTabWidget，直接将主分割器添加到布局中

  // --- Status Bar ---
  QHBoxLayout *statusBarLayout = new QHBoxLayout();
  statusBarLayout->setContentsMargins(5, 2, 5, 2);

  // Status Icon/Text
  m_lblPortInfo = new QLabel("串口关闭");
  m_lblPortInfo->setStyleSheet("color: red; font-weight: bold;");

  // Scrolling Message - wider label for smooth scrolling
  m_lblWelcome = new ScrollingLabel();
  m_lblWelcome->setStyleSheet("color: blue; font-style: italic;");
  // m_lblWelcome->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); // Not
  // needed for custom widget
  m_lblWelcome->setMinimumWidth(400); // Fixed width for scrolling area
  m_lblWelcome->setText(m_welcomeText);

  statusBarLayout->addWidget(m_lblPortInfo);
  statusBarLayout->addStretch();

  // RX/TX Statistics
  QLabel *lblRxText = new QLabel("RX:");
  lblRxText->setStyleSheet("font-weight: bold; color: #555;");
  statusBarLayout->addWidget(lblRxText);
  statusBarLayout->addWidget(m_lblRxCount);
  statusBarLayout->addSpacing(15);

  QLabel *lblTxText = new QLabel("TX:");
  lblTxText->setStyleSheet("font-weight: bold; color: #555;");
  statusBarLayout->addWidget(lblTxText);
  statusBarLayout->addWidget(m_lblTxCount);
  statusBarLayout->addSpacing(20);

  statusBarLayout->addWidget(m_lblWelcome);

  mainLayout->addLayout(statusBarLayout);
}

void SerialSession::setupChart() {
  // QCustomPlot Setup
  m_customPlot->xAxis->setLabel("Time");
  m_customPlot->yAxis->setLabel("Value");

  m_customPlot->legend->setVisible(true);

  // Context Menu Policy
  m_customPlot->setContextMenuPolicy(Qt::CustomContextMenu);

  // 图例双击显示/隐藏逻辑
  connect(m_customPlot, &QCustomPlot::legendDoubleClick, this,
          [this](QCPLegend *legend, QCPAbstractLegendItem *item,
                 QMouseEvent *event) {
            Q_UNUSED(legend);
            Q_UNUSED(event);
            if (item) {
              QCPPlottableLegendItem *plItem =
                  qobject_cast<QCPPlottableLegendItem *>(item);
              if (plItem) {
                bool visible = plItem->plottable()->visible();
                plItem->plottable()->setVisible(!visible);
                m_customPlot->replot();
              }
            }
          });

  // Interactions: Scroll and Zoom
  m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom |
                                QCP::iSelectLegend | QCP::iSelectPlottables);

  // Performance Optimizations for Large Data
  m_customPlot->setNoAntialiasingOnDrag(
      true); // Disable AA during heavy interactions
  m_customPlot->setAntialiasedElements(QCP::aePlottables | QCP::aeAxes |
                                       QCP::aeGrid);

  // Initial Range
  m_customPlot->xAxis->setRange(0, 100);
  m_customPlot->yAxis->setRange(0, 255);

  m_customPlot->setVisible(true);

  // High-Tech Neon Styling
  // 1. Dark glowing gradient background
  QLinearGradient plotGradient;
  plotGradient.setStart(0, 0);
  plotGradient.setFinalStop(0, 350);
  plotGradient.setColorAt(0, QColor("#1E1E28"));
  plotGradient.setColorAt(1, QColor("#282836"));
  m_customPlot->setBackground(plotGradient);

  // Set axis rectangle background to slightly darker translucent
  m_customPlot->axisRect()->setBackground(QColor(10, 10, 15, 180));

  // 2. Glowing cyan/blue axes
  QPen axisPen(QColor("#00E5FF"), 2);
  m_customPlot->xAxis->setBasePen(axisPen);
  m_customPlot->yAxis->setBasePen(axisPen);
  m_customPlot->xAxis->setTickPen(axisPen);
  m_customPlot->yAxis->setTickPen(axisPen);
  m_customPlot->xAxis->setSubTickPen(QPen(QColor("#00B0FF"), 1));
  m_customPlot->yAxis->setSubTickPen(QPen(QColor("#00B0FF"), 1));

  // Axis Labels and Tick Labels
  m_customPlot->xAxis->setTickLabelColor(QColor("#E0E0FF"));
  m_customPlot->yAxis->setTickLabelColor(QColor("#E0E0FF"));
  m_customPlot->xAxis->setLabelColor(QColor("#00E5FF"));
  m_customPlot->yAxis->setLabelColor(QColor("#00E5FF"));

  QFont tickFont = font();
  tickFont.setPointSize(9);
  m_customPlot->xAxis->setTickLabelFont(tickFont);
  m_customPlot->yAxis->setTickLabelFont(tickFont);

  QFont labelFont = font();
  labelFont.setPointSize(11);
  labelFont.setBold(true);
  m_customPlot->xAxis->setLabelFont(labelFont);
  m_customPlot->yAxis->setLabelFont(labelFont);

  // 3. Subtle Dashed Grids (Dark Cyan)
  m_customPlot->xAxis->grid()->setPen(
      QPen(QColor(0, 150, 200, 50), 1, Qt::DashLine));
  m_customPlot->yAxis->grid()->setPen(
      QPen(QColor(0, 150, 200, 50), 1, Qt::DashLine));
  m_customPlot->xAxis->grid()->setSubGridVisible(true);
  m_customPlot->yAxis->grid()->setSubGridVisible(true);
  m_customPlot->xAxis->grid()->setSubGridPen(
      QPen(QColor(0, 150, 200, 20), 1, Qt::DotLine));
  m_customPlot->yAxis->grid()->setSubGridPen(
      QPen(QColor(0, 150, 200, 20), 1, Qt::DotLine));

  // Axes Arrows
  m_customPlot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
  m_customPlot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);

  m_customPlot->legend->setTextColor(QColor("#00E5FF"));

  // Floating Play/Pause Button overlay
  m_btnFloatingPlay =
      new QToolButton(m_customPlot); // Parent is customPlot so it draws on top
  m_btnFloatingPlay->setFont(CIconFont::instance()->getIconFont(64));
  m_btnFloatingPlay->setText(QChar(0xE719)); // e719: Play Icon
  m_btnFloatingPlay->setFixedSize(120, 120);
  m_btnFloatingPlay->setCursor(Qt::PointingHandCursor);
  m_btnFloatingPlay->setStyleSheet("QToolButton {"
                                   "   color: rgba(255, 255, 255, 150);"
                                   "   background: transparent;"
                                   "   border: none;"
                                   "}"
                                   "QToolButton:hover {"
                                   "   color: rgba(0, 229, 255, 220);"
                                   "   background: rgba(255, 255, 255, 10);"
                                   "   border-radius: 60px;"
                                   "}");

  // Install event filter to track customPlot resize and hover
  m_customPlot->installEventFilter(this);
}

void SerialSession::applyChartTheme(int index) {
  QColor tickLabelColor, labelColor, gridColor, subGridColor;
  QPen gridPen, subGridPen, axisPen;
  QFont tickFont = font();
  tickFont.setPointSize(9);
  QFont labelFont = font();
  labelFont.setPointSize(11);
  labelFont.setBold(true);

  if (index == 0) {
    // 0: Light Theme
    m_customPlot->setBackground(Qt::white);
    m_customPlot->axisRect()->setBackground(Qt::white);

    axisPen = QPen(Qt::black, 1);
    tickLabelColor = Qt::black;
    labelColor = Qt::black;

    gridPen = QPen(Qt::lightGray, 1, Qt::DashLine);
    subGridPen = QPen(QColor(230, 230, 230), 1, Qt::DotLine);

    m_customPlot->legend->setBrush(QColor(255, 255, 255, 200));
    m_customPlot->legend->setBorderPen(QPen(Qt::lightGray));
    m_customPlot->legend->setTextColor(Qt::black);
  } else if (index == 1) {
    // 1: Neon Theme (Dark)
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor("#1E1E28"));
    plotGradient.setColorAt(1, QColor("#282836"));
    m_customPlot->setBackground(plotGradient);
    m_customPlot->axisRect()->setBackground(QColor(10, 10, 15, 180));

    axisPen = QPen(QColor("#00E5FF"), 2);
    tickLabelColor = QColor("#E0E0FF");
    labelColor = QColor("#00E5FF");

    gridPen = QPen(QColor(0, 150, 200, 50), 1, Qt::DashLine);
    subGridPen = QPen(QColor(0, 150, 200, 20), 1, Qt::DotLine);

    m_customPlot->legend->setBrush(QColor(20, 20, 30, 150));
    m_customPlot->legend->setBorderPen(Qt::NoPen);
    m_customPlot->legend->setTextColor(QColor("#00E5FF"));
  } else if (index == 2) {
    // 2: Sci-Fi Oscilloscope Theme (Intense Green/Cyan on Deep Black)
    m_customPlot->setBackground(Qt::black);
    m_customPlot->axisRect()->setBackground(QColor(0, 5, 0, 255));

    axisPen = QPen(QColor("#00FF41"), 2); // Matrix Green
    tickLabelColor = QColor("#00FF41");
    labelColor = QColor("#00FF41");

    gridPen =
        QPen(QColor(0, 255, 65, 80), 1, Qt::SolidLine); // More visible grid
    subGridPen =
        QPen(QColor(0, 255, 65, 30), 1, Qt::DotLine); // CRT-like secondary grid

    m_customPlot->legend->setBrush(QColor(0, 20, 0, 180));
    m_customPlot->legend->setBorderPen(QPen(QColor("#00FF41")));
    m_customPlot->legend->setTextColor(QColor("#00FF41"));
  }

  // Apply Axes styles
  m_customPlot->xAxis->setBasePen(axisPen);
  m_customPlot->yAxis->setBasePen(axisPen);
  m_customPlot->xAxis->setTickPen(axisPen);
  m_customPlot->yAxis->setTickPen(axisPen);
  m_customPlot->xAxis->setSubTickPen(QPen(axisPen.color(), 1));
  m_customPlot->yAxis->setSubTickPen(QPen(axisPen.color(), 1));

  m_customPlot->xAxis->setTickLabelColor(tickLabelColor);
  m_customPlot->yAxis->setTickLabelColor(tickLabelColor);
  m_customPlot->xAxis->setLabelColor(labelColor);
  m_customPlot->yAxis->setLabelColor(labelColor);

  m_customPlot->xAxis->setTickLabelFont(tickFont);
  m_customPlot->yAxis->setTickLabelFont(tickFont);
  m_customPlot->xAxis->setLabelFont(labelFont);
  m_customPlot->yAxis->setLabelFont(labelFont);

  // Apply Grid styles
  m_customPlot->xAxis->grid()->setPen(gridPen);
  m_customPlot->yAxis->grid()->setPen(gridPen);
  m_customPlot->xAxis->grid()->setSubGridVisible(true);
  m_customPlot->yAxis->grid()->setSubGridVisible(true);
  m_customPlot->xAxis->grid()->setSubGridPen(subGridPen);
  m_customPlot->yAxis->grid()->setSubGridPen(subGridPen);

  m_customPlot->replot();
}

void SerialSession::setupConnections() {
  connect(m_btnRefresh, &QPushButton::clicked, this,
          &SerialSession::refreshPorts);
  connect(m_btnOpenClose, &QPushButton::clicked, this,
          &SerialSession::openClosePort);
  connect(m_serial, &QSerialPort::readyRead, this, &SerialSession::onReadyRead);
  connect(m_serial, &QSerialPort::errorOccurred, this,
          &SerialSession::onPortError);

  connect(m_leftTabWidget, &VerticalTabWidget::currentChanged, this,
          [this](int index) {
            if (m_leftTabWidget) {
              if (index == m_designerTabIndex) {
                m_leftTabWidget->setMinimumWidth(0);
                m_leftTabWidget->setMaximumWidth(QWIDGETSIZE_MAX);
              } else {
                m_leftTabWidget->setMinimumWidth(400);
                m_leftTabWidget->setMaximumWidth(600);
              }
            }

            if (index == 0) {
              // 串口设置：显示收发区，隐藏波形图
              if (m_dataSplitter) {
                m_dataSplitter->setVisible(!m_chkHideRxTx->isChecked());
              }
              m_dockReceive->show();
              m_dockSend->show();
              if (m_waveformPage) {
                m_waveformPage->setVisible(false);
              }
            } else if (index == 1) {
              // 示波器：隐藏收发区，默认打开波形
              if (m_dataSplitter) {
                m_dataSplitter->setVisible(false);
              }
              m_dockReceive->hide();
              m_dockSend->hide();
              m_chkEnableWaveform->setChecked(true);
              if (m_waveformPage) {
                m_waveformPage->setVisible(true);
              }
            } else if (index == m_designerTabIndex) {
              // 控件设计器：隐藏收发区与波形
              if (m_dataSplitter) {
                m_dataSplitter->setVisible(false);
              }
              m_dockReceive->hide();
              m_dockSend->hide();
              if (m_waveformPage) {
                m_waveformPage->setVisible(false);
              }
            } else {
              if (m_dataSplitter) {
                m_dataSplitter->setVisible(!m_chkHideRxTx->isChecked());
              }
              if (m_waveformPage) {
                m_waveformPage->setVisible(m_chkEnableWaveform->isChecked());
              }
            }
          });

  connect(m_btnRxClear, &QToolButton::clicked, this,
          &SerialSession::clearReceiveArea);
  connect(m_btnSend, &QToolButton::clicked, this, &SerialSession::sendData);

  // Send Area Connections
  connect(m_btnTxClear, &QToolButton::clicked,
          [this]() { m_textSend->clear(); });
  connect(m_comboHistory, QOverload<int>::of(&QComboBox::activated),
          [this](int index) {
            if (index >= 0)
              m_textSend->setText(m_comboHistory->itemText(index));
          });

  connect(m_chkAutoSend, &QToolButton::toggled, [this](bool checked) {
    if (checked) {
      m_spinAutoSendInterval->show();
    } else {
      m_spinAutoSendInterval->hide();
    }
    toggleAutoSend(checked);
  });
  connect(m_spinAutoSendInterval, QOverload<int>::of(&QSpinBox::valueChanged),
          [this](int interval) {
            if (m_autoSendTimer->isActive()) {
              m_autoSendTimer->setInterval(interval);
            }
          });

  // Floating Action Toggles
  connect(m_btnRxHexToggle, &QToolButton::toggled, [this](bool checked) {
    m_rbRxHex->setChecked(checked);
    m_rbRxAscii->setChecked(!checked);
  });
  connect(m_btnTxHexToggle, &QToolButton::toggled, [this](bool checked) {
    m_rbTxHex->setChecked(checked);
    m_rbTxAscii->setChecked(!checked);
  });
  connect(m_btnRxTimeToggle, &QToolButton::toggled,
          [this](bool checked) { m_chkRxTime->setChecked(checked); });
  connect(m_btnTxTimeToggle, &QToolButton::toggled,
          [this](bool checked) { m_chkTxTime->setChecked(checked); });
  connect(m_chkTxTime, &QCheckBox::toggled, m_btnTxTimeToggle,
          &QToolButton::setChecked);
  // Connect the floating play/pause to the logical pause variable
  connect(m_btnRxPauseToggle, &QToolButton::toggled, [this](bool paused) {
    m_btnStopRx->setChecked(paused);
    if (paused) {
      m_btnRxPauseToggle->setText(QChar(0xE7D8)); // Pause icon
    } else {
      m_btnRxPauseToggle->setText(QChar(0xE617)); // Play icon
    }
  });
  connect(m_autoSendTimer, &QTimer::timeout, this,
          &SerialSession::onAutoSendTimeout);

  // Auto-convert TX input when switching ASCII <-> HEX
  connect(m_rbTxHex, &QRadioButton::toggled, this,
          &SerialSession::onTxModeChanged);

  // Multi-send tab button connections
  connect(m_btnMultiFirst, &QPushButton::clicked,
          [this]() { onMultiPageChanged(0); });
  connect(m_btnMultiPrev, &QPushButton::clicked,
          [this]() { onMultiPageChanged(m_multiPage - 1); });
  connect(m_btnMultiNext, &QPushButton::clicked,
          [this]() { onMultiPageChanged(m_multiPage + 1); });
  connect(m_btnMultiLast, &QPushButton::clicked,
          [this]() { onMultiPageChanged(m_multiPages.count() - 1); });
  connect(m_btnMultiAddPage, &QPushButton::clicked, [this]() {
    m_multiPages.append(QVector<MultiSendItem>(MULTI_PER_PAGE));
    int newPage = m_multiPages.count() - 1;
    m_spinJumpPage->setRange(1, m_multiPages.count());
    onMultiPageChanged(newPage);
  });
  // Scope Settings Toggles
  connect(m_chkHideRxTx, &QCheckBox::toggled, [this](bool checked) {
    const int index = m_leftTabWidget ? m_leftTabWidget->currentIndex() : -1;
    if (!m_dataSplitter) {
      return;
    }
    if (index == 0) {
      m_dataSplitter->setVisible(!checked);
    } else if (index == 1 || index == m_designerTabIndex) {
      m_dataSplitter->setVisible(false);
    } else {
      m_dataSplitter->setVisible(!checked);
    }
  });
  connect(m_chkHideRxData, &QCheckBox::toggled,
          [this](bool checked) { m_textReceive->setVisible(!checked); });

  connect(m_btnMultiDelPage, &QPushButton::clicked, [this]() {
    if (m_multiPages.count() <= 1)
      return;
    m_multiPages.removeAt(m_multiPage);
    m_spinJumpPage->setRange(1, m_multiPages.count());
    onMultiPageChanged(qMin(m_multiPage, m_multiPages.count() - 1));
  });
  connect(m_btnSendAll, &QPushButton::clicked, this,
          &SerialSession::sendSelectedMulti);
  connect(m_btnMultiImport, &QPushButton::clicked, this,
          &SerialSession::importMultiData);
  connect(m_btnMultiExport, &QPushButton::clicked, this,
          &SerialSession::exportMultiData);
  connect(m_chkMultiLoop, &QCheckBox::toggled, [this](bool on) {
    m_spinMultiLoopInterval->setEnabled(on);
    if (on) {
      m_multiLoopIndex = 0;
      m_multiLoopTimer->start(m_spinMultiLoopInterval->value());
    } else {
      m_multiLoopTimer->stop();
    }
  });
  connect(m_multiLoopTimer, &QTimer::timeout, this,
          &SerialSession::onMultiSendLoop);
  connect(m_spinMultiLoopInterval, QOverload<int>::of(&QSpinBox::valueChanged),
          [this](int v) {
            if (m_multiLoopTimer->isActive())
              m_multiLoopTimer->setInterval(v);
          });
  // Save grid edits back to model when focus leaves a cell
  for (int i = 0; i < MULTI_PER_PAGE; i++) {
    connect(m_chkMultiItem[i], &QCheckBox::toggled, [this, i](bool v) {
      if (m_multiPage < m_multiPages.count())
        m_multiPages[m_multiPage][i].enabled = v;
    });
    connect(m_leMultiItem[i], &QLineEdit::editingFinished, [this, i]() {
      if (m_multiPage < m_multiPages.count())
        m_multiPages[m_multiPage][i].content = m_leMultiItem[i]->text();
    });
  }

  connect(m_spinPoints, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, [this](double value) {
            const int newViewWidthPoints = displayWidthToPoints(value);
            if (newViewWidthPoints != m_viewWidthPoints) {
              m_viewWidthPoints = newViewWidthPoints;
              updateChartSettings();
            }
            refreshViewWidthSpin();
          });
  connect(m_chkShowGrid, &QCheckBox::toggled, this,
          &SerialSession::updateChartSettings);
  connect(m_btnAutoScale, &QPushButton::toggled, this,
          &SerialSession::updateChartSettings);
  connect(m_spinYMin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &SerialSession::updateChartSettings);
  connect(m_portCheckTimer, &QTimer::timeout, this, &SerialSession::checkPorts);

  connect(m_spinYMax, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &SerialSession::updateChartSettings);
  connect(m_spinYTick, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &SerialSession::updateChartSettings);

  connect(m_btnClearWaveform, &QPushButton::clicked, [this]() {
    for (int i = 0; i < m_customPlot->graphCount(); ++i) {
      if (m_customPlot->graph(i) && m_customPlot->graph(i)->data()) {
        m_customPlot->graph(i)->data()->clear();
      }
    }
    m_xValue = 0;
    m_scrollbarWaveform->setMinimum(0);
    m_scrollbarWaveform->setMaximum(0);
    m_scrollbarWaveform->setValue(0);
    updateXAxisRange();
    m_customPlot->replot();
  });

  connect(m_btnResetChart, &QPushButton::clicked, [this]() {
    m_viewWidthPoints = 100;
    refreshViewWidthSpin();
    m_btnAutoScale->setChecked(true);
    m_chkShowGrid->setChecked(true);
    m_spinYTick->setValue(0);
    updateChartSettings();
  });

  connect(m_replotTimer, &QTimer::timeout, this,
          &SerialSession::onReplotTimeout);

  connect(m_scrollbarWaveform, &QScrollBar::valueChanged, this,
          &SerialSession::onWaveformScroll);
  connect(m_comboTimeUnit, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SerialSession::onTimeUnitChanged);
  connect(
      m_spinSampleInterval,
      QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
      [this](double) { onTimeUnitChanged(m_comboTimeUnit->currentIndex()); });
  connect(m_comboChartTheme,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SerialSession::onChartThemeChanged);
  connect(m_spinBufferLimit, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int val) {
            if (m_viewWidthPoints > val) {
              m_viewWidthPoints = val;
            }
            refreshViewWidthSpin();
            updateChartSettings();
          });

  connect(m_editFrameHeader, &QLineEdit::textChanged, this,
          [this](const QString &text) {
            m_frameHeader = parseFrameMarker(text);
            m_rxBuffer.clear();
            refreshFramePreview();
          });
  connect(m_editFrameTail, &QLineEdit::textChanged, this,
          [this](const QString &text) {
            m_frameTail = parseFrameMarker(text);
            m_rxBuffer.clear();
            refreshFramePreview();
          });
  connect(m_chkStrictFrame, &QCheckBox::toggled, this, [this](bool) {
    m_rxBuffer.clear();
    refreshFramePreview();
  });
  refreshFramePreview();

  // 注释掉DockWidget相关的连接，因为已改为标签页
  // connect(m_dockScopeSettings, &QDockWidget::dockLocationChanged, this,
  //         &SerialSession::onDockLocationChanged);

  // 波形显示控制
  connect(m_chkEnableWaveform, &QCheckBox::toggled, this,
          &SerialSession::onWaveformEnabled);

  // 图表右键菜单
  connect(m_customPlot, &QCustomPlot::customContextMenuRequested, this,
          &SerialSession::onChartContextMenu);

  // 曲线设置按钮
  connect(m_btnCurveSettings, &QPushButton::clicked, this,
          &SerialSession::onCurveSettingsClicked);

  // Floating Play Pause Logic Integration
  connect(m_btnFloatingPlay, &QToolButton::clicked, [this]() {
    m_btnStopWaveform->setChecked(!m_btnStopWaveform->isChecked());
  });

  connect(m_btnStopWaveform, &QPushButton::toggled, [this](bool checked) {
    if (checked) {
      m_btnStopWaveform->setText("\ue87d");
      m_btnFloatingPlay->setText(QChar(0xE719));
      m_btnFloatingPlay->show();
    } else {
      m_btnStopWaveform->setText("\ue87c");
      m_btnFloatingPlay->hide();
    }
  });

  m_btnStopWaveform->setChecked(true);
  onTimeUnitChanged(m_comboTimeUnit->currentIndex());
}

void SerialSession::refreshFramePreview() {
  if (!m_lblFramePreview) {
    return;
  }

  const bool validTail = !m_frameTail.isEmpty();
  const bool strict = m_chkStrictFrame && m_chkStrictFrame->isChecked();

  const QString warnStyle =
      "QLabel { background-color: #FFEBEE; color: #C62828; border: 1px solid "
      "#EF9A9A; border-radius: 8px; font-weight: 600; font-size: 12px; padding: 6px 10px; line-height: 1.3; }";
  const QString okStyle =
      "QLabel { background-color: #E8F5E9; color: #1B5E20; border: 1px solid "
      "#81C784; border-radius: 8px; font-weight: 600; font-size: 12px; padding: 6px 10px; line-height: 1.3; }";
  const QString looseStyle =
      "QLabel { background-color: #FFF8E1; color: #7B5A00; border: 1px solid "
      "#FFCC80; border-radius: 8px; font-weight: 600; font-size: 12px; padding: 6px 10px; line-height: 1.3; }";

  if (!validTail) {
    m_lblFramePreview->setStyleSheet(warnStyle);
    m_lblFramePreview->setText("帧尾为空，当前无法匹配任何数据帧");
    return;
  }

  const QString headerView =
      m_frameHeader.isEmpty() ? QStringLiteral("(任意)")
                              : byteArrayHexView(m_frameHeader);
  const QString tailView = byteArrayHexView(m_frameTail);
  const QString pattern = QString("%1 │ Payload │ %2").arg(headerView, tailView);

  if (strict) {
    m_lblFramePreview->setStyleSheet(okStyle);
    m_lblFramePreview->setText(
        QString("严格匹配：%1\n仅完整匹配的合法数据帧参与绘图").arg(pattern));
  } else {
    m_lblFramePreview->setStyleSheet(looseStyle);
    m_lblFramePreview->setText(
        QString("宽松匹配：%1\n允许缺失帧头的数据帧入图").arg(pattern));
  }
}

QString SerialSession::byteArrayHexView(const QByteArray &data) const {
  if (data.isEmpty()) {
    return QStringLiteral("(空)");
  }

  QStringList tokens;
  tokens.reserve(data.size());
  for (char rawByte : data) {
    const unsigned char byte = static_cast<unsigned char>(rawByte);
    switch (byte) {
    case '\n':
      tokens << QStringLiteral("\\n");
      break;
    case '\r':
      tokens << QStringLiteral("\\r");
      break;
    case '\t':
      tokens << QStringLiteral("\\t");
      break;
    default:
      if (byte >= 0x20 && byte < 0x7F) {
        tokens << QChar(static_cast<char>(byte));
      } else {
        tokens << QString("\\x%1").arg(byte, 2, 16, QLatin1Char('0')).toUpper();
      }
      break;
    }
  }
  return tokens.join(QString());
}

void SerialSession::onCurveSettingsClicked() {
  CurveSettingsDialog dialog(m_customPlot, this);
  dialog.exec();
}

void SerialSession::refreshPorts() {
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

void SerialSession::checkPorts() {
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

void SerialSession::openClosePort() {
  if (m_serial->isOpen()) {
    m_serial->close(); // close once — duplicate calls caused state corruption
    m_welcomeText =
        "   欢迎使用uSmilePro串口示波器V1.0   "; // Revert to default
    m_lblWelcome->setText(m_welcomeText);
    ToastWidget::showToast("串口 " + m_serial->portName() + " 已关闭", false,
                           this);
    m_btnOpenClose->setText(QChar(0xe84e));
    m_btnOpenClose->setChecked(false);
    m_lblStatusIcon->setPixmap(
        QPixmap(":/icons/ONOFF/OFF5.png")
            .scaled(100, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // 更新垂直标签页的串口设置图标为关闭状态
    if (m_leftTabWidget) {
      m_leftTabWidget->updateTabIcon(0, QString(QChar(0xe88c)));
    }

    updateStatusInfo();
    // Disable auto-send
    m_chkAutoSend->setChecked(false); // triggers toggleAutoSend -> stops timer
    m_chkAutoSend->setEnabled(false);
    m_spinAutoSendInterval->setEnabled(false);
    // Enable port settings
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
      m_rxDecoderBuffer.clear();
      m_rxLineBuffer.clear();
      // 重置增量解码状态
      delete m_converterState;
      m_converterState = nullptr;
      m_lastCodecName.clear();
      m_welcomeText = "   串口 " + m_serial->portName() +
                      " 已打开   "; // Change text for scroll
      m_lblWelcome->setText(m_welcomeText);
      ToastWidget::showToast("串口 " + m_serial->portName() + " 已打开", true,
                             this);

      m_btnOpenClose->setText(QChar(0xe855));
      m_btnOpenClose->setChecked(true);
      m_lblStatusIcon->setPixmap(
          QPixmap(":/icons/ONOFF/ON2.png")
              .scaled(100, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));

      // 更新垂直标签页的串口设置图标为打开状态
      if (m_leftTabWidget) {
        m_leftTabWidget->updateTabIcon(0, QString(QChar(0xe88f)));
      }

      updateStatusInfo();
      // Enable auto-send controls
      m_chkAutoSend->setEnabled(true);
      // Disable port settings
      m_comboPort->setEnabled(false);
      m_comboBaud->setEnabled(false);
      m_comboDataBits->setEnabled(false);
      m_comboParity->setEnabled(false);
      m_comboStopBits->setEnabled(false);
    } else {
      QString errorStr = m_serial->errorString();
      QTimer::singleShot(0, this, [this, errorStr]() {
        QMessageBox::critical(this, "错误", "无法打开串口:\n" + errorStr);
      });
      m_btnOpenClose->setChecked(false);
      m_btnOpenClose->setText(QChar(0xe88f));
      m_btnOpenClose->setToolTip("打开串口");
    }
  }
}

void SerialSession::onPortError(QSerialPort::SerialPortError error) {
  if (error == QSerialPort::ResourceError) {
    // Defer dialog + close to next event loop iteration to avoid re-entrant
    // signal handling (calling QMessageBox directly from a serial error signal
    // can cause recursive event-loop processing and crash).
    QTimer::singleShot(0, this, [this]() {
      QMessageBox::critical(this, "严重错误", "串口连接中断！");
      openClosePort();
    });
  }
}

void SerialSession::onReadyRead() {
  QByteArray data = m_serial->readAll();
  m_rxCount += data.size();
  m_lblRxCount->setText(QString::number(m_rxCount));

  // 将数据转发给控件设计器中的组件
  if (m_widgetDesigner) {
    emit m_widgetDesigner->scopeDataReceived(data);
    emit m_widgetDesigner->ledDataReceived(data);
  }

  if (!m_btnStopRx->isChecked()) {
    const bool isHex = m_rbRxHex->isChecked();
    const bool logMode = m_chkRxLog->isChecked();
    const bool showTimestamp = m_chkRxTime->isChecked() || logMode;
    const QString rxTag = logMode ? "[LOG][RX]" : "[RX]";

    if (isHex) {
      // Hex 接收模式
      QString hexStr = data.toHex(' ').toUpper();
      if (!hexStr.isEmpty()) {
        hexStr += ' ';
      }

      if (!m_chkHideRxData->isChecked() && !m_chkShowRawData->isChecked()) {
        if (showTimestamp) {
          QString timeStr =
              QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
          QString htmlLine =
              QString("<span style='color:red;'>%1</span><span>%2 %3</span>")
                  .arg(timeStr.toHtmlEscaped())
                  .arg(rxTag)
                  .arg(hexStr.toHtmlEscaped());
          m_textReceive->append(htmlLine);
        } else {
          QTextCursor cursor = m_textReceive->textCursor();
          cursor.movePosition(QTextCursor::End);
          cursor.insertText(hexStr);
          m_textReceive->setTextCursor(cursor);
        }
        m_textReceive->verticalScrollBar()->setValue(
            m_textReceive->verticalScrollBar()->maximum());
      }
    } else {
      // ASCII / 文本 接收模式
      m_rxDecoderBuffer.append(data);

      // 获取当前选择的编码
      const QString selectedCodec = m_selectedCodecName;
      QByteArray validBytes;
      QString rawStr;

      if (selectedCodec.isEmpty()) {
        // ====== 自动检测模式：UTF-8 / GBK 不完整字节检测 ======
        int len = m_rxDecoderBuffer.size();
        int incompleteCount = 0;

        // 1. 检查 UTF-8 末尾不完整字节 (最多回溯 3 字节)
        for (int i = 1; i <= qMin(4, len); ++i) {
          unsigned char uc =
              static_cast<unsigned char>(m_rxDecoderBuffer.at(len - i));
          if ((uc & 0x80) == 0) {
            break;
          }
          if ((uc & 0xE0) == 0xC0) {
            if (i < 2) incompleteCount = i;
            break;
          }
          if ((uc & 0xF0) == 0xE0) {
            if (i < 3) incompleteCount = i;
            break;
          }
          if ((uc & 0xF8) == 0xF0) {
            if (i < 4) incompleteCount = i;
            break;
          }
        }

        // 2. 如果 UTF-8 未发现不完整字节，检查 GBK
        if (incompleteCount == 0 && len > 0) {
          unsigned char lastByte =
              static_cast<unsigned char>(m_rxDecoderBuffer.at(len - 1));
          if (lastByte >= 0x81 && lastByte <= 0xFE) {
            int highByteCount = 0;
            for (int i = len - 1; i >= 0; --i) {
              unsigned char b =
                  static_cast<unsigned char>(m_rxDecoderBuffer.at(i));
              if (b >= 0x80) highByteCount++;
              else break;
            }
            if (highByteCount % 2 != 0) {
              incompleteCount = 1;
            }
          }
        }

        validBytes = m_rxDecoderBuffer.left(len - incompleteCount);
        m_rxDecoderBuffer = m_rxDecoderBuffer.right(incompleteCount);

        if (!validBytes.isEmpty()) {
          rawStr = QString::fromUtf8(validBytes);
          if (rawStr.contains(QChar(0xFFFD))) {
            QString gbkStr = QString::fromLocal8Bit(validBytes);
            if (!gbkStr.contains(QChar(0xFFFD))) {
              rawStr = gbkStr;
            }
          }
        }
      } else {
        // ====== 手动编码模式：QTextCodec::ConverterState 增量解码 ======
        QTextCodec *codec = QTextCodec::codecForName(selectedCodec.toUtf8());
        if (codec) {
          // 编码变化时重置状态
          if (m_lastCodecName != selectedCodec) {
            delete m_converterState;
            m_converterState = new QTextCodec::ConverterState(
                QTextCodec::ConvertInvalidToNull);
            m_lastCodecName = selectedCodec;
            m_rxDecoderBuffer.clear();
          }
          if (!m_converterState) {
            m_converterState = new QTextCodec::ConverterState(
                QTextCodec::ConvertInvalidToNull);
            m_lastCodecName = selectedCodec;
          }
          // ConverterState 内部自动缓冲不完整字节，传入全部数据即可
          QByteArray allData = m_rxDecoderBuffer;
          m_rxDecoderBuffer.clear();
          rawStr = codec->toUnicode(allData.constData(), allData.size(),
                                    m_converterState);
          validBytes = allData;
        } else {
          // 未找到编码，回退 UTF-8
          validBytes = m_rxDecoderBuffer;
          m_rxDecoderBuffer.clear();
          rawStr = QString::fromUtf8(validBytes);
        }
      }

      if (!validBytes.isEmpty()) {

        // 统一换行符
        rawStr.replace("\r\n", "\n");
        rawStr.replace('\r', '\n');

        if (!m_chkHideRxData->isChecked() && !m_chkShowRawData->isChecked()) {
          if (showTimestamp) {
            // 时间戳/日志模式：按行拆分，只有遇到 '\n' 才输出一行带有时间戳的记录
            m_rxLineBuffer.append(rawStr);
            int lineBreakPos = -1;
            while ((lineBreakPos = m_rxLineBuffer.indexOf('\n')) >= 0) {
              QString lineStr = m_rxLineBuffer.left(lineBreakPos);
              m_rxLineBuffer.remove(0, lineBreakPos + 1);

              QString timeStr =
                  QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
              // 转换 HTML 特殊字符并保留多空格与 Tab
              QString escapedContent =
                  lineStr.toHtmlEscaped()
                      .replace("  ", " &nbsp;")
                      .replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
              QString htmlLine =
                  QString("<span style='color:red;'>%1</span><span>%2 %3</span>")
                      .arg(timeStr.toHtmlEscaped())
                      .arg(rxTag)
                      .arg(escapedContent);
              m_textReceive->append(htmlLine);
            }
          } else {
            // 非时间戳模式：标准终端无损流式输出 (保留空格、Tab、换行、段落缩进)
            QTextCursor cursor = m_textReceive->textCursor();
            cursor.movePosition(QTextCursor::End);
            cursor.insertText(rawStr);
            m_textReceive->setTextCursor(cursor);
          }
          m_textReceive->verticalScrollBar()->setValue(
              m_textReceive->verticalScrollBar()->maximum());
        }
      }
    }
  }

  updateWaveform(data);
}

void SerialSession::onWaveformEnabled(bool checked) {
  const bool inDesigner = m_leftTabWidget && (m_leftTabWidget->currentIndex() ==
                                              m_designerTabIndex);
  m_waveformPage->setVisible(checked && !inDesigner);
  if (checked && !inDesigner) {
    m_customPlot->replot();
  }
}

void SerialSession::onDockLocationChanged(Qt::DockWidgetArea area) {
  if (area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea) {
    m_settingsLayout->setDirection(QBoxLayout::LeftToRight);
  } else {
    m_settingsLayout->setDirection(QBoxLayout::TopToBottom);
  }
}

double SerialSession::currentTimeUnitScale() const {
  const double sampleIntervalMs =
      m_spinSampleInterval ? m_spinSampleInterval->value() : 1.0;

  if (!m_comboTimeUnit) {
    return 1.0;
  }

  switch (m_comboTimeUnit->currentIndex()) {
  case 1:
    return sampleIntervalMs;
  case 2:
    return sampleIntervalMs / 1000.0;
  case 0:
  default:
    return 1.0;
  }
}

int SerialSession::currentTimeUnitPrecision() const {
  const double sampleIntervalMs =
      m_spinSampleInterval ? m_spinSampleInterval->value() : 1.0;
  const bool sampleIntervalIsInteger =
      qFuzzyIsNull(sampleIntervalMs - qRound64(sampleIntervalMs));

  if (!m_comboTimeUnit) {
    return 0;
  }

  switch (m_comboTimeUnit->currentIndex()) {
  case 1:
    return sampleIntervalIsInteger ? 0 : 3;
  case 2:
    return 3;
  case 0:
  default:
    return 0;
  }
}

int SerialSession::displayWidthToPoints(double displayWidth) const {
  const double scale = currentTimeUnitScale();
  if (scale <= 0.0) {
    return 1;
  }

  const int bufferLimit = m_spinBufferLimit
                              ? qMax(1, m_spinBufferLimit->value())
                              : qMax(1, m_viewWidthPoints);
  return qBound(1, qRound(displayWidth / scale), bufferLimit);
}

double SerialSession::pointsToDisplayWidth(int points) const {
  return points * currentTimeUnitScale();
}

void SerialSession::refreshViewWidthSpin() {
  if (!m_spinPoints) {
    return;
  }

  const int bufferLimit = m_spinBufferLimit
                              ? qMax(1, m_spinBufferLimit->value())
                              : qMax(1, m_viewWidthPoints);
  m_viewWidthPoints = qBound(1, m_viewWidthPoints, bufferLimit);

  const int unitIndex = m_comboTimeUnit ? m_comboTimeUnit->currentIndex() : 0;
  const int precision = currentTimeUnitPrecision();
  const double minValue = pointsToDisplayWidth(1);
  const double maxValue = pointsToDisplayWidth(bufferLimit);

  QSignalBlocker blocker(m_spinPoints);
  m_spinPoints->setDecimals(precision);
  m_spinPoints->setRange(minValue, qMax(minValue, maxValue));
  m_spinPoints->setSingleStep(
      qMax(unitIndex == 0 ? 1.0 : 0.001, pointsToDisplayWidth(1)));

  if (unitIndex == 1) {
    m_spinPoints->setSuffix(" ms");
  } else if (unitIndex == 2) {
    m_spinPoints->setSuffix(" s");
  } else {
    m_spinPoints->setSuffix(" pt");
  }

  m_spinPoints->setValue(pointsToDisplayWidth(m_viewWidthPoints));
}

void SerialSession::rescaleXAxisData(double oldScale, double newScale) {
  if (qFuzzyCompare(oldScale, newScale) || oldScale == 0.0) {
    return;
  }

  const double ratio = newScale / oldScale;
  for (int i = 0; i < m_customPlot->graphCount(); ++i) {
    QCPGraph *graph = m_customPlot->graph(i);
    if (!graph) {
      continue;
    }

    QSharedPointer<QCPGraphDataContainer> data = graph->data();
    if (!data || data->isEmpty()) {
      continue;
    }

    QVector<double> keys;
    QVector<double> values;
    keys.reserve(data->size());
    values.reserve(data->size());

    for (auto it = data->constBegin(); it != data->constEnd(); ++it) {
      keys.append(it->key * ratio);
      values.append(it->value);
    }

    graph->setData(keys, values, true);
  }
}

void SerialSession::updateXAxisRange() {
  const int maxPoints = m_viewWidthPoints;
  const int bufferLimit = m_spinBufferLimit->value();
  const double scale = m_xAxisScale;

  const double rawDataMinX = qMax(0.0, m_xValue - bufferLimit);
  const double rawDataMaxX = m_xValue;
  const bool isTracking =
      (m_scrollbarWaveform->value() == m_scrollbarWaveform->maximum());
  const int scrollMax =
      qMax(0, static_cast<int>(rawDataMaxX - rawDataMinX - maxPoints));

  m_scrollbarWaveform->blockSignals(true);
  m_scrollbarWaveform->setMinimum(0);
  m_scrollbarWaveform->setMaximum(scrollMax);
  if (isTracking) {
    m_scrollbarWaveform->setValue(scrollMax);
  } else if (m_scrollbarWaveform->value() > scrollMax) {
    m_scrollbarWaveform->setValue(scrollMax);
  }
  m_scrollbarWaveform->blockSignals(false);

  const double visibleWidth = maxPoints * scale;
  if (rawDataMaxX <= maxPoints) {
    m_customPlot->xAxis->setRange(0, visibleWidth);
    return;
  }

  if (isTracking) {
    m_customPlot->xAxis->setRange(rawDataMaxX * scale, visibleWidth,
                                  Qt::AlignRight);
  } else {
    const double rawViewLeft = rawDataMinX + m_scrollbarWaveform->value();
    m_customPlot->xAxis->setRange(rawViewLeft * scale,
                                  (rawViewLeft + maxPoints) * scale);
  }
}

void SerialSession::updateChartSettings() {
  if (m_btnAutoScale->isChecked()) {
    m_spinYMin->setEnabled(false);
    m_spinYMax->setEnabled(false);
    m_spinYTick->setEnabled(false);
    // Determine best fit immediately
    for (int i = 0; i < m_customPlot->graphCount(); ++i) {
      if (m_customPlot->graph(i)) {
        if (i == 0)
          m_customPlot->graph(i)->rescaleValueAxis(true);
        else
          m_customPlot->graph(i)->rescaleValueAxis(true, true);
      }
    }
    // Auto handled in updateWaveform or by QCP rescale
  } else {
    m_spinYMin->setEnabled(true);
    m_spinYMax->setEnabled(true);
    m_spinYTick->setEnabled(true);
    m_customPlot->yAxis->setRange(m_spinYMin->value(), m_spinYMax->value());

    if (m_spinYTick->value() > 0) {
      QSharedPointer<QCPAxisTickerFixed> fixedTicker(new QCPAxisTickerFixed);
      fixedTicker->setTickStep(m_spinYTick->value());
      m_customPlot->yAxis->setTicker(fixedTicker);
    } else {
      QSharedPointer<QCPAxisTicker> autoTicker(new QCPAxisTicker);
      m_customPlot->yAxis->setTicker(autoTicker);
    }
  }

  // Grid
  bool showGrid = m_chkShowGrid->isChecked();
  m_customPlot->xAxis->grid()->setVisible(showGrid);
  m_customPlot->yAxis->grid()->setVisible(showGrid);
  m_customPlot->xAxis->grid()->setSubGridVisible(showGrid);
  m_customPlot->yAxis->grid()->setSubGridVisible(showGrid);

  updateXAxisRange();

  m_customPlot->replot();
}

bool SerialSession::takeNextFrame(QByteArray &payload) {
  payload.clear();

  // 帧尾是分帧的必要条件，未配置时不做任何解析
  if (m_frameTail.isEmpty()) {
    return false;
  }

  const bool strict = m_chkStrictFrame && m_chkStrictFrame->isChecked();
  const bool requireHeader = strict && !m_frameHeader.isEmpty();
  const int overflowLimit = qMax(4096, m_spinBufferLimit->value() * 64);

  while (true) {
    int payloadStart = 0;

    if (!m_frameHeader.isEmpty()) {
      const int headerIndex = m_rxBuffer.indexOf(m_frameHeader);
      if (headerIndex < 0) {
        if (requireHeader) {
          // 严格模式：没有帧头的数据一律丢弃，只保留可能被拆包的尾部字节
          const int bytesToKeep =
              qMin(m_rxBuffer.size(), m_frameHeader.size() - 1);
          m_rxBuffer = m_rxBuffer.right(bytesToKeep);
          return false;
        }
        // 宽松模式：允许没有帧头，直接按帧尾切分
      } else {
        if (headerIndex > 0) {
          m_rxBuffer.remove(0, headerIndex); // 丢弃帧头之前的噪声
        }
        payloadStart = m_frameHeader.size();
      }
    }

    const int tailIndex = m_rxBuffer.indexOf(m_frameTail, payloadStart);
    if (tailIndex < 0) {
      // 帧尾还没到，等待后续数据；缓冲异常膨胀时做安全回收
      if (m_rxBuffer.size() > overflowLimit) {
        if (m_frameHeader.isEmpty()) {
          m_rxBuffer.clear();
        } else {
          const int lastHeader = m_rxBuffer.lastIndexOf(m_frameHeader);
          m_rxBuffer = lastHeader > 0
                           ? m_rxBuffer.mid(lastHeader)
                           : m_rxBuffer.right(m_frameHeader.size() - 1);
        }
      }
      return false;
    }

    // 严格模式下，若帧头与帧尾之间又出现帧头，说明前一帧被截断，重新同步
    if (requireHeader && payloadStart > 0) {
      const int nestedHeader = m_rxBuffer.indexOf(m_frameHeader, payloadStart);
      if (nestedHeader >= 0 && nestedHeader < tailIndex) {
        m_rxBuffer.remove(0, nestedHeader);
        continue;
      }
    }

    payload = m_rxBuffer.mid(payloadStart, tailIndex - payloadStart).trimmed();
    m_rxBuffer.remove(0, tailIndex + m_frameTail.size());

    // 严格模式要求帧头存在，缺失帧头的帧直接丢弃并继续查找下一帧
    if (requireHeader && payloadStart == 0) {
      payload.clear();
      continue;
    }
    return true;
  }
}

void SerialSession::updateWaveform(const QByteArray &data) {
  bool waveformStopped = m_btnStopWaveform && m_btnStopWaveform->isChecked();
  // 若波形已暂停且不需要显示 Payload，则直接跳过
  if (waveformStopped && !m_chkShowRawData->isChecked()) {
    return;
  }

  m_rxBuffer.append(data);

  int maxPoints = m_viewWidthPoints;
  bool dataAdded = false;

  QVector<QVector<double>> channelDataBatch;
  QVector<QVector<double>> channelKeysBatch;

  QByteArray payloadBytes;
  while (takeNextFrame(payloadBytes)) {
    if (payloadBytes.isEmpty())
      continue;

    QString payload = QString::fromUtf8(payloadBytes);

    int colonIdx = payload.indexOf(':');
    QString prefix = "";
    QString dataPart = payload;

    if (colonIdx != -1) {
      prefix = payload.left(colonIdx).trimmed();
      dataPart = payload.mid(colonIdx + 1).trimmed();
    }

    if (prefix == "image") {
      continue;
    }

    if (prefix == "LED") {
      QStringList ledParts = dataPart.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
      QVector<int> ledStates;
      for (const QString &p : ledParts) {
        ledStates.append(p.toInt());
      }
      if (m_widgetDesigner) {
        emit m_widgetDesigner->ledStatesReceived(ledStates);
      }
      continue;
    }

    // 勾选「显示原始数据」时，显示去掉帧尾后的 Payload
    if (m_chkShowRawData->isChecked()) {
      m_textReceive->append("<span style='color: #4CAF50;'>[Payload] " +
                            payload.toHtmlEscaped() + "</span>");
      m_textReceive->verticalScrollBar()->setValue(
          m_textReceive->verticalScrollBar()->maximum());
    }

    // 如果波形已暂停，跳过图表数据更新
    if (waveformStopped) {
      continue;
    }

    QStringList parts =
        dataPart.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
    if (parts.isEmpty())
      continue;

    QVector<double> frameValues;
    frameValues.reserve(parts.size());
    bool validFrame = true;
    for (const QString &part : parts) {
      bool ok = false;
      const double value = part.toDouble(&ok);
      if (!ok) {
        validFrame = false;
        break;
      }
      frameValues.append(value);
    }
    if (!validFrame) {
      continue;
    }

    // Ensure we have enough graphs
    int neededGraphs = frameValues.size();
    while (m_customPlot->graphCount() < neededGraphs) {
      int idx = m_customPlot->graphCount();
      m_customPlot->addGraph();

      int hue = (idx * 137) % 360;
      QColor color = QColor::fromHsv(hue, 230, 255);
      QPen pen(color);
      pen.setWidthF(2.0f);
      m_customPlot->graph(idx)->setPen(pen);
      m_customPlot->graph(idx)->setName(QString("CH%1").arg(idx + 1));

      QWidget *chWidget = new QWidget();
      chWidget->setObjectName("channelRow");
      chWidget->setMinimumHeight(38);
      chWidget->setStyleSheet(
          "QWidget#channelRow { background: #F7F9FB; border: 1px solid #E3E8EE; "
          "border-radius: 8px; }");

      QHBoxLayout *chLayout = new QHBoxLayout(chWidget);
      chLayout->setContentsMargins(8, 2, 8, 2);
      chLayout->setSpacing(8);

      QLabel *colorLabel = new QLabel("●");
      colorLabel->setStyleSheet(
          QString("color: %1; font-size: 18px; font-weight: bold;").arg(color.name()));

      QLabel *nameLabel = new QLabel(m_customPlot->graph(idx)->name());
      nameLabel->setStyleSheet("QLabel { font-weight: 600; color: #374151; }");

      QToolButton *eyeBtn = new QToolButton();
      eyeBtn->setCheckable(true);
      eyeBtn->setChecked(true);
      eyeBtn->setFont(CIconFont::instance()->getIconFont(16));
      eyeBtn->setText(QChar(0xE846));
      eyeBtn->setToolTip("显示/隐藏通道");
      eyeBtn->setStyleSheet(
          "QToolButton { border: 1px solid #D1D5DB; border-radius: 6px; "
          "padding: 2px 6px; background: transparent; } "
          "QToolButton:checked { color: #1B5E20; border-color: #81C784; "
          "background: #E8F5E9; } "
          "QToolButton:!checked { color: #9CA3AF; border-color: #E5E7EB; }");

      connect(eyeBtn, &QToolButton::toggled, [this, idx, eyeBtn](bool checked) {
        eyeBtn->setText(checked ? QChar(0xE846) : QChar(0xE847));
        if (idx < m_customPlot->graphCount() && m_customPlot->graph(idx)) {
          m_customPlot->graph(idx)->setVisible(checked);
          m_customPlot->replot();
        }
      });

      chLayout->addWidget(colorLabel);
      chLayout->addWidget(nameLabel);
      chLayout->addStretch();
      chLayout->addWidget(eyeBtn);

      if (m_channelsLayout->count() > 0) {
        m_channelsLayout->insertWidget(m_channelsLayout->count() - 1, chWidget);
      } else {
        m_channelsLayout->addWidget(chWidget);
      }
      m_channelWidgets[idx] = chWidget;

      if (m_lblChannelHint) {
        m_lblChannelHint->hide();
      }
    }

    // Accumulate data to graphs. A frame is accepted only when every channel
    // value is valid, so all channels remain aligned to the same sample key.
    for (int i = 0; i < frameValues.size(); ++i) {
      if (i >= channelDataBatch.size()) {
        channelDataBatch.resize(i + 1);
        channelKeysBatch.resize(i + 1);
      }
      channelDataBatch[i].append(frameValues[i]);
      channelKeysBatch[i].append(m_xValue * m_xAxisScale);
    }

    m_xValue++;
    dataAdded = true;
  }

  if (dataAdded) {
    int bufferLimit = m_spinBufferLimit->value();
    // Batch removal threshold to prevent continuous array shifting
    int removalThreshold = bufferLimit + maxPoints;

    for (int i = 0; i < channelDataBatch.size(); ++i) {
      if (!channelDataBatch[i].isEmpty()) {
        m_customPlot->graph(i)->addData(channelKeysBatch[i],
                                        channelDataBatch[i]);

        // Only trim when it exceeds limit significantly (e.g. by one screen
        // width)
        if (m_customPlot->graph(i)->dataCount() > removalThreshold) {
          m_customPlot->graph(i)->data()->removeBefore(
              (m_xValue - bufferLimit) * m_xAxisScale);
        }
      }
    }

    updateXAxisRange();

    if (!m_waveformPage->isHidden()) {
      m_needsReplot = true; // Flag for the 30fps timer to pick up
    }
  }
}

void SerialSession::onReplotTimeout() {
  if (!m_needsReplot || m_waveformPage->isHidden())
    return;

  if (m_btnAutoScale->isChecked()) {
    for (int i = 0; i < m_customPlot->graphCount(); ++i) {
      if (i == 0)
        m_customPlot->graph(i)->rescaleValueAxis(false, true);
      else
        m_customPlot->graph(i)->rescaleValueAxis(true, true);
    }
  }

  m_customPlot->replot();
  m_needsReplot = false;
}

void SerialSession::onWaveformScroll(int value) {
  int maxPoints = m_viewWidthPoints;
  int bufferLimit = m_spinBufferLimit->value();
  double dataMinX = qMax(0.0, m_xValue - bufferLimit);

  double viewLeft = dataMinX + value;
  m_customPlot->xAxis->setRange(viewLeft * m_xAxisScale,
                                (viewLeft + maxPoints) * m_xAxisScale);

  if (!m_waveformPage->isHidden()) {
    m_needsReplot = true;
  }
}

void SerialSession::onTimeUnitChanged(int index) {
  QString label;
  double scale = 1.0;
  int precision = currentTimeUnitPrecision();

  if (index == 0) {
    label = "Time (Points)";
    scale = 1.0;
  } else if (index == 1) {
    label = "Time (ms)";
    scale = currentTimeUnitScale();
  } else if (index == 2) {
    label = "Time (s)";
    scale = currentTimeUnitScale();
  } else {
    label = "Time";
  }

  const double oldScale = m_xAxisScale;
  m_xAxisScale = scale;
  rescaleXAxisData(oldScale, m_xAxisScale);

  m_customPlot->xAxis->setTicker(
      QSharedPointer<QCPAxisTicker>(new QCPAxisTicker));
  m_customPlot->xAxis->setNumberFormat("f");
  m_customPlot->xAxis->setNumberPrecision(precision);
  m_customPlot->xAxis->setLabel(label);
  refreshViewWidthSpin();
  updateXAxisRange();
  m_needsReplot = true;
}

void SerialSession::onChartThemeChanged(int index) {
  applyChartTheme(index);

  // Update existing channels' line thickness and color vibrancy based on theme
  for (int i = 0; i < m_customPlot->graphCount(); ++i) {
    if (m_customPlot->graph(i)) {
      int hue = (i * 137) % 360;
      QColor color;
      QPen pen;

      if (index == 0) {
        // Light Theme: Standard saturation
        color = QColor::fromHsv(hue, 200, 200);
        pen = QPen(color, 1.5f);
      } else if (index == 1) {
        // Neon Theme: High saturation and value
        color = QColor::fromHsv(hue, 230, 255);
        pen = QPen(color, 2.0f);
      } else if (index == 2) {
        // Sci-Fi Oscilloscope: Extremely bright, almost glowing borders
        color = QColor::fromHsv(hue, 250, 255);
        pen = QPen(color, 2.5f);
      }

      m_customPlot->graph(i)->setPen(pen);
    }
  }
  m_needsReplot = true;
}

void SerialSession::onChartContextMenu(const QPoint &pos) {
  QMenu menu(this);
  menu.addAction("复制绘图", this, [this]() {
    QApplication::clipboard()->setPixmap(m_customPlot->toPixmap());
    ToastWidget::showToast("已复制到剪贴板", false, this);
  });
  menu.addSeparator();
  menu.addAction("导出为 PNG", this, [this]() {
    QString fileName =
        QFileDialog::getSaveFileName(this, "保存为 PNG", "", "Images (*.png)");
    if (!fileName.isEmpty()) {
      m_customPlot->savePng(fileName);
      ToastWidget::showToast("已成功导出为 PNG", false, this);
    }
  });
  menu.addAction("导出为 JPG", this, [this]() {
    QString fileName =
        QFileDialog::getSaveFileName(this, "保存为 JPG", "", "Images (*.jpg)");
    if (!fileName.isEmpty()) {
      m_customPlot->saveJpg(fileName);
      ToastWidget::showToast("已成功导出为 JPG", false, this);
    }
  });
  menu.addAction("导出为 PDF", this, [this]() {
    QString fileName =
        QFileDialog::getSaveFileName(this, "保存为 PDF", "", "PDF (*.pdf)");
    if (!fileName.isEmpty()) {
      m_customPlot->savePdf(fileName);
      ToastWidget::showToast("已成功导出为 PDF", false, this);
    }
  });

  menu.exec(m_customPlot->mapToGlobal(pos));
}

bool SerialSession::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_customPlot) {
    if (event->type() == QEvent::Resize) {
      if (m_btnFloatingPlay) {
        // Center the floating button dynamically
        int x = (m_customPlot->width() - m_btnFloatingPlay->width()) / 2;
        int y = (m_customPlot->height() - m_btnFloatingPlay->height()) / 2;
        m_btnFloatingPlay->move(x, y);
      }
    } else if (event->type() == QEvent::Enter) {
      // Hover Enter: if plotting is currently active, show a pause icon faintly
      if (m_btnStopWaveform && !m_btnStopWaveform->isChecked() &&
          m_btnFloatingPlay) {
        m_btnFloatingPlay->setText(QChar(0xE6D3)); // Pause icon
        m_btnFloatingPlay->show();
      }
    } else if (event->type() == QEvent::Leave) {
      // Hover Leave: hide pause button if we were just hovering during play
      if (m_btnStopWaveform && !m_btnStopWaveform->isChecked() &&
          m_btnFloatingPlay) {
        m_btnFloatingPlay->hide();
      }
    }
  } else if (watched == m_textReceive && (event->type() == QEvent::Resize ||
                                          event->type() == QEvent::Show)) {
    QSize size = m_textReceive->size();
    if (event->type() == QEvent::Resize) {
      QResizeEvent *re = static_cast<QResizeEvent *>(event);
      size = re->size();
    }
    QWidget *rxFloat = m_textReceive->findChild<QWidget *>("rxFloat");
    if (rxFloat) {
      rxFloat->adjustSize();
      rxFloat->move(qMax(4, size.width() - rxFloat->width() - 35),
                    qMax(4, size.height() - rxFloat->height() - 10));
    }
    // 更新行号区域宽度
    if (m_showLineNumbers && m_lineNumberWidget) {
      m_lineNumberWidget->updateWidth();
    }
  } else if (watched == m_textSend && (event->type() == QEvent::Resize ||
                                       event->type() == QEvent::Show)) {
    QSize size = m_textSend->size();
    if (event->type() == QEvent::Resize) {
      QResizeEvent *re = static_cast<QResizeEvent *>(event);
      size = re->size();
    }
    QWidget *txFloatRight = m_textSend->findChild<QWidget *>("txFloatRight");
    QWidget *txFloatLeft = m_textSend->findChild<QWidget *>("txFloatLeft");
    if (txFloatRight) {
      txFloatRight->adjustSize();
      txFloatRight->move(qMax(4, size.width() - txFloatRight->width() - 35),
                         qMax(4, size.height() - txFloatRight->height() - 10));
    }
    if (txFloatLeft) {
      txFloatLeft->adjustSize();
      txFloatLeft->move(10,
                        qMax(4, size.height() - txFloatLeft->height() - 10));
    }
  }

  return QWidget::eventFilter(watched, event);
}

void SerialSession::updateLineNumberDisplay() {
  if (!m_lineNumberWidget || !m_textReceive)
    return;

  m_lineNumberWidget->setVisible(m_showLineNumbers);
  m_btnRxLineNumber->setChecked(m_showLineNumbers);
  if (m_showLineNumbers) {
    m_lineNumberWidget->updateWidth();
  }
  m_lineNumberWidget->update();
}

void SerialSession::sendData() {
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

  QString txRawStr;
  if (m_rbTxHex->isChecked()) {
    txRawStr = idx.toHex(' ').toUpper();
  } else {
    txRawStr = QString::fromLocal8Bit(idx);
  }

  const bool logMode = m_chkRxLog->isChecked();
  const bool showTimestamp = m_chkTxTime->isChecked() || logMode;
  const QString txTag = logMode ? "[LOG][TX]" : "[TX]";

  // Build HTML line: blue timestamp + plain TX data
  QString txHtmlLine;
  if (showTimestamp) {
    QString timeStr =
        QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    txHtmlLine = QString("<span style='color:blue;'>%1</span>"
                         "<span>%2 %3</span>")
                     .arg(timeStr.toHtmlEscaped())
                     .arg(txTag)
                     .arg(txRawStr.toHtmlEscaped());
  } else {
    txHtmlLine =
        QString("<span>%1 %2</span>").arg(txTag).arg(txRawStr.toHtmlEscaped());
  }

  m_textReceive->append(txHtmlLine);
  m_textReceive->verticalScrollBar()->setValue(
      m_textReceive->verticalScrollBar()->maximum());

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

void SerialSession::clearReceiveArea() {
  m_textReceive->clear();
  m_rxBuffer.clear();
  m_rxDecoderBuffer.clear();
  m_rxLineBuffer.clear();
  m_rxCount = 0;
  m_lblRxCount->setText("0");
  for (int i = 0; i < m_customPlot->graphCount(); ++i) {
    if (m_customPlot->graph(i) && m_customPlot->graph(i)->data()) {
      m_customPlot->graph(i)->data()->clear();
    }
  }
  m_scrollbarWaveform->setMinimum(0);
  m_scrollbarWaveform->setMaximum(0);
  m_scrollbarWaveform->setValue(0);
  m_xValue = 0;
  updateXAxisRange();
  m_customPlot->replot();
}

void SerialSession::toggleAutoSend(bool checked) {
  if (checked) {
    m_autoSendTimer->start(m_spinAutoSendInterval->value());
  } else {
    m_autoSendTimer->stop();
  }

  m_spinAutoSendInterval->setEnabled(m_serial->isOpen());
}

void SerialSession::onAutoSendTimeout() {
  // 仅在串口未打开时停止（不检查 isVisible，切换标签页时应继续发送）
  if (!m_serial->isOpen()) {
    m_autoSendTimer->stop();
    m_chkAutoSend->setChecked(false);
    return;
  }
  sendData();
}

void SerialSession::hideEvent(QHideEvent *event) {
  // 仅在串口未打开时停止定时器并取消勾选。
  // 若串口已打开，保持定时器运行：onAutoSendTimeout/onMultiSendLoop
  // 内部均有 isOpen() 检查，隐藏时继续发送也是安全的。
  if (!m_serial->isOpen()) {
    if (m_autoSendTimer->isActive()) {
      m_autoSendTimer->stop();
      m_chkAutoSend->setChecked(false);
    }
    if (m_multiLoopTimer->isActive()) {
      m_multiLoopTimer->stop();
      m_chkMultiLoop->setChecked(false);
    }
  }
  QWidget::hideEvent(event);
}

void SerialSession::updateStatusInfo() {
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
  } else {
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

QString SerialSession::getButtonStyle(ButtonType type) {
  // Base Style with shared settings
  // NOTE: font-family is intentionally NOT in baseStyle so that icon buttons
  // (Refresh/Open) can use the iconfont set via setFont() without QSS override.
  QString baseStyle = "QPushButton { "
                      "    border-radius: 10px; "
                      "    border: 1px solid #90A4AE; "
                      "    padding: 5px; "
                      "    color: black; "
                      "}"
                      "QPushButton:hover { "
                      "    font-weight: 900; "
                      "    font-size: 10pt; "
                      "}";

  QString gradient;
  switch (type) {
  case ButtonType::Normal:
    // Glossy Blue-Gray, include font-family for CJK text buttons
    gradient = "QPushButton { "
               "    font-family: 'Microsoft YaHei UI'; "
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

void SerialSession::onTxModeChanged(bool hexChecked) {
  QString current = m_textSend->toPlainText().trimmed();
  if (current.isEmpty())
    return;

  if (hexChecked) {
    // ASCII -> HEX: encode text bytes to space-separated uppercase hex
    QByteArray bytes = current.toLocal8Bit();
    QString hexStr = bytes.toHex(' ').toUpper();
    m_textSend->setPlainText(hexStr);
  } else {
    // HEX -> ASCII: try to decode hex string back to text
    // Remove all whitespace first to handle spaces between bytes
    QString clean = current.remove(' ').remove('\t');
    QByteArray bytes = QByteArray::fromHex(clean.toUtf8());
    if (!bytes.isEmpty()) {
      m_textSend->setPlainText(QString::fromLocal8Bit(bytes));
    }
  }
}

// ──────────────────────────────────────────────────────────
// Multi-send: helper to push current page data into widgets
// ──────────────────────────────────────────────────────────
void SerialSession::refreshMultiPage() {
  if (m_multiPages.isEmpty())
    return;
  const auto &page = m_multiPages.at(m_multiPage);
  for (int i = 0; i < MULTI_PER_PAGE; i++) {
    const MultiSendItem &item =
        (i < page.size()) ? page.at(i) : MultiSendItem{};
    // Block signals to avoid saving back while loading
    m_chkMultiItem[i]->blockSignals(true);
    m_leMultiItem[i]->blockSignals(true);
    m_chkMultiItem[i]->setChecked(item.enabled);
    m_leMultiItem[i]->setText(item.content);
    m_chkMultiItem[i]->blockSignals(false);
    m_leMultiItem[i]->blockSignals(false);
  }
  int total = m_multiPages.count();
  m_lblMultiPage->setText(
      QString("第 %1 / %2 页").arg(m_multiPage + 1).arg(total));
  m_spinJumpPage->setValue(m_multiPage + 1);
  m_btnMultiFirst->setEnabled(m_multiPage > 0);
  m_btnMultiPrev->setEnabled(m_multiPage > 0);
  m_btnMultiNext->setEnabled(m_multiPage < total - 1);
  m_btnMultiLast->setEnabled(m_multiPage < total - 1);
  m_btnMultiDelPage->setEnabled(total > 1);
}

void SerialSession::onMultiPageChanged(int page) {
  // Save current page edits to model first
  if (m_multiPage < m_multiPages.size()) {
    auto &cur = m_multiPages[m_multiPage];
    for (int i = 0; i < MULTI_PER_PAGE; i++) {
      cur[i].enabled = m_chkMultiItem[i]->isChecked();
      cur[i].content = m_leMultiItem[i]->text();
    }
  }
  // Clamp
  m_multiPage = qBound(0, page, m_multiPages.count() - 1);
  refreshMultiPage();
}

// ──────────────────────────────────────────────────────────
// Send all checked items on current page
// ──────────────────────────────────────────────────────────
void SerialSession::sendSelectedMulti() {
  if (!m_serial->isOpen())
    return;

  // Save current edits first
  if (m_multiPage < m_multiPages.size()) {
    auto &cur = m_multiPages[m_multiPage];
    for (int i = 0; i < MULTI_PER_PAGE; i++) {
      cur[i].enabled = m_chkMultiItem[i]->isChecked();
      cur[i].content = m_leMultiItem[i]->text();
    }
  }

  bool globalHex = m_chkMultiHex->isChecked();
  bool globalNewLine = m_chkMultiNewLine->isChecked();
  const bool logMode = m_chkRxLog->isChecked();

  const auto &page = m_multiPages.at(m_multiPage);
  int idx = 0;
  for (const MultiSendItem &item : page) {
    idx++;
    if (!item.enabled || item.content.isEmpty())
      continue;

    QString text = item.content;
    QByteArray data;
    bool isHex = globalHex || item.isHex;
    if (isHex) {
      data = QByteArray::fromHex(text.remove(' ').toUtf8());
    } else {
      data = text.toLocal8Bit();
      if (globalNewLine)
        data.append("\r\n");
    }

    m_serial->write(data);
    m_txCount += data.size();
    m_lblTxCount->setText(QString::number(m_txCount));

    QString rawStr =
        isHex ? data.toHex(' ').toUpper() : QString::fromLocal8Bit(data);
    QString timeStr =
        QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    QString txHtml =
        QString("<span style='color:blue;'>%1</span><span>%2#%3 %4</span>")
            .arg(timeStr.toHtmlEscaped())
            .arg(logMode ? "[LOG][TX]" : "[TX]")
            .arg(idx)
            .arg(rawStr.toHtmlEscaped());
    m_textReceive->append(txHtml);
    m_textReceive->verticalScrollBar()->setValue(
        m_textReceive->verticalScrollBar()->maximum());
  }
}

// ──────────────────────────────────────────────────────────
// Auto-loop: cycle through ALL pages sending checked items
// ──────────────────────────────────────────────────────────
void SerialSession::onMultiSendLoop() {
  if (!m_serial->isOpen()) {
    m_multiLoopTimer->stop();
    return;
  }

  const bool logMode = m_chkRxLog->isChecked();

  // Find next enabled item across all pages
  int totalItems = m_multiPages.count() * MULTI_PER_PAGE;
  for (int tries = 0; tries < totalItems; tries++) {
    int page = m_multiLoopIndex / MULTI_PER_PAGE;
    int idx = m_multiLoopIndex % MULTI_PER_PAGE;
    m_multiLoopIndex = (m_multiLoopIndex + 1) % totalItems;

    if (page >= m_multiPages.count())
      continue;
    const MultiSendItem &item = m_multiPages.at(page).at(idx);
    if (!item.enabled || item.content.isEmpty())
      continue;

    bool isHex = m_chkMultiHex->isChecked() || item.isHex;
    QString text = item.content;
    QByteArray data = isHex ? QByteArray::fromHex(text.remove(' ').toUtf8())
                            : text.toLocal8Bit();
    if (m_chkMultiNewLine->isChecked() && !isHex)
      data.append("\r\n");

    m_serial->write(data);
    m_txCount += data.size();
    m_lblTxCount->setText(QString::number(m_txCount));

    QString rawStr =
        isHex ? data.toHex(' ').toUpper() : QString::fromLocal8Bit(data);
    QString timeStr =
        QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    QString html =
        QString("<span style='color:blue;'>%1</span><span>%2 P%3#%4 %5</span>")
            .arg(timeStr.toHtmlEscaped())
            .arg(logMode ? "[LOG][LOOP]" : "[LOOP]")
            .arg(page + 1)
            .arg(idx + 1)
            .arg(rawStr.toHtmlEscaped());
    m_textReceive->append(html);
    m_textReceive->verticalScrollBar()->setValue(
        m_textReceive->verticalScrollBar()->maximum());
    break; // send one per tick
  }
}

// ──────────────────────────────────────────────────────────
// Import / Export CSV
// Format: page_index,item_index,enabled,hex,content
// ──────────────────────────────────────────────────────────
void SerialSession::importMultiData() {
  QString path = QFileDialog::getOpenFileName(this, "导入多条发送数据", "",
                                              "CSV (*.csv);;All files (*)");
  if (path.isEmpty())
    return;

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::warning(this, "导入失败", f.errorString());
    return;
  }
  QTextStream ts(&f);
  // Clear existing data
  m_multiPages.clear();

  while (!ts.atEnd()) {
    QString line = ts.readLine().trimmed();
    if (line.isEmpty() || line.startsWith('#'))
      continue;
    QStringList cols = line.split(',');
    if (cols.size() < 5)
      continue;
    int pageIdx = cols[0].toInt();
    int itemIdx = cols[1].toInt();
    while (m_multiPages.size() <= pageIdx)
      m_multiPages.append(QVector<MultiSendItem>(MULTI_PER_PAGE));
    if (itemIdx >= 0 && itemIdx < MULTI_PER_PAGE) {
      MultiSendItem &item = m_multiPages[pageIdx][itemIdx];
      item.enabled = cols[2].trimmed() == "1";
      item.isHex = cols[3].trimmed() == "1";
      item.content = cols.mid(4).join(','); // content may contain commas
    }
  }
  if (m_multiPages.isEmpty())
    m_multiPages.append(QVector<MultiSendItem>(MULTI_PER_PAGE));

  m_multiPage = 0;
  m_spinJumpPage->setRange(1, m_multiPages.count());
  refreshMultiPage();
}

void SerialSession::exportMultiData() {
  // Save current page edits
  if (m_multiPage < m_multiPages.size()) {
    auto &cur = m_multiPages[m_multiPage];
    for (int i = 0; i < MULTI_PER_PAGE; i++) {
      cur[i].enabled = m_chkMultiItem[i]->isChecked();
      cur[i].content = m_leMultiItem[i]->text();
    }
  }

  QString path = QFileDialog::getSaveFileName(
      this, "导出多条发送数据", "multi_send.csv", "CSV (*.csv);;All files (*)");
  if (path.isEmpty())
    return;

  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, "导出失败", f.errorString());
    return;
  }
  QTextStream ts(&f);
  ts << "# page_index,item_index,enabled,hex,content\n";
  for (int p = 0; p < m_multiPages.count(); p++) {
    const auto &page = m_multiPages.at(p);
    for (int i = 0; i < page.size(); i++) {
      const MultiSendItem &item = page.at(i);
      ts << p << ',' << i << ',' << (item.enabled ? 1 : 0) << ','
         << (item.isHex ? 1 : 0) << ',' << item.content << '\n';
    }
  }
}

// sendAll() kept for MOC compatibility – delegates to sendSelectedMulti()
void SerialSession::sendAll() { sendSelectedMulti(); }

void SerialSession::applyTheme(const QString &themeMode) {
  if (!m_customPlot)
    return;
  bool isDark = themeMode.contains("dark", Qt::CaseInsensitive) ||
                themeMode.contains("one", Qt::CaseInsensitive);

  QColor bgColor = isDark ? QColor("#1E1E1E") : QColor("#FFFFFF");
  QColor textColor = isDark ? QColor("#DCDCDC") : QColor("#000000");
  QColor gridColor = isDark ? QColor("#3E3E42") : QColor("#E0E0E0");

  m_customPlot->setBackground(bgColor);
  m_customPlot->axisRect()->setBackground(bgColor);

  // X Axis
  m_customPlot->xAxis->setBasePen(QPen(textColor));
  m_customPlot->xAxis->setTickPen(QPen(textColor));
  m_customPlot->xAxis->setSubTickPen(QPen(textColor));
  m_customPlot->xAxis->setTickLabelColor(textColor);
  m_customPlot->xAxis->setLabelColor(textColor);
  m_customPlot->xAxis->grid()->setPen(QPen(gridColor, 1, Qt::DotLine));
  m_customPlot->xAxis->grid()->setZeroLinePen(QPen(gridColor));

  // Y Axis
  m_customPlot->yAxis->setBasePen(QPen(textColor));
  m_customPlot->yAxis->setTickPen(QPen(textColor));
  m_customPlot->yAxis->setSubTickPen(QPen(textColor));
  m_customPlot->yAxis->setTickLabelColor(textColor);
  m_customPlot->yAxis->setLabelColor(textColor);
  m_customPlot->yAxis->grid()->setPen(QPen(gridColor, 1, Qt::DotLine));
  m_customPlot->yAxis->grid()->setZeroLinePen(QPen(gridColor));

  m_customPlot->replot();
}

// =========================================================================
// SerialPortPlot: 顶层多标签页容器（会话管理器）
// =========================================================================
#include "TOOLS/CIconFont.h"
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTabBar>
#include <QToolBar>
#include <QToolButton>

SerialPortPlot::SerialPortPlot(QWidget *parent)
    : QWidget(parent), m_sessionCounter(1) {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  // --- 选项卡区域 ---
  m_sessionTabs = new QTabWidget(this);
  m_sessionTabs->setTabsClosable(true);

  // Split actions corner widget
  QWidget *cornerWidget = new QWidget(this);
  QHBoxLayout *cornerLayout = new QHBoxLayout(cornerWidget);
  cornerLayout->setContentsMargins(0, 0, 0, 0);
  cornerLayout->setSpacing(2);

  QFont iconFont = CIconFont::instance()->getIconFont(24);

  QToolButton *btnSplitH = new QToolButton(this);
  btnSplitH->setFont(iconFont);
  btnSplitH->setText(QString(QChar(0xe7f7)));
  btnSplitH->setToolTip("水平分屏");

  QToolButton *btnSplitV = new QToolButton(this);
  btnSplitV->setFont(iconFont);
  btnSplitV->setText(QString(QChar(0xe8cc)));
  btnSplitV->setToolTip("垂直分屏");

  QToolButton *btnCloseSplit = new QToolButton(this);
  btnCloseSplit->setFont(iconFont);
  btnCloseSplit->setText(QString(QChar(0xe7ac)));
  btnCloseSplit->setToolTip("关闭分屏");

  cornerLayout->addWidget(btnSplitH);
  cornerLayout->addWidget(btnSplitV);
  cornerLayout->addWidget(btnCloseSplit);

  m_sessionTabs->setCornerWidget(cornerWidget, Qt::TopRightCorner);

  connect(btnSplitH, &QToolButton::clicked, this,
          &SerialPortPlot::onSplitHorizontal);
  connect(btnSplitV, &QToolButton::clicked, this,
          &SerialPortPlot::onSplitVertical);
  connect(btnCloseSplit, &QToolButton::clicked, this,
          &SerialPortPlot::onCloseSplit);
  m_sessionTabs->setMovable(true);

  // 应用仿浏览器圆角标签页样式
  m_sessionTabs->setStyleSheet(R"(
    QTabWidget::pane {
        border-top: 1px solid #C0C0C0;
        background-color: transparent;
        margin-top: -1px;
    }
    QTabWidget::left-corner {
        background: transparent;
        border: none;
        width: 0px;
    }
    QTabWidget::right-corner {
        background: transparent;
        border: none;
    }
    QTabBar::tab {
        background: #E8E8E8;
        border: 1px solid #C0C0C0;
        border-bottom-color: #C0C0C0;
        border-top-left-radius: 8px;
        border-top-right-radius: 8px;
        min-width: 100px;
        padding: 6px 16px;
        margin-right: 2px;
        margin-top: 4px;
    }
    QTabBar::tab:selected, QTabBar::tab:hover {
        background: #FFFFFF;
        border-bottom-color: #FFFFFF;
    }
    QTabBar::tab:selected {
        margin-top: 0px;
        font-weight: bold;
    }
    QTabBar::tab:first {
        margin-left: 0px;
    }
    /* 针对我们特殊的加号标签稍作样式调整 */
    QTabBar::tab:last {
        min-width: 30px;
        padding: 6px 8px;
        background: transparent;
        border: none;
        margin-top: 4px;
        font-weight: bold;
        color: #555555;
    }
    QTabBar::tab:last:hover {
        background: #D0D0D0;
        border-radius: 8px;
        color: #000000;
    }
  )");

  // 安装事件过滤器用于监听双击重命名和“+”号假选项卡的点击
  m_sessionTabs->tabBar()->installEventFilter(this);

  connect(m_sessionTabs, &QTabWidget::tabCloseRequested, this,
          &SerialPortPlot::onTabCloseRequested);

  mainLayout->addWidget(m_sessionTabs);

  // 先添加 ➕ 号假标签（使用更宽更显眼的全角加号或者emoji包裹空格）
  m_sessionTabs->addTab(new QWidget(), "  ➕  ");
  // 隐藏假标签的关闭按钮
  m_sessionTabs->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);

  // 默认启动一个会话
  addNewSession();
}

SerialPortPlot::~SerialPortPlot() {}

bool SerialPortPlot::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_sessionTabs->tabBar()) {
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      int index = m_sessionTabs->tabBar()->tabAt(mouseEvent->pos());
      // 如果点击的是最后一个 "+" 号标签页
      if (index == m_sessionTabs->count() - 1 &&
          mouseEvent->button() == Qt::LeftButton) {
        addNewSession();
        return true;
      }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      int index = m_sessionTabs->tabBar()->tabAt(mouseEvent->pos());
      // 排除 "+" 标签页的重命名
      if (index >= 0 && index < m_sessionTabs->count() - 1) {
        onTabDoubleClicked(index);
        return true;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void SerialPortPlot::addNewSession() {
  SerialSession *session = new SerialSession(this);
  QString title;
  if (m_sessionCounter == 1) {
    title = "SerialPortPlotPro";
  } else {
    title = QString("SerialPortPlotPro%1").arg(m_sessionCounter);
  }
  m_sessionCounter++;

  // The user requested View toolbar inside the inner session instead of main
  // window. We will configure the local toolbar inside SerialSession.

  // 插入到 "  ➕  " 号前面
  int addIndex = m_sessionTabs->count() - 1;
  int index = m_sessionTabs->insertTab(addIndex, session, title);
  m_sessionTabs->setCurrentIndex(index);
}

SerialSession *SerialPortPlot::getActiveSession() const {
  if (!m_sessionTabs)
    return nullptr;
  int currentIndex = m_sessionTabs->currentIndex();
  if (currentIndex < 0 || currentIndex >= m_sessionTabs->count() - 1)
    return nullptr;

  return qobject_cast<SerialSession *>(m_sessionTabs->widget(currentIndex));
}

void SerialPortPlot::toggleDock(int dockType, bool checked) {
  // Get current session
  int currentIndex = m_sessionTabs->currentIndex();
  if (currentIndex < 0 || currentIndex >= m_sessionTabs->count() - 1)
    return;

  SerialSession *session =
      qobject_cast<SerialSession *>(m_sessionTabs->widget(currentIndex));
  if (!session)
    return;

  switch (dockType) {
  case 0:
    // m_dockPort已改为标签页，不再需要setVisible
    // session->m_dockPort->setVisible(checked);
    break;
  case 1:
    session->m_dockRx->setVisible(checked);
    break;
  case 2:
    session->m_dockTx->setVisible(checked);
    break;
  case 3:
    // m_dockScopeSettings已改为标签页，不再需要setVisible
    // session->m_dockScopeSettings->setVisible(checked);
    break;
  }
}

void SerialPortPlot::onTabDoubleClicked(int index) {
  if (index < 0)
    return;
  bool ok;
  QString currentTitle = m_sessionTabs->tabText(index);
  QString newTitle = QInputDialog::getText(
      this, "重命名会话", "新名称:", QLineEdit::Normal, currentTitle, &ok);
  if (ok && !newTitle.isEmpty()) {
    m_sessionTabs->setTabText(index, newTitle);
  }
}

void SerialPortPlot::onTabCloseRequested(int index) {
  // 禁止关闭 "+" 号标签页
  if (index == m_sessionTabs->count() - 1)
    return;

  // 保持至少一个真实会话，避免误关闭整个串口工具窗口
  if (m_sessionTabs->count() <= 2) {
    return;
  }

  // 记录即将关闭前要跳转到的索引，防止跳到 "+" 标签
  int nextIndex = -1;
  if (m_sessionTabs->currentIndex() == index) {
    if (index > 0) {
      nextIndex = index - 1; // 优先跳到左边
    } else if (index < m_sessionTabs->count() - 2) {
      nextIndex = index + 1; // 不可能的话跳到右边的真实标签
    }
  }

  QWidget *widget = m_sessionTabs->widget(index);
  if (widget) {
    widget->deleteLater();
  }
  m_sessionTabs->removeTab(index);

  // 切换到合适的索引，防止激活 "+" 标签
  if (nextIndex >= 0) {
    m_sessionTabs->setCurrentIndex(nextIndex);
  } else {
    // 默认情况如果跑到了 "+" 标签，则强制跳回最后一个真实标签
    if (m_sessionTabs->currentIndex() == m_sessionTabs->count() - 1) {
      m_sessionTabs->setCurrentIndex(m_sessionTabs->count() - 2);
    }
  }
}

void SerialPortPlot::applyGlobalTheme(const QString &themeFile) {
  QFile file(QString(":/resources/styles/") + themeFile);
  if (file.open(QFile::ReadOnly)) {
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet + unifiedToolTipStyleSheet());
    ensureUnifiedToolTipStyle();
    file.close();
  }

  // Also pass to internal sessions
  for (int i = 0; i < m_sessionTabs->count() - 1; i++) {
    SerialSession *session =
        qobject_cast<SerialSession *>(m_sessionTabs->widget(i));
    if (session) {
      session->applyTheme(themeFile);
    }
  }

  emit themeChanged(themeFile);
}

void SerialPortPlot::applyFileIconTheme(const QString &themeName) {
  QIcon::setThemeName(themeName);
}

void SerialPortPlot::onSplitHorizontal() { emit requestSplitHorizontal(this); }

void SerialPortPlot::onSplitVertical() { emit requestSplitVertical(this); }

void SerialPortPlot::onCloseSplit() { emit requestCloseSplit(this); }

// =========================================================================
// SerialPortContainer Implementation
// =========================================================================

SerialPortContainer::SerialPortContainer(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  // --- 全局工具栏 ---
  m_toolbar = new QToolBar("Main Toolbar", this);
  m_toolbar->setMovable(false);

  // Settings
  m_toolbar->addAction("设置");

  // --------- Theme (Menu) ---------
  QToolButton *btnTheme = new QToolButton(this);
  btnTheme->setText("主题");
  btnTheme->setPopupMode(QToolButton::InstantPopup);
  QMenu *menuTheme = new QMenu(btnTheme);

  // 1. Color Theme Submenu
  QMenu *menuColorTheme = menuTheme->addMenu("颜色主题");
  // Github
  QMenu *menuGithub = menuColorTheme->addMenu("Github");
  menuGithub->addAction("Dark", this,
                        [this]() { applyGlobalTheme("githubdark.qss"); });
  menuGithub->addAction("Light", this,
                        [this]() { applyGlobalTheme("githublight.qss"); });
  // Aura
  QMenu *menuAura = menuColorTheme->addMenu("Aura");
  menuAura->addAction("Dark", this,
                      [this]() { applyGlobalTheme("auradark.qss"); });
  menuAura->addAction("Light", this,
                      [this]() { applyGlobalTheme("auralight.qss"); });
  // ATOM
  QMenu *menuAtom = menuColorTheme->addMenu("ATOM");
  menuAtom->addAction("Dark", this,
                      [this]() { applyGlobalTheme("atomone.qss"); });
  menuAtom->addAction("Light", this,
                      [this]() { applyGlobalTheme("atomlight.qss"); });
  // Solarized
  QMenu *menuSolarized = menuColorTheme->addMenu("Solarized");
  menuSolarized->addAction("Dark", this,
                           [this]() { applyGlobalTheme("solarizeddark.qss"); });
  menuSolarized->addAction(
      "Light", this, [this]() { applyGlobalTheme("solarizedlight.qss"); });

  // 2. File Icon Theme Submenu
  QMenu *menuFileIcon = menuTheme->addMenu("文件图标主题");
  menuFileIcon->addAction("Material Icon", this,
                          [this]() { applyFileIconTheme("material"); });
  menuFileIcon->addAction("VSCode Icon", this,
                          [this]() { applyFileIconTheme("vscode"); });

  // 3. Product Icon Theme Submenu
  QMenu *menuProductIcon = menuTheme->addMenu("产品图标主题");
  menuProductIcon->addAction("Default", this,
                             [this]() { applyFileIconTheme("default"); });

  btnTheme->setMenu(menuTheme);
  m_toolbar->addWidget(btnTheme);

  // Help
  m_toolbar->addAction("帮助");

  // About
  m_toolbar->addAction("关于");

  // 视图 (View)
  QToolButton *btnView = new QToolButton(this);
  btnView->setText("视图");
  btnView->setPopupMode(QToolButton::InstantPopup);
  QMenu *menuView = new QMenu(btnView);

  connect(menuView, &QMenu::aboutToShow, this, [this, menuView]() {
    menuView->clear();
    if (m_plotHistory.isEmpty())
      return;

    SerialPortPlot *activePlot = nullptr;
    for (SerialPortPlot *plot : m_plotHistory) {
      if (plot->isAncestorOf(QApplication::focusWidget()) || plot->hasFocus()) {
        activePlot = plot;
        break;
      }
    }
    if (!activePlot)
      activePlot = m_plotHistory.last();

    SerialSession *session = activePlot->getActiveSession();
    if (session) {
      // m_dockPort和m_dockScopeSettings已改为标签页，不再添加到菜单
      // menuView->addAction(session->m_dockPort->toggleViewAction());
      menuView->addAction(session->m_dockRx->toggleViewAction());
      menuView->addAction(session->m_dockTx->toggleViewAction());
      menuView->addSeparator();
      menuView->addAction(session->m_dockReceive->toggleViewAction());
      menuView->addAction(session->m_dockSend->toggleViewAction());
      // menuView->addAction(session->m_dockScopeSettings->toggleViewAction());
    } else {
      menuView->addAction("当前无活动会话")->setEnabled(false);
    }
  });

  btnView->setMenu(menuView);
  m_toolbar->addWidget(btnView);

  layout->addWidget(m_toolbar);

  m_mainSplitter = new QSplitter(Qt::Horizontal, this);
  layout->addWidget(m_mainSplitter);

  SerialPortPlot *initialPlot = createNewPlot();
  m_mainSplitter->addWidget(initialPlot);
}

SerialPortContainer::~SerialPortContainer() {}

SerialPortPlot *SerialPortContainer::createNewPlot() {
  SerialPortPlot *plot = new SerialPortPlot(this);
  connect(plot, &SerialPortPlot::requestSplitHorizontal, this,
          &SerialPortContainer::handleSplitHorizontal);
  connect(plot, &SerialPortPlot::requestSplitVertical, this,
          &SerialPortContainer::handleSplitVertical);
  connect(plot, &SerialPortPlot::requestCloseSplit, this,
          &SerialPortContainer::handleCloseSplit);
  connect(plot, &SerialPortPlot::themeChanged, this,
          &SerialPortContainer::handleThemeChanged);

  if (!m_currentTheme.isEmpty()) {
    plot->applyGlobalTheme(m_currentTheme);
  }

  m_plotHistory.append(plot);
  return plot;
}

void SerialPortContainer::handleThemeChanged(const QString &themeName) {
  m_currentTheme = themeName;
}

void SerialPortContainer::applyGlobalTheme(const QString &themeFile) {
  m_currentTheme = themeFile;
  for (SerialPortPlot *plot : qAsConst(m_plotHistory)) {
    plot->applyGlobalTheme(themeFile);
  }
}

void SerialPortContainer::applyFileIconTheme(const QString &themeName) {
  for (SerialPortPlot *plot : qAsConst(m_plotHistory)) {
    plot->applyFileIconTheme(themeName);
  }
}

void SerialPortContainer::handleSplitHorizontal() {
  SerialPortPlot *senderPlot = qobject_cast<SerialPortPlot *>(sender());
  if (!senderPlot)
    return;

  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(senderPlot->parentWidget());
  if (!parentSplitter)
    return;

  SerialPortPlot *newPlot = createNewPlot();

  if (parentSplitter->orientation() == Qt::Horizontal) {
    int index = parentSplitter->indexOf(senderPlot);
    parentSplitter->insertWidget(index + 1, newPlot);
  } else {
    int index = parentSplitter->indexOf(senderPlot);
    QSplitter *newSplitter = new QSplitter(Qt::Horizontal, parentSplitter);

    QList<int> sizes = parentSplitter->sizes();

    parentSplitter->insertWidget(index, newSplitter);
    newSplitter->addWidget(senderPlot);
    newSplitter->addWidget(newPlot);

    parentSplitter->setSizes(sizes);
  }
}

void SerialPortContainer::handleSplitVertical() {
  SerialPortPlot *senderPlot = qobject_cast<SerialPortPlot *>(sender());
  if (!senderPlot)
    return;

  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(senderPlot->parentWidget());
  if (!parentSplitter)
    return;

  SerialPortPlot *newPlot = createNewPlot();

  if (parentSplitter->orientation() == Qt::Vertical) {
    int index = parentSplitter->indexOf(senderPlot);
    parentSplitter->insertWidget(index + 1, newPlot);
  } else {
    int index = parentSplitter->indexOf(senderPlot);
    QSplitter *newSplitter = new QSplitter(Qt::Vertical, parentSplitter);

    QList<int> sizes = parentSplitter->sizes();

    parentSplitter->insertWidget(index, newSplitter);
    newSplitter->addWidget(senderPlot);
    newSplitter->addWidget(newPlot);

    parentSplitter->setSizes(sizes);
  }
}

void SerialPortContainer::handleCloseSplit(SerialPortPlot *plotToClose) {
  if (m_plotHistory.size() <= 1) {
    return; // Don't close the last one
  }
  if (!plotToClose || !m_plotHistory.contains(plotToClose)) {
    return;
  }

  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(plotToClose->parentWidget());
  if (parentSplitter) {
    plotToClose->hide();
    plotToClose->deleteLater();
    m_plotHistory.removeAll(plotToClose);
  }
}
