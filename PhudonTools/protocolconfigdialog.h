#ifndef PROTOCOLCONFIGDIALOG_H
#define PROTOCOLCONFIGDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QGroupBox>

enum class ProtocolType {
  CAN_Bus = 0,
  Modbus_RTU = 1,
  Modbus_TCP = 2,
  RS232_RS485 = 3,
  TCP_UDP_Socket = 4
};

struct CommProtocolConfig {
  ProtocolType protocolType = ProtocolType::CAN_Bus;
  int canChannel = -1; // -1: 任意/双通道, 0: 通道0, 1: 通道1
  QString canDeviceName = QStringLiteral("USBCAN / CANFD 接口适配器");
  
  // Modbus / Serial / Network 预留扩展配置
  QString serialPort = QStringLiteral("COM1");
  int baudRate = 115200;
  int slaveId = 1;
  QString ipAddress = QStringLiteral("192.168.1.100");
  int port = 502;

  QString summaryString() const {
    if (protocolType == ProtocolType::CAN_Bus) {
      if (canChannel == 0) return QStringLiteral("🔌 协议: CAN (通道 0)");
      if (canChannel == 1) return QStringLiteral("🔌 协议: CAN (通道 1)");
      return QStringLiteral("🔌 协议: CAN (全部/任意通道)");
    } else if (protocolType == ProtocolType::Modbus_RTU) {
      return QStringLiteral("🔌 协议: Modbus RTU (%1)").arg(serialPort);
    } else if (protocolType == ProtocolType::Modbus_TCP) {
      return QStringLiteral("🔌 协议: Modbus TCP (%1:%2)").arg(ipAddress).arg(port);
    } else if (protocolType == ProtocolType::RS232_RS485) {
      return QStringLiteral("🔌 协议: RS232/RS485 (%1)").arg(serialPort);
    } else {
      return QStringLiteral("🔌 协议: TCP/UDP (%1:%2)").arg(ipAddress).arg(port);
    }
  }
};

class CommProtocolConfigDialog : public QDialog {
  Q_OBJECT
public:
  explicit CommProtocolConfigDialog(const CommProtocolConfig &config, QWidget *parent = nullptr);
  CommProtocolConfig config() const;

private slots:
  void onProtocolChanged(int index);

private:
  void setupUi();

  CommProtocolConfig m_config;

  QComboBox *m_protocolTypeCombo = nullptr;
  QStackedWidget *m_stackedPages = nullptr;

  // CAN 属性页 UI
  QComboBox *m_canChannelCombo = nullptr;
  QLineEdit *m_canDeviceEdit = nullptr;

  // Modbus RTU / 串口 属性页 UI
  QLineEdit *m_serialPortEdit = nullptr;
  QComboBox *m_baudRateCombo = nullptr;
  QSpinBox *m_slaveIdSpin = nullptr;

  // Modbus TCP / Socket 属性页 UI
  QLineEdit *m_ipAddressEdit = nullptr;
  QSpinBox *m_portSpin = nullptr;
};

#endif // PROTOCOLCONFIGDIALOG_H
