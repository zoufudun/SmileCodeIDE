#ifndef IAPTOOL_H
#define IAPTOOL_H

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTcpSocket>
#include <QTextEdit>
#include <QTimer>
#include <QUdpSocket>
#include <QWidget>


class IAPTool : public QWidget {
  Q_OBJECT
public:
  explicit IAPTool(QWidget *parent = nullptr);
  ~IAPTool();

private slots:
  void onProtocolChanged(int index);
  void onBrowseFile();
  void onStartUpgrade();
  void onConnectClicked();

  // Serial Port
  void refreshSerialPorts();
  void onSerialDataReceived();

  // Network
  void onUdpDataReceived();
  void onTcpConnected();
  void onTcpDisconnected();
  void onTcpDataReceived();

private:
  void setupUi();
  void setupConnections();
  void log(const QString &msg);

  // Protocol Pages
  QWidget *createSerialPage();
  QWidget *createUsbPage();
  QWidget *createWifiPage(); // TCP Client usually
  QWidget *createUdpPage();

  // UI Elements
  QComboBox *m_protocolCombo;
  QStackedWidget *m_settingsStack;
  QPushButton *m_btnConnect; // For Serial/TCP connect/disconnect

  // Serial Settings
  QComboBox *m_serialPortCombo;
  QComboBox *m_baudRateCombo;

  // Network Settings
  QLineEdit *m_ipEdit;
  QSpinBox *m_portScan;

  // Common
  QLineEdit *m_filePathEdit;
  QPushButton *m_btnBrowse;
  QPushButton *m_btnStart;
  QProgressBar *m_progressBar;
  QTextEdit *m_logText;

  // Core Logic
  bool m_isConnected;
  bool m_isTransferring;
  QFile m_firmwareFile;

  // Hardware Handles
  QSerialPort *m_serialPort;
  QUdpSocket *m_udpSocket;
  QTcpSocket *m_tcpSocket;

  // Transfer State
  int m_transferState; // For state machine
  qint64 m_bytesSent;
  qint64 m_totalBytes;
};

#endif // IAPTOOL_H
