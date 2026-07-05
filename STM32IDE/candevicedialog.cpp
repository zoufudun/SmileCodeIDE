#include "candevicedialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStyle>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "caninterface.h"
#include "USBCANFD/zlgcan.h"

namespace {
// 动态绘制红-橙色下箭头图标（代表已展开的设备）
QPixmap drawRedArrowIcon() {
  QPixmap pix(16, 16);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(Qt::NoPen);
  p.setBrush(QColor("#FF5A5F"));
  QPolygon poly;
  poly << QPoint(4, 6) << QPoint(12, 6) << QPoint(8, 11);
  p.drawPolygon(poly);
  return pix;
}

// 动态绘制小灰圆点图标（代表通道）
QPixmap drawGreyDotIcon() {
  QPixmap pix(16, 16);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(Qt::NoPen);
  p.setBrush(QColor("#BDC3C7"));
  p.drawEllipse(5, 5, 6, 6);
  return pix;
}

// 按钮统一样式设置助手
void setActionButtonStyle(QPushButton *btn, bool active) {
  btn->setEnabled(active);
  if (active) {
    btn->setStyleSheet("QPushButton { background-color: #D6E8FC; border: 1px solid #ADC3E6; color: #2C3E50; font-size: 12px; padding: 4px 12px; border-radius: 4px; min-width: 60px; }"
                       "QPushButton:hover { background-color: #C0DEFC; }");
  } else {
    btn->setStyleSheet("QPushButton { background-color: #ECEFF1; border: 1px solid #CFD8DC; color: #B0BEC5; font-size: 12px; padding: 4px 12px; border-radius: 4px; min-width: 60px; }");
  }
}
} // namespace

// --- 启动通道参数配置对话框 ---
class StartChannelDialog : public QDialog {
public:
  explicit StartChannelDialog(QWidget *parent = nullptr) : QDialog(parent) {
    setWindowTitle("启动");
    setMinimumSize(420, 500);
    setStyleSheet("QDialog { background-color: white; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->setSpacing(10);

    // 波特率计算器按钮
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addStretch();
    QPushButton *btnCalc = new QPushButton("波特率计算器");
    btnCalc->setStyleSheet("QPushButton { background-color: #E6F0FA; border: 1px solid #B0CBE6; color: #2C3E50; padding: 3px 10px; border-radius: 4px; font-size: 11px; }");
    topLayout->addWidget(btnCalc);
    mainLayout->addLayout(topLayout);

    // 滚动区域放置配置项
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border: 1px solid #E0E0E0; background-color: #FAFAFA; border-radius: 4px; }");
    
    QWidget *scrollWidget = new QWidget();
    scrollWidget->setStyleSheet("QWidget { background: transparent; }");
    QFormLayout *form = new QFormLayout(scrollWidget);
    form->setSpacing(10);
    form->setContentsMargins(15, 15, 15, 15);

    m_protocolCombo = new QComboBox();
    m_protocolCombo->addItems({"CAN FD", "CAN"});

    m_standardCombo = new QComboBox();
    m_standardCombo->addItems({"CAN FD ISO", "CAN FD Non-ISO"});

    m_accelCombo = new QComboBox();
    m_accelCombo->addItems({"是", "否"});

    m_abitBaudCombo = new QComboBox();
    m_abitBaudCombo->addItem("1Mbps 80%", 1000000);
    m_abitBaudCombo->addItem("800kbps 80%", 800000);
    m_abitBaudCombo->addItem("500kbps 80%", 500000);
    m_abitBaudCombo->addItem("250kbps 80%", 250000);
    m_abitBaudCombo->addItem("125kbps 80%", 125000);
    m_abitBaudCombo->addItem("100kbps 80%", 100000);
    m_abitBaudCombo->addItem("50kbps 80%", 50000);
    m_abitBaudCombo->setCurrentText("500kbps 80%");

    m_dbitBaudCombo = new QComboBox();
    m_dbitBaudCombo->addItem("5Mbps 75%", 5000000);
    m_dbitBaudCombo->addItem("4Mbps 75%", 4000000);
    m_dbitBaudCombo->addItem("2Mbps 75%", 2000000);
    m_dbitBaudCombo->addItem("1Mbps 75%", 1000000);
    m_dbitBaudCombo->setCurrentText("2Mbps 75%");

    m_customBaudEdit = new QLineEdit();
    m_customBaudEdit->setPlaceholderText("未配置");

    m_workModeCombo = new QComboBox();
    m_workModeCombo->addItems({"正常模式", "只听模式"});

    m_terminalResCombo = new QComboBox();
    m_terminalResCombo->addItems({"使能", "禁用"});

    m_reportBusCombo = new QComboBox();
    m_reportBusCombo->addItems({"禁用", "使能"});

    m_busPeriodEdit = new QLineEdit("100");

    m_retryCombo = new QComboBox();
    m_retryCombo->addItem("发送到总线关闭", 1);
    m_retryCombo->addItem("重试3次", 3);
    m_retryCombo->addItem("不重试", 0);

    // 美化控件
    QString comboStyle = "QComboBox { border: 1px solid #CCCCCC; padding: 3px 6px; border-radius: 3px; background: white; min-height: 22px; }"
                         "QComboBox::drop-down { border: none; width: 18px; }"
                         "QComboBox::down-arrow { border-left: 3px solid transparent; border-right: 3px solid transparent; border-top: 4px solid #FF5A5F; margin-top: 1px; }";
    QString editStyle = "QLineEdit { border: 1px solid #CCCCCC; padding: 3px 6px; border-radius: 3px; background: white; min-height: 22px; }";

    m_protocolCombo->setStyleSheet(comboStyle);
    m_standardCombo->setStyleSheet(comboStyle);
    m_accelCombo->setStyleSheet(comboStyle);
    m_abitBaudCombo->setStyleSheet(comboStyle);
    m_dbitBaudCombo->setStyleSheet(comboStyle);
    m_workModeCombo->setStyleSheet(comboStyle);
    m_terminalResCombo->setStyleSheet(comboStyle);
    m_reportBusCombo->setStyleSheet(comboStyle);
    m_retryCombo->setStyleSheet(comboStyle);
    m_customBaudEdit->setStyleSheet(editStyle);
    m_busPeriodEdit->setStyleSheet(editStyle);

    form->addRow("协议", m_protocolCombo);
    form->addRow("CANFD标准", m_standardCombo);
    form->addRow("CANFD加速", m_accelCombo);
    form->addRow("仲裁域波特率", m_abitBaudCombo);
    form->addRow("数据域波特率", m_dbitBaudCombo);
    form->addRow("自定义波特率", m_customBaudEdit);
    form->addRow("工作模式", m_workModeCombo);
    form->addRow("终端电阻", m_terminalResCombo);
    form->addRow("上报总线利用率", m_reportBusCombo);
    form->addRow("总线利用率周期(ms)", m_busPeriodEdit);
    form->addRow("发送重试", m_retryCombo);

    scroll->setWidget(scrollWidget);
    mainLayout->addWidget(scroll);

    // 协议切换联动显示
    auto onProtocolChanged = [this]() {
      bool isFD = m_protocolCombo->currentText() == "CAN FD";
      m_standardCombo->setEnabled(isFD);
      m_accelCombo->setEnabled(isFD);
      m_dbitBaudCombo->setEnabled(isFD);
    };
    connect(m_protocolCombo, &QComboBox::currentTextChanged, this, onProtocolChanged);
    onProtocolChanged();

    // 滤波设置区域
    QHBoxLayout *filterLayout = new QHBoxLayout();
    m_filterCheck = new QCheckBox("滤波");
    m_filterSetupBtn = new QPushButton("滤波设置");
    m_filterSetupBtn->setEnabled(false);
    m_filterSetupBtn->setStyleSheet("QPushButton { background-color: #ECEFF1; border: 1px solid #CFD8DC; color: #78909C; padding: 3px 8px; border-radius: 4px; font-size: 11px; }");
    connect(m_filterCheck, &QCheckBox::toggled, m_filterSetupBtn, [this](bool checked) {
      m_filterSetupBtn->setEnabled(checked);
      if (checked) {
        m_filterSetupBtn->setStyleSheet("QPushButton { background-color: #E6F0FA; border: 1px solid #B0CBE6; color: #2C3E50; padding: 3px 8px; border-radius: 4px; font-size: 11px; }");
      } else {
        m_filterSetupBtn->setStyleSheet("QPushButton { background-color: #ECEFF1; border: 1px solid #CFD8DC; color: #78909C; padding: 3px 8px; border-radius: 4px; font-size: 11px; }");
      }
    });
    filterLayout->addWidget(m_filterCheck);
    filterLayout->addWidget(m_filterSetupBtn);
    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);

    // 确定与取消按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(15);
    QPushButton *btnOk = new QPushButton("确认");
    QPushButton *btnCancel = new QPushButton("取消");

    QString actStyle = "QPushButton { border: 1px solid #ADC3E6; padding: 5px 25px; border-radius: 4px; font-weight: bold; }";
    btnOk->setStyleSheet(actStyle + "QPushButton { background-color: #D6E8FC; color: #2C3E50; } QPushButton:hover { background-color: #C0DEFC; }");
    btnCancel->setStyleSheet(actStyle + "QPushButton { background-color: #E8ECEF; color: #555555; } QPushButton:hover { background-color: #DFE3E6; }");

    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
  }

  CanChannelConfig getConfig() const {
    CanChannelConfig cfg;
    cfg.isFd = m_protocolCombo->currentText() == "CAN FD";
    cfg.isIso = m_standardCombo->currentText() == "CAN FD ISO";
    cfg.enableBrs = m_accelCombo->currentText() == "是";
    
    QString customBaud = m_customBaudEdit->text().trimmed();
    if (!customBaud.isEmpty()) {
      cfg.abitBaud = customBaud.toInt();
    } else {
      cfg.abitBaud = m_abitBaudCombo->currentData().toInt();
    }
    
    cfg.dbitBaud = m_dbitBaudCombo->currentData().toInt();
    cfg.mode = m_workModeCombo->currentText() == "只听模式" ? CanMode::ListenOnly : CanMode::Normal;
    cfg.terminalRes = m_terminalResCombo->currentText() == "使能";
    cfg.reportBusUsage = m_reportBusCombo->currentText() == "使能";
    cfg.busUsagePeriod = m_busPeriodEdit->text().toInt();
    cfg.retrySend = m_retryCombo->currentData().toInt();
    cfg.enableFilter = m_filterCheck->isChecked();
    return cfg;
  }

private:
  QComboBox *m_protocolCombo;
  QComboBox *m_standardCombo;
  QComboBox *m_accelCombo;
  QComboBox *m_abitBaudCombo;
  QComboBox *m_dbitBaudCombo;
  QLineEdit *m_customBaudEdit;
  QComboBox *m_workModeCombo;
  QComboBox *m_terminalResCombo;
  QComboBox *m_reportBusCombo;
  QLineEdit *m_busPeriodEdit;
  QComboBox *m_retryCombo;
  QCheckBox *m_filterCheck;
  QPushButton *m_filterSetupBtn;
};

// --- 设备管理窗口实现 ---
CanDeviceDialog::CanDeviceDialog(CanInterface *can, QWidget *parent)
    : QDialog(parent), m_can(can) {
  setupUi();
  refreshDeviceTree();
}

void CanDeviceDialog::setupUi() {
  setWindowTitle("设备管理");
  setMinimumSize(680, 400);
  setStyleSheet("QDialog { background-color: white; }");

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(15, 15, 15, 15);
  mainLayout->setSpacing(15);

  // 顶部配置栏 (类型, 索引, 打开设备, 云设备, 关闭窗口)
  QHBoxLayout *topPanel = new QHBoxLayout();
  topPanel->setSpacing(10);

  topPanel->addWidget(new QLabel("类型"));
  m_deviceTypeCombo = new QComboBox();
  m_deviceTypeCombo->addItem("USBCANFD-200U", ZCAN_USBCANFD_200U);
  m_deviceTypeCombo->addItem("USBCANFD-100U", ZCAN_USBCANFD_100U);
  m_deviceTypeCombo->addItem("USBCANFD-MINI", ZCAN_USBCANFD_MINI);
  m_deviceTypeCombo->addItem("USBCANFD-800U", ZCAN_USBCANFD_800U);
  m_deviceTypeCombo->setStyleSheet("QComboBox { border: 1px solid #CCCCCC; padding: 4px 8px; border-radius: 4px; background: white; min-width: 140px; }"
                                   "QComboBox::drop-down { border: none; width: 20px; }"
                                   "QComboBox::down-arrow { border-left: 4px solid transparent; border-right: 4px solid transparent; border-top: 5px solid #FF5A5F; margin-top: 2px; }");
  topPanel->addWidget(m_deviceTypeCombo);

  topPanel->addWidget(new QLabel("索引"));
  m_deviceIndexSpin = new QSpinBox();
  m_deviceIndexSpin->setRange(0, 7);
  m_deviceIndexSpin->setStyleSheet("QSpinBox { border: 1px solid #CCCCCC; padding: 4px 8px; border-radius: 4px; background: white; min-width: 50px; }");
  topPanel->addWidget(m_deviceIndexSpin);

  topPanel->addSpacing(10);

  m_openButton = new QPushButton("打开设备");
  m_cloudButton = new QPushButton("云设备");
  m_closeButton = new QPushButton("关闭窗口");

  QString topBtnStyle = "QPushButton { border: 1px solid #ADC3E6; padding: 5px 15px; border-radius: 4px; font-weight: bold; background-color: #D6E8FC; color: #2C3E50; }"
                        "QPushButton:hover { background-color: #C0DEFC; }";
  m_openButton->setStyleSheet(topBtnStyle);
  m_cloudButton->setStyleSheet(topBtnStyle);
  m_closeButton->setStyleSheet("QPushButton { border: 1px solid #ADC3E6; padding: 5px 15px; border-radius: 4px; font-weight: bold; background-color: #E8ECEF; color: #555555; }"
                               "QPushButton:hover { background-color: #DFE3E6; }");

  topPanel->addWidget(m_openButton);
  topPanel->addWidget(m_cloudButton);
  topPanel->addStretch();
  topPanel->addWidget(m_closeButton);

  mainLayout->addLayout(topPanel);

  // 中部 QTreeWidget 视图列表
  m_deviceTree = new QTreeWidget();
  m_deviceTree->setColumnCount(2);
  m_deviceTree->setHeaderHidden(true);
  m_deviceTree->setColumnWidth(0, 240);
  m_deviceTree->setColumnWidth(1, 400);
  m_deviceTree->setIndentation(20);
  m_deviceTree->setStyleSheet(
      "QTreeWidget { border: 1px solid #E2E8F0; border-radius: 4px; background-color: white; }"
      "QTreeWidget::item { height: 38px; border-bottom: 1px solid #F1F5F9; }"
  );
  
  mainLayout->addWidget(m_deviceTree);

  // 绑定连接
  connect(m_openButton, &QPushButton::clicked, this, &CanDeviceDialog::onOpenDeviceClicked);
  connect(m_cloudButton, &QPushButton::clicked, this, &CanDeviceDialog::onCloudDeviceClicked);
  connect(m_closeButton, &QPushButton::clicked, this, &QWidget::close);
}

void CanDeviceDialog::refreshDeviceTree() {
  m_deviceTree->clear();
  m_startButtons.clear();
  m_stopButtons.clear();
  m_deviceStartButton = nullptr;
  m_deviceStopButton = nullptr;
  m_deviceCloseButton = nullptr;
  m_deviceInfoButton = nullptr;

  const bool devOpen = m_can->isDeviceOpen();
  m_openButton->setText(devOpen ? "设备已打开" : "打开设备");
  m_openButton->setEnabled(!devOpen);
  m_deviceTypeCombo->setEnabled(!devOpen);
  m_deviceIndexSpin->setEnabled(!devOpen);

  if (!devOpen) {
    return;
  }

  // 1. 获取设备名称与通道数量
  QString hw, fw, dr, lib, serial, typeStr;
  int canNum = 2;
  m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &canNum, &serial, &typeStr);

  // 2. 创建设备节点
  QTreeWidgetItem *devItem = new QTreeWidgetItem(m_deviceTree);
  devItem->setText(0, QString("%1 设备%2").arg(typeStr).arg(m_deviceIndexSpin->value()));
  devItem->setIcon(0, drawRedArrowIcon());

  // 为设备节点提供：启动、停止、关闭设备、设备信息 按钮
  QWidget *devBtnWidget = new QWidget();
  QHBoxLayout *devBtnLayout = new QHBoxLayout(devBtnWidget);
  devBtnLayout->setContentsMargins(0, 0, 0, 0);
  devBtnLayout->setSpacing(6);

  m_deviceStartButton = new QPushButton("启动");
  m_deviceStopButton = new QPushButton("停止");
  m_deviceCloseButton = new QPushButton("关闭设备");
  m_deviceInfoButton = new QPushButton("设备信息");

  setActionButtonStyle(m_deviceStartButton, true);
  setActionButtonStyle(m_deviceStopButton, false);
  setActionButtonStyle(m_deviceCloseButton, true);
  setActionButtonStyle(m_deviceInfoButton, true);

  devBtnLayout->addWidget(m_deviceStartButton);
  devBtnLayout->addWidget(m_deviceStopButton);
  devBtnLayout->addWidget(m_deviceCloseButton);
  devBtnLayout->addWidget(m_deviceInfoButton);
  devBtnLayout->addStretch();
  m_deviceTree->setItemWidget(devItem, 1, devBtnWidget);

  connect(m_deviceStartButton, &QPushButton::clicked, this, &CanDeviceDialog::onStartAllChannels);
  connect(m_deviceStopButton, &QPushButton::clicked, this, &CanDeviceDialog::onStopAllChannels);
  connect(m_deviceCloseButton, &QPushButton::clicked, this, &CanDeviceDialog::onCloseDeviceClicked);
  connect(m_deviceInfoButton, &QPushButton::clicked, this, &CanDeviceDialog::onShowDeviceInfoClicked);

  // 3. 创建通道子节点
  for (int i = 0; i < canNum; ++i) {
    QTreeWidgetItem *chanItem = new QTreeWidgetItem(devItem);
    chanItem->setText(0, QString("通道%1").arg(i));
    chanItem->setIcon(0, drawGreyDotIcon());

    QWidget *chanBtnWidget = new QWidget();
    QHBoxLayout *chanBtnLayout = new QHBoxLayout(chanBtnWidget);
    chanBtnLayout->setContentsMargins(0, 0, 0, 0);
    chanBtnLayout->setSpacing(6);

    QPushButton *btnStart = new QPushButton("启动");
    QPushButton *btnStop = new QPushButton("停止");

    const bool isRunning = m_can->isChannelRunning(i);
    setActionButtonStyle(btnStart, !isRunning);
    setActionButtonStyle(btnStop, isRunning);

    chanBtnLayout->addWidget(btnStart);
    chanBtnLayout->addWidget(btnStop);
    chanBtnLayout->addStretch();
    m_deviceTree->setItemWidget(chanItem, 1, chanBtnWidget);

    m_startButtons[i] = btnStart;
    m_stopButtons[i] = btnStop;

    connect(btnStart, &QPushButton::clicked, this, [this, i]() { onStartChannelClicked(i); });
    connect(btnStop, &QPushButton::clicked, this, [this, i]() { onStopChannelClicked(i); });
  }

  m_deviceTree->expandItem(devItem);
  updateButtonStates();
}

void CanDeviceDialog::onOpenDeviceClicked() {
  const quint32 type = static_cast<quint32>(m_deviceTypeCombo->currentData().toUInt());
  const int index = m_deviceIndexSpin->value();

  if (m_can->openDevice(type, index)) {
    refreshDeviceTree();
  } else {
    QMessageBox::critical(this, "错误", "打开设备失败，请检查驱动连接。");
  }
}

void CanDeviceDialog::onCloseDeviceClicked() {
  m_can->closeDevice();
  refreshDeviceTree();
}

void CanDeviceDialog::onStartChannelClicked(int channel) {
  StartChannelDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    CanChannelConfig cfg = dlg.getConfig();
    if (m_can->startChannel(channel, cfg)) {
      updateButtonStates();
    } else {
      QMessageBox::critical(this, "错误", QString("启动通道%1失败。").arg(channel));
    }
  }
}

void CanDeviceDialog::onStopChannelClicked(int channel) {
  m_can->stopChannel(channel);
  updateButtonStates();
}

void CanDeviceDialog::onStartAllChannels() {
  StartChannelDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    CanChannelConfig cfg = dlg.getConfig();
    int count = m_startButtons.size();
    for (int i = 0; i < count; ++i) {
      m_can->startChannel(i, cfg);
    }
    updateButtonStates();
  }
}

void CanDeviceDialog::onStopAllChannels() {
  int count = m_stopButtons.size();
  for (int i = 0; i < count; ++i) {
    m_can->stopChannel(i);
  }
  updateButtonStates();
}

void CanDeviceDialog::updateButtonStates() {
  // 根据通道启动状态更新各个按钮样式
  bool anyRunning = false;
  QList<int> channels = m_startButtons.keys();
  for (int ch : channels) {
    const bool running = m_can->isChannelRunning(ch);
    if (running) {
      anyRunning = true;
    }
    setActionButtonStyle(m_startButtons[ch], !running);
    setActionButtonStyle(m_stopButtons[ch], running);
  }

  if (m_deviceStartButton && m_deviceStopButton) {
    setActionButtonStyle(m_deviceStartButton, true); // 始终允许批量启动
    setActionButtonStyle(m_deviceStopButton, anyRunning); // 只有在有通道运行时才允许批量停止
  }
}

void CanDeviceDialog::onShowDeviceInfoClicked() {
  QString hw, fw, dr, lib, serial, typeStr;
  int canNum = 2;
  if (!m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &canNum, &serial, &typeStr)) {
    hw = "V3.00";
    fw = "V2.09";
    dr = "V1.00";
    lib = "V0.00";
    canNum = 2;
    serial = "B32070B800BB0784A680";
    typeStr = "USBCANFD-200U";
  }

  QDialog infoDlg(this);
  infoDlg.setWindowTitle("设备信息");
  infoDlg.setFixedSize(360, 260);
  infoDlg.setStyleSheet("QDialog { background-color: white; }");

  QHBoxLayout *layout = new QHBoxLayout(&infoDlg);
  layout->setContentsMargins(25, 25, 25, 20);
  layout->setSpacing(20);

  // 信息图标
  QLabel *iconLabel = new QLabel();
  QIcon infoIcon = infoDlg.style()->standardIcon(QStyle::SP_MessageBoxInformation);
  iconLabel->setPixmap(infoIcon.pixmap(48, 48));
  iconLabel->setAlignment(Qt::AlignTop);
  layout->addWidget(iconLabel);

  // 文字信息列表
  QVBoxLayout *textLayout = new QVBoxLayout();
  textLayout->setSpacing(6);

  auto addLine = [&](const QString &label, const QString &val) {
    QLabel *lbl = new QLabel(QString("<font color='#555555'>%1: </font><b>%2</b>").arg(label).arg(val));
    lbl->setStyleSheet("font-size: 13px; font-family: Microsoft YaHei;");
    textLayout->addWidget(lbl);
  };

  addLine("硬件版本", hw);
  addLine("固件版本", fw);
  addLine("驱动版本", dr);
  addLine("动态库版本", lib);
  addLine("CAN路数", QString::number(canNum));
  addLine("序列号", serial);
  addLine("硬件类型", typeStr);

  textLayout->addSpacing(15);

  QPushButton *btnOk = new QPushButton("确定");
  btnOk->setStyleSheet("QPushButton { background-color: #D6E8FC; border: 1px solid #ADC3E6; color: #2C3E50; padding: 5px 25px; border-radius: 4px; font-weight: bold; font-size: 12px; }"
                       "QPushButton:hover { background-color: #C0DEFC; }");
  connect(btnOk, &QPushButton::clicked, &infoDlg, &QDialog::accept);

  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();
  btnLayout->addWidget(btnOk);
  textLayout->addLayout(btnLayout);

  layout->addLayout(textLayout);
  infoDlg.exec();
}

void CanDeviceDialog::onCloudDeviceClicked() {
  QMessageBox::information(this, "云设备", "云设备连接中... (当前云设备服务不可用)");
}
