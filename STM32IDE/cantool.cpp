#include "cantool.h"

CANTool::CANTool(QWidget *parent) : QDialog(parent) { setupUi(); }

void CANTool::setupUi() {
  setWindowTitle("CAN调试助手");
  setMinimumSize(600, 400);

  // 创建布局
  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // 创建设置区域
  QGroupBox *settingsGroup = new QGroupBox("CAN设置");
  QGridLayout *settingsLayout = new QGridLayout(settingsGroup);

  // 添加波特率选择
  QLabel *baudRateLabel = new QLabel("波特率:");
  m_baudRateComboBox = new QComboBox();
  m_baudRateComboBox->addItems(
      {"1000K", "800K", "500K", "250K", "125K", "100K", "50K", "20K", "10K"});
  m_baudRateComboBox->setCurrentText("500K");

  // 添加工作模式选择
  QLabel *modeLabel = new QLabel("工作模式:");
  m_modeComboBox = new QComboBox();
  m_modeComboBox->addItems(
      {"正常模式", "环回模式", "静默模式", "环回静默模式"});

  // 添加连接/断开按钮
  m_connectButton = new QPushButton("打开CAN设备");

  // 将控件添加到设置布局
  settingsLayout->addWidget(baudRateLabel, 0, 0);
  settingsLayout->addWidget(m_baudRateComboBox, 0, 1);
  settingsLayout->addWidget(modeLabel, 1, 0);
  settingsLayout->addWidget(m_modeComboBox, 1, 1);
  settingsLayout->addWidget(m_connectButton, 2, 0, 1, 2);

  // 创建数据显示区域
  QGroupBox *dataGroup = new QGroupBox("数据接收");
  QVBoxLayout *dataLayout = new QVBoxLayout(dataGroup);

  m_receiveTreeWidget = new QTreeWidget();
  m_receiveTreeWidget->setHeaderLabels(
      {"时间", "帧ID", "帧类型", "数据长度", "数据"});
  m_receiveTreeWidget->setColumnWidth(0, 150);
  dataLayout->addWidget(m_receiveTreeWidget);

  // 创建发送区域
  QGroupBox *sendGroup = new QGroupBox("数据发送");
  QGridLayout *sendLayout = new QGridLayout(sendGroup);

  QLabel *idLabel = new QLabel("帧ID(Hex):");
  m_idLineEdit = new QLineEdit("123");
  QLabel *dataLabel = new QLabel("数据(Hex):");
  m_dataLineEdit = new QLineEdit("00 11 22 33 44 55 66 77");
  m_sendButton = new QPushButton("发送");

  QHBoxLayout *typeLayout = new QHBoxLayout();
  m_stdFrameBtn = new QRadioButton("标准帧");
  m_extFrameBtn = new QRadioButton("扩展帧");
  m_stdFrameBtn->setChecked(true);
  typeLayout->addWidget(m_stdFrameBtn);
  typeLayout->addWidget(m_extFrameBtn);

  sendLayout->addWidget(idLabel, 0, 0);
  sendLayout->addWidget(m_idLineEdit, 0, 1);
  sendLayout->addLayout(typeLayout, 0, 2);
  sendLayout->addWidget(dataLabel, 1, 0);
  sendLayout->addWidget(m_dataLineEdit, 1, 1);
  sendLayout->addWidget(m_sendButton, 1, 2);

  // 将所有组添加到主布局
  mainLayout->addWidget(settingsGroup);
  mainLayout->addWidget(dataGroup);
  mainLayout->addWidget(sendGroup);

  // 设置属性，确保窗口关闭时释放资源
  setAttribute(Qt::WA_DeleteOnClose);
}
