#ifndef SERIALPORTPLOT_H
#define SERIALPORTPLOT_H

#include <QAction>
#include <QByteArray>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QString>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVector>
#include <QWidget>

// Charts
#include "../qcustomplot/qcustomplot.h"
#include "curvesettings.h"
#include "scrollinglabel.h"
#include <QDialog>
#include <QDockWidget>
#include <QMainWindow>

// ---- Multi-send data item ----
struct MultiSendItem {
  bool enabled = false;
  QString content;
  bool isHex = false;
};

static const int MULTI_COLS = 2;
static const int MULTI_ROWS = 5;
static const int MULTI_PER_PAGE = MULTI_COLS * MULTI_ROWS; // 10

class SerialSession : public QWidget {
  Q_OBJECT

public:
  explicit SerialSession(QWidget *parent = nullptr);
  ~SerialSession();

  // Status Bar
  QLabel *m_lblPortInfo;
  ScrollingLabel *m_lblWelcome;
  QString m_welcomeText;

  // Hot Plug
  QTimer *m_portCheckTimer;
  int m_lastPortCount;

private slots:
  // Serial Port Control
  void refreshPorts();
  void checkPorts();
  void openClosePort();
  void onPortError(QSerialPort::SerialPortError error);

  // Data Handling
  void onReadyRead();
  void sendData();
  void clearReceiveArea();
  void toggleAutoSend(bool checked);
  void onAutoSendTimeout();
  void onTxModeChanged(bool hexChecked);

protected:
  void hideEvent(QHideEvent *event) override;

  // Multi-send
  void sendAll();           // kept for single-tab compat
  void sendSelectedMulti(); // send checked items on current page
  void onMultiPageChanged(int page);
  void onMultiSendLoop(); // timer-driven loop send
  void importMultiData();
  void exportMultiData();

  // Waveform Settings
  void onDockLocationChanged(Qt::DockWidgetArea area);
  void updateChartSettings();
  void onWaveformScroll(int value);
  void onTimeUnitChanged(int index);
  void onChartThemeChanged(int index);

  // UI Updates
  void updateWaveform(const QByteArray &data);
  void onChartContextMenu(const QPoint &pos);
  void onReplotTimeout();

public:
  void applyTheme(const QString &themeMode);

  // Waveform Settings (Accessed by Toolbar in Manager)
  void onWaveformEnabled(bool checked);
  void onCurveSettingsClicked();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  enum class ButtonType { Normal, Refresh, Open, Close };
  QString getButtonStyle(ButtonType type);

  void setupUi();
  void setupConnections();
  void setupChart();
  void applyChartTheme(int index);
  void updateStatusInfo();
  void updateXAxisRange();
  void rescaleXAxisData(double oldScale, double newScale);
  double currentTimeUnitScale() const;
  int currentTimeUnitPrecision() const;
  int displayWidthToPoints(double displayWidth) const;
  double pointsToDisplayWidth(int points) const;
  void refreshViewWidthSpin();
  void refreshMultiPage(); // redraw 20 widgets for current page

  // Serial Port
  QSerialPort *m_serial;
  QTimer *m_autoSendTimer;
  quint64 m_rxCount;
  quint64 m_txCount;

  // UI Elements - Port Settings
  QComboBox *m_comboPort;
  QComboBox *m_comboBaud;
  QComboBox *m_comboDataBits;
  QComboBox *m_comboParity;
  QComboBox *m_comboStopBits;
  QPushButton *m_btnOpenClose;
  QLabel *m_lblStatusIcon;
  QPushButton *m_btnRefresh;

  // UI Elements - Receive Settings
  QRadioButton *m_rbRxAscii;
  QRadioButton *m_rbRxHex;
  QCheckBox *m_chkRxLog;
  QCheckBox *m_chkRxTime;
  QPushButton *m_btnClearRx;
  QPushButton *m_btnStopRx;

  // Floating Controls for Receive Text Area
  QToolButton *m_btnRxHexToggle;
  QToolButton *m_btnRxTimeToggle;
  QToolButton *m_btnRxPauseToggle;
  QToolButton *m_btnRxClear;

  // UI Elements - Send Settings
  QRadioButton *m_rbTxAscii;
  QRadioButton *m_rbTxHex;
  QToolButton *m_chkTxNewLine;
  QCheckBox *m_chkTxTime;
  QToolButton *m_chkAutoSend;
  QSpinBox *m_spinAutoSendInterval;
  QComboBox *m_comboHistory;
  QTextEdit *m_textSend;
  QToolButton *m_btnSend;

  // Floating Controls for Send Text Area
  QToolButton *m_btnTxHexToggle;
  QToolButton *m_btnTxTimeToggle;
  QToolButton *m_btnTxClear;

  // Send tab widget (wrapped in GroupBox)
  // m_sendTabWidget; // moved to public section

  // ---- Multi-send data model ----
  int m_multiPage;                              // 0-based current page
  QVector<QVector<MultiSendItem>> m_multiPages; // all pages

  // Multi-send grid widgets (20 per page, reused across pages)
  QCheckBox *m_chkMultiItem[MULTI_PER_PAGE];
  QLineEdit *m_leMultiItem[MULTI_PER_PAGE];

  // Multi-send page navigation
  QLabel *m_lblMultiPage;
  QSpinBox *m_spinJumpPage;
  QPushButton *m_btnMultiFirst;
  QPushButton *m_btnMultiPrev;
  QPushButton *m_btnMultiNext;
  QPushButton *m_btnMultiLast;
  QPushButton *m_btnMultiAddPage;
  QPushButton *m_btnMultiDelPage;
  QPushButton *m_btnMultiImport;
  QPushButton *m_btnMultiExport;
  QPushButton *m_btnSendAll; // send selected on current page

  // Multi-send options
  QCheckBox *m_chkMultiNewLine;
  QCheckBox *m_chkMultiHex;
  QCheckBox *m_chkMultiLoop;
  QSpinBox *m_spinMultiLoopInterval;
  QTimer *m_multiLoopTimer;
  int m_multiLoopIndex;

public:
  // UI Elements - Dock Widgets
  QDockWidget *m_dockPort;
  QDockWidget *m_dockRx;
  QDockWidget *m_dockTx;
  QDockWidget *m_dockScopeSettings;
  QDockWidget *m_dockReceive;
  QDockWidget *m_dockSend;

  // Statistics Labels
  QLabel *m_lblRxCount;
  QLabel *m_lblTxCount;

  // Waveform elements
  QCheckBox *m_chkEnableWaveform;
  QCustomPlot *m_customPlot;
  QBoxLayout *m_settingsLayout;

  // Other UI members that were likely public
  QTabWidget *m_sendTabWidget;
  QTextEdit *m_textReceive;

  // Waveform parameters
  double m_xValue;
  double m_xAxisScale;
  int m_viewWidthPoints;
  QDoubleSpinBox *m_spinPoints;
  QByteArray m_frameHeader;
  QByteArray m_frameTail;
  QLineEdit *m_editFrameHeader;
  QLineEdit *m_editFrameTail;
  QSpinBox *m_spinBufferLimit;
  QDoubleSpinBox *m_spinSampleInterval;
  QComboBox *m_comboTimeUnit;
  QComboBox *m_comboChartTheme;
  QPushButton *m_btnAutoScale;
  QCheckBox *m_chkShowGrid;
  QDoubleSpinBox *m_spinYMin;
  QDoubleSpinBox *m_spinYMax;
  QDoubleSpinBox *m_spinYTick;
  QPushButton *m_btnResetChart;
  QPushButton *m_btnClearWaveform;
  QPushButton *m_btnStopWaveform;
  QPushButton *m_btnCurveSettings;

  QVBoxLayout *m_channelsLayout;
  QMap<int, QWidget *> m_channelWidgets;

  QCheckBox *m_chkHideRxTx;
  QCheckBox *m_chkHideRxData;
  QCheckBox *m_chkShowRawData;
  QWidget *m_dataSplitter;
  QSplitter *m_mainHorizSplitter;
  QScrollBar *m_scrollbarWaveform;
  QToolButton *m_btnFloatingPlay;

  QMainWindow *m_waveformPage;
  QMainWindow *m_innerMainWindow;

  QGroupBox *m_groupPlotParams;
  QGroupBox *m_groupYAxis;

  // 控件设计器相关
  class WidgetDesignerArea *m_widgetDesigner;
  class WidgetToolbox *m_widgetToolbox;
  class VerticalTabWidget *m_leftTabWidget;
  QDockWidget *m_dockWidgetDesigner;
  int m_designerTabIndex = -1;

private:
  // Render Throttling
  QTimer *m_replotTimer;
  bool m_needsReplot;

  // Per-instance waveform receive buffer (must NOT be static)
  QByteArray m_rxBuffer;
};

// -------------------------------------------------------------
// SerialPortPlot: 顶层多标签页容器（"会话管理器"）
// -------------------------------------------------------------
class SerialPortPlot : public QWidget {
  Q_OBJECT

public:
  explicit SerialPortPlot(QWidget *parent = nullptr);
  ~SerialPortPlot();

  void applyGlobalTheme(const QString &themeFile);
  void applyFileIconTheme(const QString &themeName);

  SerialSession *getActiveSession() const;

signals:
  void themeChanged(const QString &themeName);
  void requestSplitHorizontal(SerialPortPlot *plot);
  void requestSplitVertical(SerialPortPlot *plot);
  void requestCloseSplit(SerialPortPlot *plot);

public slots:
  void toggleDock(int dockType, bool checked);

private slots:
  void addNewSession();
  void onTabDoubleClicked(int index);
  void onTabCloseRequested(int index);

  void onSplitHorizontal();
  void onSplitVertical();
  void onCloseSplit();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  QTabWidget *m_sessionTabs;
  int m_sessionCounter;
};

class SerialPortContainer : public QWidget {
  Q_OBJECT

public:
  explicit SerialPortContainer(QWidget *parent = nullptr);
  ~SerialPortContainer();

private slots:
  void handleSplitHorizontal();
  void handleSplitVertical();
  void handleCloseSplit(SerialPortPlot *plot);
  void handleThemeChanged(const QString &themeName);

private:
  QToolBar *m_toolbar;
  QSplitter *m_mainSplitter;

  void applyGlobalTheme(const QString &themeFile);
  void applyFileIconTheme(const QString &themeName);

  SerialPortPlot *createNewPlot();
  QString m_currentTheme;
  QList<SerialPortPlot *> m_plotHistory;
};

#endif // SERIALPORTPLOT_H
