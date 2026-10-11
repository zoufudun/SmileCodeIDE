#include "candevicedialog.h"

#include <QCheckBox>
#include <QApplication>
#include <QClipboard>
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
#include <QTableWidget>
#include <QHeaderView>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QListView>
#include <QStandardItemModel>
#include <QStackedWidget>

#include "caninterface.h"
#include "USBCANFD/zlgcan.h"

namespace {

struct DeviceDefinition {
  QString name;
  quint32 type;
  QString vendor;
};

const QList<DeviceDefinition> kDeviceDefinitions = {
  // 软件仿真
  {"🌟 纯软件仿真 (免硬件全流程测试)", CAN_DEV_SIMULATION, "软件仿真"},
  {"虚拟 CAN 设备 (VirtualUSBCAN)", ZCAN_VIRTUAL_DEVICE, "软件仿真"},

  // 周立功 ZLG
  {"USBCANFD-200U", ZCAN_USBCANFD_200U, "周立功 ZLG"},
  {"USBCANFD-100U", ZCAN_USBCANFD_100U, "周立功 ZLG"},
  {"USBCANFD-MINI", ZCAN_USBCANFD_MINI, "周立功 ZLG"},
  {"USBCANFD-800U", ZCAN_USBCANFD_800U, "周立功 ZLG"},
  {"USBCAN-4E-U", ZCAN_USBCAN_4E_U, "周立功 ZLG"},
  {"USBCAN-2E-U", ZCAN_USBCAN_2E_U, "周立功 ZLG"},
  {"USBCAN-2", ZCAN_USBCAN2, "周立功 ZLG"},
  {"USBCAN-1", ZCAN_USBCAN1, "周立功 ZLG"},

  // 创芯科技
  {"创芯 USBCAN-2C", CX_USBCAN2, "创芯科技"},
  {"创芯 USBCAN-1C", CX_USBCAN1, "创芯科技"},
  {"创芯 USBCAN-2E-U", CX_USBCAN_2E_U, "创芯科技"},
  {"创芯 USBCAN-E-U", CX_USBCAN_E_U, "创芯科技"},

  // Qt SerialBus
  {"PEAK PCAN 硬件卡 (Qt peakcan)", CAN_DEV_QT_PEAKCAN, "Qt SerialBus"},
  {"Vector VN 系列 (Qt vectorcan)", CAN_DEV_QT_VECTORCAN, "Qt SerialBus"},
  {"SAE J2534 PassThru (Qt passthrucan)", CAN_DEV_QT_PASSTHRUCAN, "Qt SerialBus"},
  {"Qt 虚拟 CAN (Qt virtualcan)", CAN_DEV_QT_VIRTUALCAN, "Qt SerialBus"},
  {"SYS TEC USB-CAN (Qt systeccan)", CAN_DEV_QT_SYSTECCAN, "Qt SerialBus"},
  {"MHS TinyCAN (Qt tinycan)", CAN_DEV_QT_TINYCAN, "Qt SerialBus"},
  {"Linux SocketCAN (Qt socketcan)", CAN_DEV_QT_SOCKETCAN, "Qt SerialBus"},

  // 同星智能
  {"同星智能 TOSUN (TSMaster)", CAN_DEV_TOSUN_TSMASTER, "同星智能"}
};

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
  btn->setStyleSheet("QPushButton { font-size: 11px; padding: 4px 10px; border-radius: 4px; min-width: 50px; font-family: 'Microsoft YaHei'; }");
}

QString getParentStyleSheet(QWidget *w) {
  while (w) {
    if (!w->styleSheet().isEmpty()) {
      return w->styleSheet();
    }
    w = w->parentWidget();
  }
  return "";
}
} // namespace

// --- 通道滤波设置对话框 ---
class FilterSettingsDialog : public QDialog {
public:
  FilterSettingsDialog(QList<CanFilterRule> *rules, int channel, QWidget *parent = nullptr)
      : QDialog(parent), m_rules(rules), m_channel(channel) {
    setWindowTitle(QString("通道%1 滤波设置").arg(channel));
    setMinimumSize(480, 360);
    setStyleSheet("");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // 1. 过滤规则表格
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"过滤格式", "起始帧ID", "结束帧ID"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setStyleSheet("QTableWidget { gridline-color: palette(mid); }");
    mainLayout->addWidget(m_table);

    // 2. 规则控制栏
    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    ctrlLayout->setSpacing(8);

    ctrlLayout->addWidget(new QLabel("模式:"));
    m_modeCombo = new QComboBox();
    m_modeCombo->setView(new QListView(m_modeCombo));
    m_modeCombo->addItems({"标准帧明确ID", "扩展帧明确ID", "标准帧段ID", "扩展帧段ID"});
    ctrlLayout->addWidget(m_modeCombo);

    ctrlLayout->addWidget(new QLabel("起始ID: 0x"));
    m_startIdEdit = new QLineEdit("0");
    m_startIdEdit->setMaxLength(8);
    m_startIdEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9a-fA-F]{1,8}$"), this));
    m_startIdEdit->setFixedWidth(70);
    ctrlLayout->addWidget(m_startIdEdit);

    ctrlLayout->addWidget(new QLabel("结束ID: 0x"));
    m_endIdEdit = new QLineEdit("0");
    m_endIdEdit->setMaxLength(8);
    m_endIdEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9a-fA-F]{1,8}$"), this));
    m_endIdEdit->setFixedWidth(70);
    m_endIdEdit->setEnabled(false);
    ctrlLayout->addWidget(m_endIdEdit);

    mainLayout->addLayout(ctrlLayout);

    // 3. 按钮栏
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnAdd = new QPushButton("添加");
    m_btnDelete = new QPushButton("删除");
    m_btnOk = new QPushButton("确定");
    m_btnCancel = new QPushButton("取消");

    m_btnAdd->setStyleSheet("QPushButton { padding: 4px 12px; }");
    m_btnDelete->setStyleSheet("QPushButton { padding: 4px 12px; }");
    m_btnOk->setStyleSheet("QPushButton { padding: 5px 20px; }");
    m_btnCancel->setStyleSheet("QPushButton { padding: 5px 20px; }");

    m_btnDelete->setEnabled(false);

    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOk);
    btnLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(btnLayout);

    // 4. 事件连接
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterSettingsDialog::onModeChanged);
    connect(m_btnAdd, &QPushButton::clicked, this, &FilterSettingsDialog::onAddClicked);
    connect(m_btnDelete, &QPushButton::clicked, this, &FilterSettingsDialog::onDeleteClicked);
    connect(m_btnOk, &QPushButton::clicked, this, &FilterSettingsDialog::onOkClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
      m_btnDelete->setEnabled(!m_table->selectedItems().isEmpty());
    });

    // 加载已有滤波规则
    loadFilters();
  }

private slots:
  void onModeChanged(int idx) {
    bool isSegment = (idx == 2 || idx == 3);
    m_endIdEdit->setEnabled(isSegment);
    if (!isSegment) {
      m_endIdEdit->setText("0");
    }
  }

  void onAddClicked() {
    QString startStr = m_startIdEdit->text().trimmed();
    QString endStr = m_endIdEdit->text().trimmed();
    if (startStr.isEmpty()) startStr = "0";
    if (endStr.isEmpty()) endStr = "0";

    bool ok1, ok2;
    quint32 startId = startStr.toUInt(&ok1, 16);
    quint32 endId = endStr.toUInt(&ok2, 16);

    if (!ok1 || !ok2) {
      QMessageBox::warning(this, "警告", "帧ID输入格式不正确！");
      return;
    }

    int modeIdx = m_modeCombo->currentIndex();
    bool isExtended = (modeIdx == 1 || modeIdx == 3);
    quint32 maxId = isExtended ? 0x1FFFFFFF : 0x7FF;

    if (startId > maxId || endId > maxId) {
      QMessageBox::warning(this, "警告", QString("帧ID超出范围！%1帧最大ID为 0x%2").arg(isExtended ? "扩展" : "标准").arg(maxId, 0, 16).toUpper());
      return;
    }

    bool isSegment = (modeIdx == 2 || modeIdx == 3);
    if (isSegment && startId > endId) {
      QMessageBox::warning(this, "警告", "起始ID不能大于结束ID！");
      return;
    }

    addTableRow(m_modeCombo->currentText(), "0x" + QString::number(startId, 16).toUpper(), isSegment ? "0x" + QString::number(endId, 16).toUpper() : "-");
  }

  void onDeleteClicked() {
    int row = m_table->currentRow();
    if (row >= 0) {
      m_table->removeRow(row);
    }
  }

  void onOkClicked() {
    QList<CanFilterRule> rules;
    for (int r = 0; r < m_table->rowCount(); ++r) {
      CanFilterRule rule;
      QString mStr = m_table->item(r, 0)->text();
      if (mStr == "标准帧明确ID") rule.mode = 0;
      else if (mStr == "扩展帧明确ID") rule.mode = 1;
      else if (mStr == "标准帧段ID") rule.mode = 2;
      else if (mStr == "扩展帧段ID") rule.mode = 3;

      rule.startId = m_table->item(r, 1)->text().toUInt(nullptr, 16);
      if (m_table->item(r, 2)->text() == "-") {
        rule.endId = 0;
      } else {
        rule.endId = m_table->item(r, 2)->text().toUInt(nullptr, 16);
      }
      rules.append(rule);
    }

    *m_rules = rules;
    accept();
  }

private:
  void addTableRow(const QString &mode, const QString &start, const QString &end) {
    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem(mode));
    m_table->setItem(row, 1, new QTableWidgetItem(start));
    m_table->setItem(row, 2, new QTableWidgetItem(end));
    // Center items
    m_table->item(row, 0)->setTextAlignment(Qt::AlignCenter);
    m_table->item(row, 1)->setTextAlignment(Qt::AlignCenter);
    m_table->item(row, 2)->setTextAlignment(Qt::AlignCenter);
  }

  void loadFilters() {
    QList<CanFilterRule> rules = *m_rules;
    for (const auto &rule : rules) {
      QString modeStr;
      if (rule.mode == 0) modeStr = "标准帧明确ID";
      else if (rule.mode == 1) modeStr = "扩展帧明确ID";
      else if (rule.mode == 2) modeStr = "标准帧段ID";
      else if (rule.mode == 3) modeStr = "扩展帧段ID";

      bool isSegment = (rule.mode == 2 || rule.mode == 3);
      addTableRow(modeStr, "0x" + QString::number(rule.startId, 16).toUpper(), isSegment ? "0x" + QString::number(rule.endId, 16).toUpper() : "-");
    }
  }

  QList<CanFilterRule> *m_rules;
  int m_channel;
  QTableWidget *m_table;
  QComboBox *m_modeCombo;
  QLineEdit *m_startIdEdit;
  QLineEdit *m_endIdEdit;
  QPushButton *m_btnAdd;
  QPushButton *m_btnDelete;
  QPushButton *m_btnOk;
  QPushButton *m_btnCancel;
};

// --- 启动通道参数配置对话框 ---
class StartChannelDialog : public QDialog {
public:
  explicit StartChannelDialog(bool fdCapable, int channel, QWidget *parent = nullptr)
      : QDialog(parent), m_fdCapable(fdCapable), m_channel(channel) {
    setWindowTitle("启动");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::WindowMinMaxButtonsHint);
    setMinimumSize(440, 520);
    setStyleSheet("");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->setSpacing(12);

    // 波特率计算器按钮
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addStretch();
    QPushButton *btnCalc = new QPushButton("波特率计算器");
    btnCalc->setStyleSheet("QPushButton { padding: 3px 10px; font-size: 11px; }");
    topLayout->addWidget(btnCalc);
    mainLayout->addLayout(topLayout);

    // 协议选择
    QFormLayout *protoForm = new QFormLayout();
    protoForm->setContentsMargins(10, 0, 10, 0);
    m_protocolCombo = new QComboBox();
    m_protocolCombo->setView(new QListView(m_protocolCombo));
    if (m_fdCapable) {
      m_protocolCombo->addItems({"CAN FD", "CAN"});
    } else {
      m_protocolCombo->addItem("CAN");
      m_protocolCombo->setEnabled(false);
    }
    protoForm->addRow("协议", m_protocolCombo);
    mainLayout->addLayout(protoForm);

    // 滚动区域放置具体配置页
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->setStyleSheet("background: transparent;");

    // ==========================================
    // 页面 0: 纯 CAN 参数配置页 (只显示 CAN 的配置)
    // ==========================================
    m_canPage = new QWidget();
    m_canPage->setStyleSheet("background: transparent;");
    QFormLayout *canForm = new QFormLayout(m_canPage);
    canForm->setSpacing(10);
    canForm->setContentsMargins(10, 5, 10, 5);

    m_canBaudCombo = new QComboBox();
    m_canBaudCombo->setView(new QListView(m_canBaudCombo));
    m_canBaudCombo->addItem("1Mbps 80%", 1000000);
    m_canBaudCombo->addItem("800kbps 80%", 800000);
    m_canBaudCombo->addItem("500kbps 80%", 500000);
    m_canBaudCombo->addItem("250kbps 80%", 250000);
    m_canBaudCombo->addItem("125kbps 80%", 125000);
    m_canBaudCombo->addItem("100kbps 80%", 100000);
    m_canBaudCombo->addItem("50kbps 80%", 50000);
    m_canBaudCombo->addItem("20kbps 80%", 20000);
    m_canBaudCombo->addItem("10kbps 80%", 10000);
    m_canBaudCombo->setCurrentText("500kbps 80%");

    m_canCustomBaudEdit = new QLineEdit();
    m_canCustomBaudEdit->setPlaceholderText("未配置");

    m_canWorkModeCombo = new QComboBox();
    m_canWorkModeCombo->setView(new QListView(m_canWorkModeCombo));
    m_canWorkModeCombo->addItems({"正常模式", "只听模式"});

    m_canTerminalResCombo = new QComboBox();
    m_canTerminalResCombo->setView(new QListView(m_canTerminalResCombo));
    m_canTerminalResCombo->addItems({"使能", "禁用"});

    m_canRetryCombo = new QComboBox();
    m_canRetryCombo->setView(new QListView(m_canRetryCombo));
    m_canRetryCombo->addItem("发送到总线关闭", 1);
    m_canRetryCombo->addItem("重试3次", 3);
    m_canRetryCombo->addItem("不重试", 0);

    m_canReportBusCombo = new QComboBox();
    m_canReportBusCombo->setView(new QListView(m_canReportBusCombo));
    m_canReportBusCombo->addItems({"禁用", "使能"});

    m_canBusPeriodEdit = new QLineEdit("100");

    canForm->addRow("波特率", m_canBaudCombo);
    canForm->addRow("自定义波特率", m_canCustomBaudEdit);
    canForm->addRow("工作模式", m_canWorkModeCombo);
    if (m_fdCapable) {
      canForm->addRow("终端电阻", m_canTerminalResCombo);
      canForm->addRow("发送重试", m_canRetryCombo);
      canForm->addRow("上报总线利用率", m_canReportBusCombo);
      canForm->addRow("总线利用率周期(ms)", m_canBusPeriodEdit);
    }
    m_stackedWidget->addWidget(m_canPage);

    // ==========================================
    // 页面 1: CAN FD 参数配置页 (显示所有 CANFD 配置)
    // ==========================================
    m_fdPage = new QWidget();
    m_fdPage->setStyleSheet("background: transparent;");
    QFormLayout *fdForm = new QFormLayout(m_fdPage);
    fdForm->setSpacing(10);
    fdForm->setContentsMargins(10, 5, 10, 5);

    m_fdStandardCombo = new QComboBox();
    m_fdStandardCombo->setView(new QListView(m_fdStandardCombo));
    m_fdStandardCombo->addItems({"CAN FD ISO", "CAN FD Non-ISO"});

    m_fdAccelCombo = new QComboBox();
    m_fdAccelCombo->setView(new QListView(m_fdAccelCombo));
    m_fdAccelCombo->addItems({"是", "否"});

    m_fdAbitBaudCombo = new QComboBox();
    m_fdAbitBaudCombo->setView(new QListView(m_fdAbitBaudCombo));
    m_fdAbitBaudCombo->addItem("1Mbps 80%", 1000000);
    m_fdAbitBaudCombo->addItem("800kbps 80%", 800000);
    m_fdAbitBaudCombo->addItem("500kbps 80%", 500000);
    m_fdAbitBaudCombo->addItem("250kbps 80%", 250000);
    m_fdAbitBaudCombo->addItem("125kbps 80%", 125000);
    m_fdAbitBaudCombo->addItem("100kbps 80%", 100000);
    m_fdAbitBaudCombo->addItem("50kbps 80%", 50000);
    m_fdAbitBaudCombo->setCurrentText("500kbps 80%");

    m_fdDbitBaudCombo = new QComboBox();
    m_fdDbitBaudCombo->setView(new QListView(m_fdDbitBaudCombo));
    m_fdDbitBaudCombo->addItem("5Mbps 75%", 5000000);
    m_fdDbitBaudCombo->addItem("4Mbps 75%", 4000000);
    m_fdDbitBaudCombo->addItem("2Mbps 75%", 2000000);
    m_fdDbitBaudCombo->addItem("1Mbps 75%", 1000000);
    m_fdDbitBaudCombo->setCurrentText("2Mbps 75%");

    m_fdCustomBaudEdit = new QLineEdit();
    m_fdCustomBaudEdit->setPlaceholderText("未配置");

    m_fdWorkModeCombo = new QComboBox();
    m_fdWorkModeCombo->setView(new QListView(m_fdWorkModeCombo));
    m_fdWorkModeCombo->addItems({"正常模式", "只听模式"});

    m_fdTerminalResCombo = new QComboBox();
    m_fdTerminalResCombo->setView(new QListView(m_fdTerminalResCombo));
    m_fdTerminalResCombo->addItems({"使能", "禁用"});

    m_fdRetryCombo = new QComboBox();
    m_fdRetryCombo->setView(new QListView(m_fdRetryCombo));
    m_fdRetryCombo->addItem("发送到总线关闭", 1);
    m_fdRetryCombo->addItem("重试3次", 3);
    m_fdRetryCombo->addItem("不重试", 0);

    m_fdReportBusCombo = new QComboBox();
    m_fdReportBusCombo->setView(new QListView(m_fdReportBusCombo));
    m_fdReportBusCombo->addItems({"禁用", "使能"});

    m_fdBusPeriodEdit = new QLineEdit("100");

    fdForm->addRow("CANFD标准", m_fdStandardCombo);
    fdForm->addRow("CANFD加速", m_fdAccelCombo);
    fdForm->addRow("仲裁域波特率", m_fdAbitBaudCombo);
    fdForm->addRow("数据域波特率", m_fdDbitBaudCombo);
    fdForm->addRow("自定义波特率", m_fdCustomBaudEdit);
    fdForm->addRow("工作模式", m_fdWorkModeCombo);
    fdForm->addRow("终端电阻", m_fdTerminalResCombo);
    fdForm->addRow("发送重试", m_fdRetryCombo);
    fdForm->addRow("上报总线利用率", m_fdReportBusCombo);
    fdForm->addRow("总线利用率周期(ms)", m_fdBusPeriodEdit);

    m_stackedWidget->addWidget(m_fdPage);

    // 协议联动切换
    auto switchProtocol = [this](const QString &proto) {
      if (proto == "CAN FD" && m_fdCapable) {
        m_stackedWidget->setCurrentWidget(m_fdPage);
      } else {
        m_stackedWidget->setCurrentWidget(m_canPage);
      }
      this->adjustSize();
    };

    connect(m_protocolCombo, &QComboBox::currentTextChanged, this, switchProtocol);
    switchProtocol(m_protocolCombo->currentText());

    scroll->setWidget(m_stackedWidget);
    mainLayout->addWidget(scroll);

    // 滤波设置区域
    QHBoxLayout *filterLayout = new QHBoxLayout();
    m_filterCheck = new QCheckBox("滤波");
    m_filterSetupBtn = new QPushButton("滤波设置");
    m_filterSetupBtn->setEnabled(false);
    m_filterSetupBtn->setStyleSheet("QPushButton { padding: 3px 8px; font-size: 11px; }");
    connect(m_filterCheck, &QCheckBox::toggled, m_filterSetupBtn, [this](bool checked) {
      m_filterSetupBtn->setEnabled(checked);
    });
    connect(m_filterSetupBtn, &QPushButton::clicked, this, [this]() {
      FilterSettingsDialog dlg(&m_tempRules, m_channel, this);
      QString currentQss = getParentStyleSheet(this);
      if (!currentQss.isEmpty()) {
        dlg.setStyleSheet(currentQss);
      }
      dlg.exec();
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

    btnOk->setStyleSheet("QPushButton { padding: 5px 25px; }");
    btnCancel->setStyleSheet("QPushButton { padding: 5px 25px; }");

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
    cfg.isFd = m_fdCapable && (m_protocolCombo->currentText() == "CAN FD");
    if (cfg.isFd) {
      cfg.isIso = m_fdStandardCombo->currentText() == "CAN FD ISO";
      cfg.enableBrs = m_fdAccelCombo->currentText() == "是";

      QString customBaud = m_fdCustomBaudEdit->text().trimmed();
      if (!customBaud.isEmpty()) {
        cfg.abitBaud = customBaud.toInt();
      } else {
        cfg.abitBaud = m_fdAbitBaudCombo->currentData().toInt();
      }

      cfg.dbitBaud = m_fdDbitBaudCombo->currentData().toInt();
      cfg.mode = m_fdWorkModeCombo->currentText() == "只听模式" ? CanMode::ListenOnly : CanMode::Normal;
      cfg.terminalRes = m_fdTerminalResCombo->currentText() == "使能";
      cfg.reportBusUsage = m_fdReportBusCombo->currentText() == "使能";
      cfg.busUsagePeriod = m_fdBusPeriodEdit->text().toInt();
      cfg.retrySend = m_fdRetryCombo->currentData().toInt();
    } else {
      cfg.isIso = false;
      cfg.enableBrs = false;

      QString customBaud = m_canCustomBaudEdit->text().trimmed();
      if (!customBaud.isEmpty()) {
        cfg.abitBaud = customBaud.toInt();
      } else {
        cfg.abitBaud = m_canBaudCombo->currentData().toInt();
      }

      cfg.dbitBaud = 0;
      cfg.mode = m_canWorkModeCombo->currentText() == "只听模式" ? CanMode::ListenOnly : CanMode::Normal;
      cfg.terminalRes = m_canTerminalResCombo->currentText() == "使能";
      cfg.reportBusUsage = m_canReportBusCombo->currentText() == "使能";
      cfg.busUsagePeriod = m_canBusPeriodEdit->text().toInt();
      cfg.retrySend = m_canRetryCombo->currentData().toInt();
    }

    cfg.enableFilter = m_filterCheck->isChecked();
    return cfg;
  }

  QList<CanFilterRule> getFilterRules() const { return m_tempRules; }

private:
  bool m_fdCapable;
  int m_channel;
  QList<CanFilterRule> m_tempRules;
  QComboBox *m_protocolCombo;
  QStackedWidget *m_stackedWidget;

  // 纯 CAN 配置组件
  QWidget *m_canPage;
  QComboBox *m_canBaudCombo;
  QLineEdit *m_canCustomBaudEdit;
  QComboBox *m_canWorkModeCombo;
  QComboBox *m_canTerminalResCombo;
  QComboBox *m_canRetryCombo;
  QComboBox *m_canReportBusCombo;
  QLineEdit *m_canBusPeriodEdit;

  // CAN FD 配置组件
  QWidget *m_fdPage;
  QComboBox *m_fdStandardCombo;
  QComboBox *m_fdAccelCombo;
  QComboBox *m_fdAbitBaudCombo;
  QComboBox *m_fdDbitBaudCombo;
  QLineEdit *m_fdCustomBaudEdit;
  QComboBox *m_fdWorkModeCombo;
  QComboBox *m_fdTerminalResCombo;
  QComboBox *m_fdRetryCombo;
  QComboBox *m_fdReportBusCombo;
  QLineEdit *m_fdBusPeriodEdit;

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
  setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
  setMinimumSize(960, 580);
  resize(1120, 680);
  setStyleSheet("");

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(15, 15, 15, 15);
  mainLayout->setSpacing(10);

  // 第一行: 设备选择与主连接操作
  QHBoxLayout *devRow = new QHBoxLayout();
  devRow->setSpacing(8);

  devRow->addWidget(new QLabel("厂商:"));
  m_vendorFilterCombo = new QComboBox();
  m_vendorFilterCombo->setView(new QListView(m_vendorFilterCombo));
  m_vendorFilterCombo->addItems({"全部厂商", "软件仿真", "周立功 ZLG", "创芯科技", "Qt SerialBus", "同星智能"});
  m_vendorFilterCombo->setMinimumWidth(110);
  devRow->addWidget(m_vendorFilterCombo);

  devRow->addWidget(new QLabel("搜索:"));
  m_searchEdit = new QLineEdit();
  m_searchEdit->setPlaceholderText("型号或接口名称");
  m_searchEdit->setClearButtonEnabled(true);
  m_searchEdit->setMinimumWidth(125);
  m_searchEdit->setMaximumWidth(180);
  devRow->addWidget(m_searchEdit);

  devRow->addWidget(new QLabel("型号:"));
  m_deviceTypeCombo = new QComboBox();
  m_deviceTypeCombo->setView(new QListView(m_deviceTypeCombo));
  m_deviceTypeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_deviceTypeCombo->setMinimumWidth(220);
  devRow->addWidget(m_deviceTypeCombo, 1);

  devRow->addWidget(new QLabel("索引:"));
  m_deviceIndexSpin = new QSpinBox();
  m_deviceIndexSpin->setRange(0, 7);
  m_deviceIndexSpin->setFixedWidth(50);
  devRow->addWidget(m_deviceIndexSpin);

  m_openButton = new QPushButton("打开设备");
  m_openButton->setMinimumWidth(85);
  devRow->addWidget(m_openButton);

  m_detectButton = new QPushButton("检测接入设备");
  m_detectButton->setMinimumWidth(105);
  devRow->addWidget(m_detectButton);

  mainLayout->addLayout(devRow);

  // 第二行: 工具操作栏
  QHBoxLayout *toolRow = new QHBoxLayout();
  toolRow->setSpacing(8);

  m_refreshButton = new QPushButton("刷新");
  m_refreshButton->setMinimumWidth(60);
  toolRow->addWidget(m_refreshButton);

  m_expandButton = new QPushButton("全部展开");
  m_expandButton->setMinimumWidth(75);
  toolRow->addWidget(m_expandButton);

  m_collapseButton = new QPushButton("全部收起");
  m_collapseButton->setMinimumWidth(75);
  toolRow->addWidget(m_collapseButton);

  m_copyConfigButton = new QPushButton("复制配置");
  m_copyConfigButton->setMinimumWidth(75);
  toolRow->addWidget(m_copyConfigButton);

  m_cloudButton = new QPushButton("云设备");
  m_cloudButton->setMinimumWidth(65);
  toolRow->addWidget(m_cloudButton);

  toolRow->addStretch();

  m_closeButton = new QPushButton("关闭窗口");
  m_closeButton->setMinimumWidth(75);
  toolRow->addWidget(m_closeButton);

  mainLayout->addLayout(toolRow);

  // 接入设备实时检测提示条
  m_detectBannerLabel = new QLabel();
  m_detectBannerLabel->setWordWrap(true);
  mainLayout->addWidget(m_detectBannerLabel);

  m_summaryLabel = new QLabel();
  m_summaryLabel->setStyleSheet("color: palette(mid); padding: 2px 4px;");
  mainLayout->addWidget(m_summaryLabel);

  // 中部 QTreeWidget 视图列表
  m_deviceTree = new QTreeWidget();
  m_deviceTree->setColumnCount(2);
  m_deviceTree->setHeaderLabels({"设备目录（按厂商分组）", "状态与操作"});
  m_deviceTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
  m_deviceTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_deviceTree->setColumnWidth(0, 360);
  m_deviceTree->setIndentation(20);
  m_deviceTree->setStyleSheet("QTreeWidget::item { height: 36px; }");
  mainLayout->addWidget(m_deviceTree);

  // 按照厂商分类填充型号下拉列表，并自动检测当前接入设备
  populateDeviceTypeCombo(0);
  detectAndPromptDevices(true);

  // 绑定连接
  connect(m_openButton, &QPushButton::clicked, this, &CanDeviceDialog::onOpenDeviceClicked);
  connect(m_detectButton, &QPushButton::clicked, this, &CanDeviceDialog::onDetectDevicesClicked);
  connect(m_cloudButton, &QPushButton::clicked, this, &CanDeviceDialog::onCloudDeviceClicked);
  connect(m_refreshButton, &QPushButton::clicked, this, &CanDeviceDialog::onRefreshClicked);
  connect(m_expandButton, &QPushButton::clicked, this, &CanDeviceDialog::onExpandAllClicked);
  connect(m_collapseButton, &QPushButton::clicked, this, &CanDeviceDialog::onCollapseAllClicked);
  connect(m_copyConfigButton, &QPushButton::clicked, this, &CanDeviceDialog::onCopyConfigClicked);
  connect(m_deviceTree, &QTreeWidget::itemDoubleClicked, this, &CanDeviceDialog::onTreeDeviceActivated);
  connect(m_searchEdit, &QLineEdit::textChanged, this, &CanDeviceDialog::onSearchTextChanged);
  connect(m_vendorFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CanDeviceDialog::onVendorFilterChanged);
  connect(m_closeButton, &QPushButton::clicked, this, &QWidget::close);
}

void CanDeviceDialog::populateDeviceTypeCombo(int vendorFilterIdx) {
  const quint32 previousType = m_deviceTypeCombo->currentData().toUInt();
  QStandardItemModel *model = new QStandardItemModel(m_deviceTypeCombo);

  QString targetVendor = "全部厂商";
  if (vendorFilterIdx > 0 && vendorFilterIdx < m_vendorFilterCombo->count()) {
    targetVendor = m_vendorFilterCombo->itemText(vendorFilterIdx);
  }

  QStringList vendors;
  if (targetVendor == "全部厂商") {
    vendors << "软件仿真" << "周立功 ZLG" << "创芯科技" << "Qt SerialBus" << "同星智能";
  } else {
    vendors << targetVendor;
  }

  QList<CanDetectedDevice> detected = m_can->detectConnectedDevices();
  QSet<quint32> detectedHwTypes;
  for (const auto &d : detected) {
    if (d.isHardware) {
      detectedHwTypes.insert(d.deviceType);
    }
  }

  int selectIndex = -1;
  int currentItemIndex = 0;

  for (const QString &v : vendors) {
    // 厂商分类标题项（不可选中、不可高亮，充当分类 Header）
    QStandardItem *headerItem = new QStandardItem(QString("── %1 ──").arg(v));
    headerItem->setSelectable(false);
    headerItem->setEnabled(false);
    QFont font = headerItem->font();
    font.setBold(true);
    headerItem->setFont(font);
    headerItem->setForeground(QBrush(QColor("#10B981")));
    model->appendRow(headerItem);
    ++currentItemIndex;

    for (const auto &dev : kDeviceDefinitions) {
      if (dev.vendor != v) continue;

      QString displayName = "   " + dev.name;
      if (detectedHwTypes.contains(dev.type)) {
        displayName += "  [✔ 已接入]";
      }

      QStandardItem *item = new QStandardItem(displayName);
      item->setData(dev.type, Qt::UserRole);
      if (detectedHwTypes.contains(dev.type)) {
        item->setForeground(QBrush(QColor("#10B981")));
        QFont itemFont = item->font();
        itemFont.setBold(true);
        item->setFont(itemFont);
      }
      model->appendRow(item);

      if (dev.type == previousType) {
        selectIndex = currentItemIndex;
      }
      ++currentItemIndex;
    }
  }

  m_deviceTypeCombo->setModel(model);
  if (selectIndex >= 0) {
    m_deviceTypeCombo->setCurrentIndex(selectIndex);
  } else {
    // 默认选择第一个可选项（跳过分类标题）
    for (int i = 0; i < model->rowCount(); ++i) {
      if (!model->item(i)->data(Qt::UserRole).isNull()) {
        m_deviceTypeCombo->setCurrentIndex(i);
        break;
      }
    }
  }
}

void CanDeviceDialog::detectAndPromptDevices(bool autoSelectFirstHardware) {
  QList<CanDetectedDevice> detected = m_can->detectConnectedDevices();

  CanDetectedDevice firstHw;
  bool hasHw = false;
  for (const auto &d : detected) {
    if (d.isHardware) {
      firstHw = d;
      hasHw = true;
      break;
    }
  }

  if (m_can->isDeviceOpen()) {
    QString hw, fw, dr, lib, serial, typeStr;
    int canNum = 0;
    m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &canNum, &serial, &typeStr);
    if (typeStr.isEmpty()) typeStr = m_deviceTypeCombo->currentText().trimmed();
    m_detectBannerLabel->setText(QString("🟢 <b>设备已在线连接</b>：【%1 (设备%2)】 硬件: %3 · 固件: %4 · 驱动: %5 · 序列号: %6 · 运行路数: %7 路")
        .arg(typeStr).arg(m_can->deviceIndex()).arg(hw).arg(fw).arg(dr).arg(serial).arg(canNum));
    m_detectBannerLabel->setStyleSheet("color: #10B981; font-size: 12px; padding: 6px 10px; border: 1px solid #10B981; border-radius: 4px; background: rgba(16, 185, 129, 0.08);");
    return;
  }

  if (hasHw) {
    m_detectBannerLabel->setText(QString("🟢 <b>已检测到接入的 CAN 硬件卡</b>：【%1】（%2），已为您自动定位选中该型号，点击<b>[打开设备]</b>即可快速连接！")
        .arg(firstHw.modelName).arg(firstHw.detail));
    m_detectBannerLabel->setStyleSheet("color: #10B981; font-size: 12px; padding: 6px 10px; border: 1px solid #10B981; border-radius: 4px; background: rgba(16, 185, 129, 0.08);");

    if (autoSelectFirstHardware) {
      for (int i = 0; i < m_deviceTypeCombo->count(); ++i) {
        if (m_deviceTypeCombo->itemData(i).toUInt() == firstHw.deviceType) {
          m_deviceTypeCombo->setCurrentIndex(i);
          break;
        }
      }
      m_deviceIndexSpin->setValue(firstHw.deviceIndex);
    }
  } else {
    m_detectBannerLabel->setText(QString("💡 <b>未检测到接入的物理 CAN 硬件</b>，已为您就绪【🌟 纯软件仿真模式】，免硬件全流程收发与测试！"));
    m_detectBannerLabel->setStyleSheet("color: #F59E0B; font-size: 12px; padding: 6px 10px; border: 1px solid #F59E0B; border-radius: 4px; background: rgba(245, 158, 11, 0.08);");
  }
}

void CanDeviceDialog::refreshDeviceTree() {
  m_deviceTree->clear();
  m_startButtons.clear();
  m_stopButtons.clear();
  m_filterButtons.clear();
  m_deviceStartButton = nullptr;
  m_deviceStopButton = nullptr;
  m_deviceCloseButton = nullptr;
  m_deviceInfoButton = nullptr;

  const bool devOpen = m_can->isDeviceOpen();
  m_openButton->setText(devOpen ? "关闭设备" : "打开设备");
  m_openButton->setEnabled(true);
  m_deviceTypeCombo->setEnabled(!devOpen);
  m_deviceIndexSpin->setEnabled(!devOpen);

  // 获取已检测到的硬件列表
  QList<CanDetectedDevice> detected = m_can->detectConnectedDevices();
  QSet<quint32> detectedHwTypes;
  for (const auto &d : detected) {
    if (d.isHardware) {
      detectedHwTypes.insert(d.deviceType);
    }
  }

  // 始终显示完整型号目录，设备未连接时也能检索和选择型号。
  QMap<QString, QTreeWidgetItem *> vendorItems;
  int visibleDevices = 0;
  for (const auto &dev : kDeviceDefinitions) {
    const QString name = dev.name;
    const quint32 type = dev.type;
    const QString vendor = dev.vendor;
    if (!deviceMatchesFilter(vendor, name)) continue;
    QTreeWidgetItem *vendorItem = vendorItems.value(vendor, nullptr);
    if (!vendorItem) {
      vendorItem = new QTreeWidgetItem(m_deviceTree);
      vendorItem->setText(0, vendor);
      vendorItem->setText(1, "");
      vendorItem->setFont(0, QFont("Microsoft YaHei", 10, QFont::Bold));
      vendorItems.insert(vendor, vendorItem);
    }
    QTreeWidgetItem *item = new QTreeWidgetItem(vendorItem);
    item->setText(0, name);
    item->setData(0, Qt::UserRole, type);

    if (detectedHwTypes.contains(type)) {
      item->setText(1, "✔ [已检测到接入]");
      item->setForeground(1, QBrush(QColor("#10B981")));
    } else if (type == CAN_DEV_SIMULATION) {
      item->setText(1, "就绪可用");
      item->setForeground(1, QBrush(QColor("#00E5FF")));
    } else {
      item->setText(1, "未连接");
      item->setForeground(1, QBrush(QColor("#8a8f98")));
    }
    ++visibleDevices;
  }

  if (!devOpen) {
    updateSummary(kDeviceDefinitions.count(), visibleDevices, 0);
    m_deviceTree->expandAll();
    return;
  }

  // 1. 获取设备名称与通道数量（从实际硬件读取；VCI 设备查询失败时使用设备类型回退值）
  QString hw, fw, dr, lib, serial, typeStr;
  int canNum = 0;
  const bool infoOk = m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &canNum, &serial, &typeStr);
  if (canNum <= 0) {
    canNum = 2;
  }
  if (typeStr.isEmpty()) {
    typeStr = m_deviceTypeCombo->currentText().trimmed();
  }
  Q_UNUSED(infoOk);

  // 2. 将当前连接设备挂入对应厂商分组
  const quint32 currentType = m_can->deviceType();
  const QString currentName = m_deviceTypeCombo->currentText().trimmed();
  const QString currentVendor = vendorForDevice(currentName, currentType);
  QTreeWidgetItem *vendorItem = vendorItems.value(currentVendor, nullptr);
  if (!vendorItem) {
    vendorItem = new QTreeWidgetItem(m_deviceTree);
    vendorItem->setText(0, currentVendor);
    vendorItems.insert(currentVendor, vendorItem);
  }
  QTreeWidgetItem *devItem = nullptr;
  for (int i = 0; i < vendorItem->childCount(); ++i) {
    if (vendorItem->child(i)->data(0, Qt::UserRole).toUInt() == currentType) {
      devItem = vendorItem->child(i);
      break;
    }
  }
  if (!devItem) {
    devItem = new QTreeWidgetItem(vendorItem);
    devItem->setData(0, Qt::UserRole, currentType);
  }

  // 【核心修复图1】：清空 column 1 文本，将状态移至 column 0，防止文字从停止按钮底部/间隙穿透！
  devItem->setText(0, QString("%1  设备%2  [在线 · %3 路通道]").arg(typeStr).arg(m_can->deviceIndex()).arg(canNum));
  devItem->setIcon(0, drawRedArrowIcon());
  devItem->setText(1, QString()); // 确保 column 1 无文本，完全由 devBtnWidget 承载
  devItem->setForeground(0, QBrush(QColor("#10B981")));
  devItem->setToolTip(0, QString("硬件型号: %1\n硬件版本: %2\n固件版本: %3\n驱动版本: %4\n动态库: %5\n序列号: %6\n通道数量: %7 路")
                             .arg(typeStr).arg(hw).arg(fw).arg(dr).arg(lib).arg(serial).arg(canNum));

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

  // 展示已读取的设备版本信息子节点
  QTreeWidgetItem *infoNode = new QTreeWidgetItem(devItem);
  infoNode->setText(0, QString("📋 详细信息: 固件 %1 · 驱动 %2 · 序列号 %3").arg(fw).arg(dr).arg(serial));
  infoNode->setText(1, QString("硬件: %1 · 动态库: %2").arg(hw).arg(lib));
  infoNode->setForeground(0, QBrush(QColor("#38BDF8")));
  infoNode->setForeground(1, QBrush(QColor("#94A9C9")));

  // 3. 创建通道子节点
  for (int i = 0; i < canNum; ++i) {
    QTreeWidgetItem *chanItem = new QTreeWidgetItem(devItem);
    chanItem->setText(0, QString("通道%1").arg(i));
    chanItem->setIcon(0, drawGreyDotIcon());
    chanItem->setText(1, QString()); // 确保 column 1 无文字

    QWidget *chanBtnWidget = new QWidget();
    QHBoxLayout *chanBtnLayout = new QHBoxLayout(chanBtnWidget);
    chanBtnLayout->setContentsMargins(0, 0, 0, 0);
    chanBtnLayout->setSpacing(6);

    QPushButton *btnStart = new QPushButton("启动");
    QPushButton *btnStop = new QPushButton("停止");
    QPushButton *btnFilter = new QPushButton("滤波");

    const bool isRunning = m_can->isChannelRunning(i);
    setActionButtonStyle(btnStart, !isRunning);
    setActionButtonStyle(btnStop, isRunning);
    setActionButtonStyle(btnFilter, isRunning);

    chanBtnLayout->addWidget(btnStart);
    chanBtnLayout->addWidget(btnStop);
    chanBtnLayout->addWidget(btnFilter);
    chanBtnLayout->addStretch();
    m_deviceTree->setItemWidget(chanItem, 1, chanBtnWidget);

    m_startButtons[i] = btnStart;
    m_stopButtons[i] = btnStop;
    m_filterButtons[i] = btnFilter;

    connect(btnStart, &QPushButton::clicked, this, [this, i]() { onStartChannelClicked(i); });
    connect(btnStop, &QPushButton::clicked, this, [this, i]() { onStopChannelClicked(i); });
    connect(btnFilter, &QPushButton::clicked, this, [this, i]() { onFilterSettingsClicked(i); });
  }

  m_deviceTree->expandItem(vendorItem);
  m_deviceTree->expandItem(devItem);
  int runningChannels = 0;
  for (int i = 0; i < canNum; ++i) {
    if (m_can->isChannelRunning(i)) ++runningChannels;
  }
  updateSummary(kDeviceDefinitions.count(), visibleDevices, runningChannels);
  updateButtonStates();
}

void CanDeviceDialog::onOpenDeviceClicked() {
  if (m_can->isDeviceOpen()) {
    onCloseDeviceClicked();
    return;
  }
  const quint32 type = static_cast<quint32>(m_deviceTypeCombo->currentData().toUInt());
  const int index = m_deviceIndexSpin->value();

  if (m_can->openDevice(type, index)) {
    // 立即读取设备信息并更新提示与树
    QString hw, fw, dr, lib, serial, typeStr;
    int canNum = 0;
    m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &canNum, &serial, &typeStr);
    refreshDeviceTree();
    detectAndPromptDevices(false);
  } else {
    QString errDetail = m_can->lastError();
    if (errDetail.isEmpty()) {
      errDetail = QStringLiteral("打开设备失败，请检查驱动与设备连接。");
    }
    QMessageBox::critical(this, QStringLiteral("打开设备失败"), errDetail);
  }
}

void CanDeviceDialog::onCloseDeviceClicked() {
  m_can->closeDevice();
  refreshDeviceTree();
  detectAndPromptDevices(false);
}

void CanDeviceDialog::onStartChannelClicked(int channel) {
  const quint32 devType = static_cast<quint32>(m_deviceTypeCombo->currentData().toUInt());
  StartChannelDialog dlg(CanInterface::isDeviceFdCapable(devType), channel, this);
  QString currentQss = !m_currentStyle.isEmpty() ? m_currentStyle : getParentStyleSheet(this);
  if (!currentQss.isEmpty()) {
    dlg.setStyleSheet(currentQss);
  }
  if (dlg.exec() == QDialog::Accepted) {
    CanChannelConfig cfg = dlg.getConfig();
    QList<CanFilterRule> rules = dlg.getFilterRules();
    m_can->setChannelFilters(channel, rules);
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

void CanDeviceDialog::onFilterSettingsClicked(int channel) {
  QList<CanFilterRule> rules = m_can->channelFilters(channel);
  FilterSettingsDialog dlg(&rules, channel, this);
  QString currentQss = !m_currentStyle.isEmpty() ? m_currentStyle : getParentStyleSheet(this);
  if (!currentQss.isEmpty()) {
    dlg.setStyleSheet(currentQss);
  }
  if (dlg.exec() == QDialog::Accepted) {
    m_can->setChannelFilters(channel, rules);
    CanChannelConfig cfg = m_can->channelConfig(channel);
    cfg.enableFilter = true;
    m_can->setChannelConfig(channel, cfg);
  }
}

void CanDeviceDialog::onStartAllChannels() {
  const quint32 devType = static_cast<quint32>(m_deviceTypeCombo->currentData().toUInt());
  StartChannelDialog dlg(CanInterface::isDeviceFdCapable(devType), 0, this);
  QString currentQss = !m_currentStyle.isEmpty() ? m_currentStyle : getParentStyleSheet(this);
  if (!currentQss.isEmpty()) {
    dlg.setStyleSheet(currentQss);
  }
  if (dlg.exec() == QDialog::Accepted) {
    CanChannelConfig cfg = dlg.getConfig();
    QList<CanFilterRule> rules = dlg.getFilterRules();
    int count = m_startButtons.size();
    for (int i = 0; i < count; ++i) {
      m_can->setChannelFilters(i, rules);
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
    if (m_filterButtons.contains(ch)) {
      setActionButtonStyle(m_filterButtons[ch], running);
    }
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
    typeStr = m_deviceTypeCombo->currentText().trimmed();
    hw = QStringLiteral("—");
    fw = QStringLiteral("—");
    dr = QStringLiteral("—");
    lib = QStringLiteral("—");
    serial = QStringLiteral("—");
  }

  QDialog infoDlg(this);
  infoDlg.setWindowTitle("设备信息");
  infoDlg.setMinimumSize(380, 290);
  if (!m_currentStyle.isEmpty()) {
    infoDlg.setStyleSheet(m_currentStyle);
  } else {
    QString qss = getParentStyleSheet(this);
    if (!qss.isEmpty()) infoDlg.setStyleSheet(qss);
  }

  QVBoxLayout *mainLayout = new QVBoxLayout(&infoDlg);
  mainLayout->setContentsMargins(20, 20, 20, 15);
  mainLayout->setSpacing(12);

  QHBoxLayout *topLayout = new QHBoxLayout();
  topLayout->setSpacing(15);

  QLabel *iconLabel = new QLabel(&infoDlg);
  QIcon infoIcon = infoDlg.style()->standardIcon(QStyle::SP_MessageBoxInformation);
  iconLabel->setPixmap(infoIcon.pixmap(42, 42));
  iconLabel->setAlignment(Qt::AlignTop);
  topLayout->addWidget(iconLabel);

  QVBoxLayout *textLayout = new QVBoxLayout();
  textLayout->setSpacing(8);

  auto addLine = [&](const QString &label, const QString &val) {
    QLabel *lbl = new QLabel(QString("<span style='color: palette(mid);'>%1: </span><b>%2</b>").arg(label).arg(val), &infoDlg);
    lbl->setStyleSheet("font-size: 13px;");
    textLayout->addWidget(lbl);
  };

  addLine("硬件型号", typeStr);
  addLine("硬件版本", hw);
  addLine("固件版本", fw);
  addLine("驱动版本", dr);
  addLine("动态库版本", lib);
  addLine("CAN 通道数", QString("%1 路").arg(canNum));
  addLine("产品序列号", serial);

  topLayout->addLayout(textLayout);
  topLayout->addStretch();
  mainLayout->addLayout(topLayout);

  mainLayout->addSpacing(10);

  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();
  QPushButton *btnOk = new QPushButton("确定", &infoDlg);
  btnOk->setStyleSheet("QPushButton { padding: 6px 30px; font-weight: bold; font-size: 12px; }");
  connect(btnOk, &QPushButton::clicked, &infoDlg, &QDialog::accept);
  btnLayout->addWidget(btnOk);
  mainLayout->addLayout(btnLayout);

  infoDlg.exec();
}

void CanDeviceDialog::onCloudDeviceClicked() {
  QMessageBox::information(this, "云设备", "云设备连接中... (当前云设备服务不可用)");
}

void CanDeviceDialog::onDetectDevicesClicked() {
  detectAndPromptDevices(true);
  populateDeviceTypeCombo(m_vendorFilterCombo->currentIndex());
  refreshDeviceTree();
}

QString CanDeviceDialog::vendorForDevice(const QString &deviceName, quint32 deviceType) const {
  Q_UNUSED(deviceType);
  if (deviceName.contains("创芯")) return "创芯科技";
  if (deviceName.contains("USBCAN") || deviceName.contains("PCIE") || deviceName.contains("PCI-")) return "周立功 ZLG";
  if (deviceName.contains("PEAK") || deviceName.contains("Vector") || deviceName.contains("PassThru") ||
      deviceName.contains("SerialBus") || deviceName.contains("SYS TEC") || deviceName.contains("TinyCAN") ||
      deviceName.contains("SocketCAN") || deviceName.contains("Qt ")) return "Qt SerialBus";
  if (deviceName.contains("同星")) return "同星智能";
  if (deviceName.contains("仿真") || deviceName.contains("虚拟")) return "软件仿真";
  return "其他厂商";
}

bool CanDeviceDialog::deviceMatchesFilter(const QString &vendor, const QString &deviceName) const {
  const int vendorIndex = m_vendorFilterCombo ? m_vendorFilterCombo->currentIndex() : 0;
  if (vendorIndex > 0 && vendor != m_vendorFilterCombo->itemText(vendorIndex)) return false;
  const QString query = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
  return query.isEmpty() || deviceName.contains(query, Qt::CaseInsensitive) || vendor.contains(query, Qt::CaseInsensitive);
}

void CanDeviceDialog::updateSummary(int totalDevices, int visibleDevices, int runningChannels) {
  if (!m_summaryLabel) return;
  const QString state = m_can->isDeviceOpen() ? "设备在线" : "未连接设备";
  m_summaryLabel->setText(QString("%1  |  厂商目录 %2 项，当前筛选 %3 项，运行通道 %4").arg(state).arg(totalDevices).arg(visibleDevices).arg(runningChannels));
}

void CanDeviceDialog::onRefreshClicked() {
  detectAndPromptDevices(false);
  populateDeviceTypeCombo(m_vendorFilterCombo->currentIndex());
  refreshDeviceTree();
}

void CanDeviceDialog::onExpandAllClicked() {
  m_deviceTree->expandAll();
}

void CanDeviceDialog::onCollapseAllClicked() {
  m_deviceTree->collapseAll();
}

void CanDeviceDialog::onCopyConfigClicked() {
  QString config = QString("设备型号: %1\n设备索引: %2\n设备状态: %3\n").arg(m_deviceTypeCombo->currentText()).arg(m_can->deviceIndex()).arg(m_can->isDeviceOpen() ? "在线" : "未连接");
  if (m_can->isDeviceOpen()) {
    int channels = 0;
    QString hw, fw, dr, lib, serial, type;
    m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &channels, &serial, &type);
    config += QString("通道数量: %1\n序列号: %2\n硬件类型: %3\n").arg(channels).arg(serial).arg(type);
  }
  QApplication::clipboard()->setText(config);
  m_summaryLabel->setText("设备配置已复制到剪贴板");
}

void CanDeviceDialog::onTreeDeviceActivated(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  if (!item || !item->parent() || item->parent()->parent()) return;
  const quint32 type = item->data(0, Qt::UserRole).toUInt();
  for (int i = 0; i < m_deviceTypeCombo->count(); ++i) {
    if (m_deviceTypeCombo->itemData(i).toUInt() == type) {
      m_deviceTypeCombo->setCurrentIndex(i);
      break;
    }
  }
  if (!m_can->isDeviceOpen()) onOpenDeviceClicked();
}

void CanDeviceDialog::onSearchTextChanged(const QString &text) {
  Q_UNUSED(text);
  refreshDeviceTree();
}

void CanDeviceDialog::onVendorFilterChanged(int index) {
  populateDeviceTypeCombo(index);
  refreshDeviceTree();
}

void CanDeviceDialog::applyThemeStyle(const QString &qss) {
  m_currentStyle = qss;
  setStyleSheet(qss);
}

