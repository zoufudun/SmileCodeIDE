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
#include <QTableWidget>
#include <QHeaderView>
#include <QRegExpValidator>

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
    m_modeCombo->addItems({"标准帧明确ID", "扩展帧明确ID", "标准帧段ID", "扩展帧段ID"});
    ctrlLayout->addWidget(m_modeCombo);

    ctrlLayout->addWidget(new QLabel("起始ID: 0x"));
    m_startIdEdit = new QLineEdit("0");
    m_startIdEdit->setMaxLength(8);
    m_startIdEdit->setValidator(new QRegExpValidator(QRegExp("[0-9a-fA-F]{1,8}"), this));
    m_startIdEdit->setFixedWidth(70);
    ctrlLayout->addWidget(m_startIdEdit);

    ctrlLayout->addWidget(new QLabel("结束ID: 0x"));
    m_endIdEdit = new QLineEdit("0");
    m_endIdEdit->setMaxLength(8);
    m_endIdEdit->setValidator(new QRegExpValidator(QRegExp("[0-9a-fA-F]{1,8}"), this));
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
    setMinimumSize(420, 500);
    setStyleSheet("");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->setSpacing(10);

    // 波特率计算器按钮
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addStretch();
    QPushButton *btnCalc = new QPushButton("波特率计算器");
    btnCalc->setStyleSheet("QPushButton { padding: 3px 10px; font-size: 11px; }");
    topLayout->addWidget(btnCalc);
    mainLayout->addLayout(topLayout);

    // 滚动区域放置配置项
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border: 1px solid palette(mid); background: transparent; }");

    QWidget *scrollWidget = new QWidget();
    scrollWidget->setStyleSheet("QWidget { background: transparent; }");
    QFormLayout *form = new QFormLayout(scrollWidget);
    form->setSpacing(10);
    form->setContentsMargins(15, 15, 15, 15);

    // 协议选择：非 FD 设备仅可选 CAN
    m_protocolCombo = new QComboBox();
    if (m_fdCapable) {
      m_protocolCombo->addItems({"CAN FD", "CAN"});
    } else {
      m_protocolCombo->addItem("CAN");
      m_protocolCombo->setEnabled(false);
    }

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

    // Form rows setup (will inherit the theme's inputs stylesheet)
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

    // 非 FD 设备：隐藏不支持的配置行
    if (!m_fdCapable) {
      hideFormRow(form, m_standardCombo);
      hideFormRow(form, m_accelCombo);
      hideFormRow(form, m_dbitBaudCombo);
      hideFormRow(form, m_terminalResCombo);
      hideFormRow(form, m_reportBusCombo);
      hideFormRow(form, m_busPeriodEdit);
      hideFormRow(form, m_retryCombo);
    }

    scroll->setWidget(scrollWidget);
    mainLayout->addWidget(scroll);

    // 协议切换联动显示（仅 FD 设备有意义）
    if (m_fdCapable) {
      auto onProtocolChanged = [this]() {
        bool isFD = m_protocolCombo->currentText() == "CAN FD";
        m_standardCombo->setEnabled(isFD);
        m_accelCombo->setEnabled(isFD);
        m_dbitBaudCombo->setEnabled(isFD);
      };
      connect(m_protocolCombo, &QComboBox::currentTextChanged, this, onProtocolChanged);
      onProtocolChanged();
    }

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
    cfg.isFd = m_fdCapable && m_protocolCombo->currentText() == "CAN FD";
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

  QList<CanFilterRule> getFilterRules() const { return m_tempRules; }

private:
  // 隐藏 QFormLayout 中的一行（label + widget）
  static void hideFormRow(QFormLayout *form, QWidget *field) {
    if (QLabel *label = qobject_cast<QLabel *>(form->labelForField(field))) {
      label->setVisible(false);
    }
    field->setVisible(false);
  }

  bool m_fdCapable;
  int m_channel;
  QList<CanFilterRule> m_tempRules;
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
  setStyleSheet("");

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
  m_deviceTypeCombo->addItem("USBCAN-4E-U", ZCAN_USBCAN_4E_U);
  m_deviceTypeCombo->setStyleSheet("QComboBox { min-width: 140px; }");
  topPanel->addWidget(m_deviceTypeCombo);

  topPanel->addWidget(new QLabel("索引"));
  m_deviceIndexSpin = new QSpinBox();
  m_deviceIndexSpin->setRange(0, 7);
  m_deviceIndexSpin->setStyleSheet("QSpinBox { min-width: 50px; }");
  topPanel->addWidget(m_deviceIndexSpin);

  topPanel->addSpacing(10);

  m_openButton = new QPushButton("打开设备");
  m_cloudButton = new QPushButton("云设备");
  m_closeButton = new QPushButton("关闭窗口");

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
      "QTreeWidget::item { height: 38px; }"
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
  m_filterButtons.clear();
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

  // 1. 获取设备名称与通道数量（从实际硬件读取；VCI 设备查询失败时使用设备类型回退值）
  QString hw, fw, dr, lib, serial, typeStr;
  int canNum = 0;
  const bool infoOk = m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &canNum, &serial, &typeStr);
  if (canNum <= 0) {
    // 极端兜底：确保至少显示设备节点
    canNum = 2;
  }
  if (typeStr.isEmpty()) {
    typeStr = m_deviceTypeCombo->currentText();
  }
  Q_UNUSED(infoOk);  // 返回值仅用于日志/诊断，通道数与名称已保证有效

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
  const quint32 devType = static_cast<quint32>(m_deviceTypeCombo->currentData().toUInt());
  StartChannelDialog dlg(CanInterface::isDeviceFdCapable(devType), channel, this);
  QString currentQss = getParentStyleSheet(this);
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
  QString currentQss = getParentStyleSheet(this);
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
  QString currentQss = getParentStyleSheet(this);
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
    // 硬件查询失败时，使用当前选中的设备类型作为兜底显示
    typeStr = m_deviceTypeCombo->currentText();
    hw = QStringLiteral("—");
    fw = QStringLiteral("—");
    dr = QStringLiteral("—");
    lib = QStringLiteral("—");
    serial = QStringLiteral("—");
    // canNum 保持默认值，由调用方结合实际设备判断
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

void CanDeviceDialog::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss);
}
