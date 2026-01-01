#ifndef SERIALPORTPLOT_H
#define SERIALPORTPLOT_H

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

// Charts
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

class SerialPortPlot : public QWidget {
  Q_OBJECT

public:
  explicit SerialPortPlot(QWidget *parent = nullptr);
  ~SerialPortPlot();

  // Status Bar
  QLabel *m_lblPortInfo;
  QLabel *m_lblWelcome;
  QTimer *m_scrollTimer;
  QString m_welcomeText;
  int m_scrollPos;

  // Hot Plug
  QTimer *m_portCheckTimer;
  int m_lastPortCount;

private slots:
  // Serial Port Control
  void refreshPorts();
  void checkPorts(); // Hot-plug check
  void openClosePort();
  void onPortError(QSerialPort::SerialPortError error);

  // Data Handling
  void onReadyRead();
  void sendData();
  void clearReceiveArea();
  void toggleAutoSend(bool checked);
  void onAutoSendTimeout();

  // Waveform Settings
  void onWaveformEnabled(bool checked);
  void updateChartSettings();

  // UI Updates
  void updateWaveform(const QByteArray &data);
  void scrollWelcomeMessage();

private:
  enum class ButtonType { Normal, Refresh, Open, Close };
  QString getButtonStyle(ButtonType type);

  void setupUi();
  void setupConnections();
  void setupChart();
  void updateStatusInfo();

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
  QPushButton *m_btnStopRx; // Pauses display, not reception buffer

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

  // Data Display
  QTextEdit *m_textReceive;
  QLabel *m_lblRxCount;
  QLabel *m_lblTxCount;

  // Waveform
  QCheckBox *m_chkEnableWaveform;
  QChartView *m_chartView;
  QLineSeries *m_series;
  QValueAxis *m_axisX;
  QValueAxis *m_axisY;
  double m_xValue;

  // Waveform Settings UI
  QGroupBox *m_grpWaveformSettings;
  QSpinBox *m_spinPoints;
  QCheckBox *m_chkAutoY;
  QDoubleSpinBox *m_spinYMin;
  QDoubleSpinBox *m_spinYMax;
  QPushButton *m_btnResetChart;
};

#endif // SERIALPORTPLOT_H
