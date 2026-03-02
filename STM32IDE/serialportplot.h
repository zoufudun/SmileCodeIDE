#ifndef SERIALPORTPLOT_H
#define SERIALPORTPLOT_H

#include <QAction>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QString>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QVector>
#include <QWidget>

// Charts
#include "../qcustomplot/qcustomplot.h"
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
  int m_scrollPos;

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

  // UI Updates
  void updateWaveform(const QByteArray &data);
  void scrollWelcomeMessage();
  void onChartContextMenu(const QPoint &pos);

public:
  void applyTheme(const QString &themeMode);

  // Waveform Settings (Accessed by Toolbar in Manager)
  void onWaveformEnabled(bool checked);

private:
  enum class ButtonType { Normal, Refresh, Open, Close };
  QString getButtonStyle(ButtonType type);

  void setupUi();
  void setupConnections();
  void setupChart();
  void updateStatusInfo();
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
  QLabel *m_statusLabel;
  QPushButton *m_btnRefresh;

  // UI Elements - Receive Settings
  QRadioButton *m_rbRxAscii;
  QRadioButton *m_rbRxHex;
  QCheckBox *m_chkRxLog;
  QCheckBox *m_chkRxTime;
  QCheckBox *m_chkRxNewLine;
  QPushButton *m_btnClearRx;
  QPushButton *m_btnStopRx;

  // UI Elements - Send Settings
  QRadioButton *m_rbTxAscii;
  QRadioButton *m_rbTxHex;
  QCheckBox *m_chkTxNewLine;
  QCheckBox *m_chkTxTime;
  QCheckBox *m_chkAutoSend;
  QSpinBox *m_spinAutoSendInterval;
  QComboBox *m_comboHistory;
  QTextEdit *m_textSend;
  QPushButton *m_btnSend;
  QPushButton *m_btnClearSend;

  // Send tab widget (wrapped in GroupBox)
  QTabWidget *m_sendTabWidget;

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

  // Data Display
  QTextEdit *m_textReceive;
  QLabel *m_lblRxCount;
  QLabel *m_lblTxCount;

  // Waveform
  QCheckBox *m_chkEnableWaveform;
  QCheckBox *m_chkScopeSettings;
  QCustomPlot *m_customPlot;
  double m_xValue;

public:
  // Waveform Settings UI
  QDockWidget *m_dockSettings;
  QBoxLayout *m_settingsLayout;
  QSpinBox *m_spinPoints;
  QPushButton *m_btnAutoScale;
  QCheckBox *m_chkShowGrid;
  QDoubleSpinBox *m_spinYMin;
  QDoubleSpinBox *m_spinYMax;
  QDoubleSpinBox *m_spinYTick;
  QPushButton *m_btnResetChart;
  QPushButton *m_btnClearWaveform;
  QPushButton *m_btnStopWaveform;

  // New UI controls for Waveform
  QCheckBox *m_chkHideRxTx;
  QCheckBox *m_chkHideRxData;
  QCheckBox *m_chkShowRawData;
  QSplitter *m_dataSplitter;

  // Extended Page
  QMainWindow *m_waveformPage;

  // Global UI Structure
  QTabWidget *m_mainTabWidget;
};

// -------------------------------------------------------------
// SerialPortPlot: 顶层多标签页容器（"会话管理器"）
// -------------------------------------------------------------
class SerialPortPlot : public QWidget {
  Q_OBJECT

public:
  explicit SerialPortPlot(QWidget *parent = nullptr);
  ~SerialPortPlot();

signals:
  void themeChanged(const QString &themeName);

private slots:
  void addNewSession();
  void onTabDoubleClicked(int index);
  void onTabCloseRequested(int index);

  // Theme functions
  void applyGlobalTheme(const QString &themeFile);
  void applyFileIconTheme(const QString &themeName);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  QTabWidget *m_sessionTabs;
  int m_sessionCounter;
};

#endif // SERIALPORTPLOT_H
