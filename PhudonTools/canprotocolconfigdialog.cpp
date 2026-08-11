#include "canprotocolconfigdialog.h"

#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

CanProtocolConfigDialog::CanProtocolConfigDialog(QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(QStringLiteral("CAN 协议设备映射配置"));
  setMinimumSize(850, 550);
  setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(15, 15, 15, 15);
  mainLayout->setSpacing(12);

  // 表格放在最上方
  m_table = new QTableWidget(0, 9, this);
  m_table->setHorizontalHeaderLabels(
      {QStringLiteral("设备ID"), QStringLiteral("标签"), QStringLiteral("类型"),
       QStringLiteral("CAN ID (hex)"), QStringLiteral("字节"), QStringLiteral("位"),
       QStringLiteral("默认值"), QStringLiteral("所属界面"), QStringLiteral("所属房间")});
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setAlternatingRowColors(true);
  m_table->verticalHeader()->setVisible(false);
  m_table->horizontalHeader()->setStretchLastSection(true);
  m_table->setColumnWidth(0, 50);
  m_table->setColumnWidth(1, 120);
  m_table->setColumnWidth(2, 100);
  m_table->setColumnWidth(3, 90);
  m_table->setColumnWidth(4, 50);
  m_table->setColumnWidth(5, 50);
  m_table->setColumnWidth(6, 90);
  m_table->setColumnWidth(7, 90);
  m_table->setColumnWidth(8, 100);

  mainLayout->addWidget(m_table, 1);

  // 输入编辑区域 (紧凑水平布局，放在下方)
  QWidget *editArea = new QWidget(this);
  QHBoxLayout *editLayout = new QHBoxLayout(editArea);
  editLayout->setContentsMargins(5, 0, 5, 0);
  editLayout->setSpacing(8);

  editLayout->addWidget(new QLabel(QStringLiteral("设备标签:"), this));
  m_labelEdit = new QLineEdit(this);
  m_labelEdit->setMaxLength(32);
  m_labelEdit->setPlaceholderText(QStringLiteral("上限32字，如: 餐厅烟感"));
  m_labelEdit->setMinimumWidth(100);
  editLayout->addWidget(m_labelEdit);

  editLayout->addWidget(new QLabel(QStringLiteral("类型:"), this));
  m_defaultTypeCombo = new QComboBox(this);
  m_defaultTypeCombo->addItem(QStringLiteral("烟温探测器"), QStringLiteral("detector"));
  m_defaultTypeCombo->addItem(QStringLiteral("分配阀(蝶阀)"), QStringLiteral("valve_distributor"));
  m_defaultTypeCombo->addItem(QStringLiteral("区域阀(闸阀)"), QStringLiteral("valve_zone"));
  m_defaultTypeCombo->addItem(QStringLiteral("总管隔离阀(截止阀)"), QStringLiteral("valve_main_isolation"));
  m_defaultTypeCombo->addItem(QStringLiteral("控制分配阀"), QStringLiteral("valve"));
  m_defaultTypeCombo->addItem(QStringLiteral("手动报警按钮"), QStringLiteral("manual_alarm"));
  m_defaultTypeCombo->addItem(QStringLiteral("1301气体钢瓶"), QStringLiteral("gas_cylinder"));
  m_defaultTypeCombo->addItem(QStringLiteral("水泵"), QStringLiteral("water_pump"));
  m_defaultTypeCombo->addItem(QStringLiteral("压力开关"), QStringLiteral("pressure_switch"));
  m_defaultTypeCombo->addItem(QStringLiteral("移动喷枪"), QStringLiteral("mobile_spray_gun"));
  editLayout->addWidget(m_defaultTypeCombo);

  editLayout->addWidget(new QLabel(QStringLiteral("CAN ID:"), this));
  m_defaultCanIdEdit = new QLineEdit(QStringLiteral("0x100"), this);
  m_defaultCanIdEdit->setFixedWidth(70);
  editLayout->addWidget(m_defaultCanIdEdit);

  editLayout->addWidget(new QLabel(QStringLiteral("字节(0-7):"), this));
  m_defaultByteSpin = new QSpinBox(this);
  m_defaultByteSpin->setRange(0, 7);
  m_defaultByteSpin->setValue(0);
  editLayout->addWidget(m_defaultByteSpin);

  editLayout->addWidget(new QLabel(QStringLiteral("位(0-7):"), this));
  m_defaultBitSpin = new QSpinBox(this);
  m_defaultBitSpin->setRange(0, 7);
  m_defaultBitSpin->setValue(0);
  editLayout->addWidget(m_defaultBitSpin);

  editLayout->addWidget(new QLabel(QStringLiteral("默认值:"), this));
  m_defaultValCombo = new QComboBox(this);
  m_defaultValCombo->addItem(QStringLiteral("0 (正常/关闭)"), 0);
  m_defaultValCombo->addItem(QStringLiteral("1 (报警/开启)"), 1);
  editLayout->addWidget(m_defaultValCombo);

  editLayout->addWidget(new QLabel(QStringLiteral("所属界面:"), this));
  m_targetViewCombo = new QComboBox(this);
  m_targetViewCombo->setEditable(true); // 允许下拉选择或手动输入
  m_targetViewCombo->addItem(QStringLiteral("界面1"));
  m_targetViewCombo->addItem(QStringLiteral("界面2"));
  m_targetViewCombo->addItem(QStringLiteral("界面3"));
  m_targetViewCombo->setFixedWidth(90);
  editLayout->addWidget(m_targetViewCombo);

  editLayout->addWidget(new QLabel(QStringLiteral("所属房间:"), this));
  m_targetRoomCombo = new QComboBox(this);
  m_targetRoomCombo->setEditable(true);
  m_targetRoomCombo->addItem(QStringLiteral("(未指定/未放置区)"), QString());
  m_targetRoomCombo->setFixedWidth(120);
  editLayout->addWidget(m_targetRoomCombo);

  mainLayout->addWidget(editArea);

  // 控制按钮区域
  auto *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(8);

  m_btnAdd = new QPushButton(QStringLiteral("＋ 添加设备"), this);
  m_btnAdd->setCursor(Qt::PointingHandCursor);
  m_btnModify = new QPushButton(QStringLiteral("✓ 修改选中"), this);
  m_btnModify->setCursor(Qt::PointingHandCursor);
  m_btnModify->setEnabled(false);

  m_btnDelete = new QPushButton(QStringLiteral("✕ 删除选中"), this);
  m_btnDelete->setObjectName("deleteButton");
  m_btnDelete->setCursor(Qt::PointingHandCursor);
  m_btnDelete->setEnabled(false);

  m_btnClearAll = new QPushButton(QStringLiteral("🗑 清空"), this);
  m_btnClearAll->setObjectName("clearAllButton");
  m_btnClearAll->setCursor(Qt::PointingHandCursor);
  m_btnClearAll->setStyleSheet(
      "QPushButton { color: #F87171; background: #271A1A; border: 1px solid #3E1E1E; "
      "padding: 4px 12px; border-radius: 3px; font-weight: bold; } "
      "QPushButton:hover { background: #3E1E1E; border-color: #EF4444; }");

  m_btnImport = new QPushButton(QStringLiteral("导入 JSON"), this);
  m_btnImport->setCursor(Qt::PointingHandCursor);
  m_btnExport = new QPushButton(QStringLiteral("导出 JSON"), this);
  m_btnExport->setCursor(Qt::PointingHandCursor);

  btnLayout->addWidget(m_btnAdd);
  btnLayout->addWidget(m_btnModify);
  btnLayout->addWidget(m_btnDelete);
  btnLayout->addWidget(m_btnClearAll);
  btnLayout->addStretch();
  btnLayout->addWidget(m_btnImport);
  btnLayout->addWidget(m_btnExport);
  mainLayout->addLayout(btnLayout);

  // 确定/取消
  auto *bottomLayout = new QHBoxLayout();
  bottomLayout->setSpacing(15);
  m_btnOk = new QPushButton(QStringLiteral("确定"), this);
  m_btnOk->setCursor(Qt::PointingHandCursor);
  m_btnCancel = new QPushButton(QStringLiteral("取消"), this);
  m_btnCancel->setCursor(Qt::PointingHandCursor);

  bottomLayout->addStretch();
  bottomLayout->addWidget(m_btnOk);
  bottomLayout->addWidget(m_btnCancel);
  bottomLayout->addStretch();
  mainLayout->addLayout(bottomLayout);

  // 信号绑定
  connect(m_btnAdd, &QPushButton::clicked, this, &CanProtocolConfigDialog::onAddRow);
  connect(m_btnModify, &QPushButton::clicked, this, &CanProtocolConfigDialog::onModifyRow);
  connect(m_btnDelete, &QPushButton::clicked, this, &CanProtocolConfigDialog::onDeleteRow);
  connect(m_btnClearAll, &QPushButton::clicked, this, [this]() {
    if (QMessageBox::question(this, QStringLiteral("清空全部"),
          QStringLiteral("确定要清空所有设备映射吗？此操作不可撤销。")) == QMessageBox::Yes) {
      m_table->setRowCount(0);
    }
  });
  connect(m_btnImport, &QPushButton::clicked, this, &CanProtocolConfigDialog::onImportJson);
  connect(m_btnExport, &QPushButton::clicked, this, &CanProtocolConfigDialog::onExportJson);
  connect(m_btnOk, &QPushButton::clicked, this, &QDialog::accept);
  connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
  connect(m_table, &QTableWidget::itemSelectionChanged, this, &CanProtocolConfigDialog::onSelectionChanged);
}

void CanProtocolConfigDialog::setAvailableViews(const QStringList &viewNames) {
  m_availableViews = viewNames;
  if (m_targetViewCombo) {
    m_targetViewCombo->clear();
    for (const QString &v : m_availableViews) {
      m_targetViewCombo->addItem(v);
    }
  }
}

void CanProtocolConfigDialog::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss + 
    "\nQPushButton#deleteButton { background-color: #ef4444; color: white; }"
    "\nQPushButton#deleteButton:hover { background-color: #dc2626; }"
    "\nQPushButton#deleteButtonRow { background-color: rgba(239, 68, 68, 0.15); color: #ef4444; border: 1px solid rgba(239, 68, 68, 0.3); padding: 2px 8px; border-radius: 4px; font-size: 11px; }"
    "\nQPushButton#deleteButtonRow:hover { background-color: #ef4444; color: white; }"
  );
}

void CanProtocolConfigDialog::addTableRow(int deviceId, const QString &label,
                                          const QString &type, quint32 canId,
                                          int byteIdx, int bitIdx, int defaultVal,
                                          const QString &targetView,
                                          const QString &targetRoom) {
  int row = m_table->rowCount();
  m_table->insertRow(row);

  // 设备ID (只读)
  auto *idItem = new QTableWidgetItem(QString::number(deviceId));
  idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
  idItem->setTextAlignment(Qt::AlignCenter);
  m_table->setItem(row, 0, idItem);

  // 标签 (可编辑)
  auto *labelItem = new QTableWidgetItem(label);
  labelItem->setTextAlignment(Qt::AlignCenter);
  m_table->setItem(row, 1, labelItem);

  // 类型 (下拉)
  auto *typeCombo = new QComboBox(m_table);
  typeCombo->addItem(QStringLiteral("烟温探测器"), QStringLiteral("detector"));
  typeCombo->addItem(QStringLiteral("分配阀(蝶阀)"), QStringLiteral("valve_distributor"));
  typeCombo->addItem(QStringLiteral("区域阀(闸阀)"), QStringLiteral("valve_zone"));
  typeCombo->addItem(QStringLiteral("总管隔离阀(截止阀)"), QStringLiteral("valve_main_isolation"));
  typeCombo->addItem(QStringLiteral("控制分配阀"), QStringLiteral("valve"));
  typeCombo->addItem(QStringLiteral("手动报警按钮"), QStringLiteral("manual_alarm"));
  typeCombo->addItem(QStringLiteral("1301气体钢瓶"), QStringLiteral("gas_cylinder"));
  typeCombo->addItem(QStringLiteral("水泵"), QStringLiteral("water_pump"));
  typeCombo->addItem(QStringLiteral("压力开关"), QStringLiteral("pressure_switch"));
  typeCombo->addItem(QStringLiteral("移动喷枪"), QStringLiteral("mobile_spray_gun"));

  int index = typeCombo->findData(type);
  if (index >= 0) {
    typeCombo->setCurrentIndex(index);
  }
  m_table->setCellWidget(row, 2, typeCombo);

  // CAN ID (hex)
  auto *idHexItem = new QTableWidgetItem(QString("0x%1").arg(canId, 0, 16).toUpper());
  idHexItem->setTextAlignment(Qt::AlignCenter);
  m_table->setItem(row, 3, idHexItem);

  // 字节索引
  auto *byteSpin = new QSpinBox(m_table);
  byteSpin->setRange(0, 7);
  byteSpin->setValue(byteIdx);
  byteSpin->setAlignment(Qt::AlignCenter);
  m_table->setCellWidget(row, 4, byteSpin);

  // 位索引
  auto *bitSpin = new QSpinBox(m_table);
  bitSpin->setRange(0, 7);
  bitSpin->setValue(bitIdx);
  bitSpin->setAlignment(Qt::AlignCenter);
  m_table->setCellWidget(row, 5, bitSpin);

  // 默认值
  auto *defaultCombo = new QComboBox(m_table);
  defaultCombo->addItem(QStringLiteral("0 (正常/关闭)"), 0);
  defaultCombo->addItem(QStringLiteral("1 (报警/开启)"), 1);
  defaultCombo->setCurrentIndex(defaultVal == 1 ? 1 : 0);
  m_table->setCellWidget(row, 6, defaultCombo);

  // 所属界面 (下拉框选择)
  auto *targetViewCombo = new QComboBox(m_table);
  targetViewCombo->setEditable(true);
  QStringList views = m_availableViews.isEmpty() ? QStringList{QStringLiteral("界面1"), QStringLiteral("界面2"), QStringLiteral("界面3")} : m_availableViews;
  for (const QString &v : views) {
    targetViewCombo->addItem(v);
  }
  QString tView = targetView.isEmpty() ? QStringLiteral("界面1") : targetView;
  int vIdx = targetViewCombo->findText(tView);
  if (vIdx >= 0) {
    targetViewCombo->setCurrentIndex(vIdx);
  } else {
    targetViewCombo->setCurrentText(tView);
  }
  m_table->setCellWidget(row, 7, targetViewCombo);

  // 所属房间 (可编辑下拉单元格)
  auto *targetRoomComboCell = new QComboBox(m_table);
  targetRoomComboCell->setEditable(true);
  targetRoomComboCell->addItem(QStringLiteral("(未指定/未放置区)"), QString());
  for (const QString &rName : m_availableRooms) {
    if (!rName.trimmed().isEmpty()) {
      targetRoomComboCell->addItem(rName.trimmed(), rName.trimmed());
    }
  }
  QString tRoom = targetRoom.trimmed();
  int rIdx = targetRoomComboCell->findText(tRoom);
  if (rIdx >= 0) {
    targetRoomComboCell->setCurrentIndex(rIdx);
  } else if (!tRoom.isEmpty()) {
    targetRoomComboCell->setCurrentText(tRoom);
  }
  m_table->setCellWidget(row, 8, targetRoomComboCell);
}

// ---- 公共接口 ----

void CanProtocolConfigDialog::setAvailableRooms(const QStringList &roomNames) {
  m_availableRooms = roomNames;
  if (m_targetRoomCombo) {
    QString curText = m_targetRoomCombo->currentText();
    m_targetRoomCombo->clear();
    m_targetRoomCombo->addItem(QStringLiteral("(未指定/未放置区)"), QString());
    for (const QString &r : m_availableRooms) {
      if (!r.trimmed().isEmpty()) {
        m_targetRoomCombo->addItem(r.trimmed(), r.trimmed());
      }
    }
    if (!curText.isEmpty()) {
      int rIdx = m_targetRoomCombo->findText(curText);
      if (rIdx >= 0) m_targetRoomCombo->setCurrentIndex(rIdx);
      else m_targetRoomCombo->setCurrentText(curText);
    }
  }
}

void CanProtocolConfigDialog::setMappings(
    const QList<DeviceBitMapping> &mappings) {
  m_table->setRowCount(0);
  int maxId = 0;
  for (const auto &m : mappings) {
    addTableRow(m.deviceId, m.label, m.deviceType, m.canId, m.byteIndex,
                m.bitIndex, m.defaultVal, m.targetView, m.targetRoom);
    if (m.deviceId > maxId) maxId = m.deviceId;
  }
  m_nextDeviceId = maxId + 1;
}

QList<DeviceBitMapping> CanProtocolConfigDialog::mappings() const {
  QList<DeviceBitMapping> result;
  for (int r = 0; r < m_table->rowCount(); ++r) {
    DeviceBitMapping m;
    m.deviceId = m_table->item(r, 0)->text().toInt();
    m.label = m_table->item(r, 1)->text().trimmed();

    auto *typeCombo = qobject_cast<QComboBox *>(m_table->cellWidget(r, 2));
    m.deviceType =
        typeCombo ? typeCombo->currentData().toString() : QStringLiteral("detector");

    QString canIdText = m_table->item(r, 3)->text().trimmed();
    bool ok = false;
    m.canId = canIdText.toUInt(&ok, 0);
    if (!ok) m.canId = 0;

    auto *byteSpin = qobject_cast<QSpinBox *>(m_table->cellWidget(r, 4));
    m.byteIndex = byteSpin ? byteSpin->value() : 0;

    auto *bitSpin = qobject_cast<QSpinBox *>(m_table->cellWidget(r, 5));
    m.bitIndex = bitSpin ? bitSpin->value() : 0;

    auto *defaultCombo = qobject_cast<QComboBox *>(m_table->cellWidget(r, 6));
    m.defaultVal = defaultCombo ? defaultCombo->currentData().toInt() : 0;

    auto *targetViewCombo = qobject_cast<QComboBox *>(m_table->cellWidget(r, 7));
    if (targetViewCombo) {
      m.targetView = targetViewCombo->currentText().trimmed();
    } else if (m_table->item(r, 7)) {
      m.targetView = m_table->item(r, 7)->text().trimmed();
    }
    if (m.targetView.isEmpty()) {
      m.targetView = QStringLiteral("界面1");
    }

    auto *targetRoomCombo = qobject_cast<QComboBox *>(m_table->cellWidget(r, 8));
    if (targetRoomCombo) {
      QString rText = targetRoomCombo->currentText().trimmed();
      if (rText == QStringLiteral("(未指定/未放置区)")) rText.clear();
      m.targetRoom = rText;
    } else if (m_table->item(r, 8)) {
      m.targetRoom = m_table->item(r, 8)->text().trimmed();
    }

    result.append(m);
  }
  return result;
}

// ---- 添加/删除 ----

void CanProtocolConfigDialog::onAddRow() {
  QString type = m_defaultTypeCombo->currentData().toString();
  
  bool ok = false;
  QString canIdText = m_defaultCanIdEdit->text().trimmed();
  quint32 canId = canIdText.toUInt(&ok, 0);
  if (!ok) {
    canId = 0x100;
  }

  int byteIdx = m_defaultByteSpin->value();
  int bitIdx = m_defaultBitSpin->value();
  int defaultVal = m_defaultValCombo->currentData().toInt();
  QString targetView = m_targetViewCombo ? m_targetViewCombo->currentText().trimmed() : QStringLiteral("界面1");
  if (targetView.isEmpty()) targetView = QStringLiteral("界面1");
  QString targetRoom = m_targetRoomCombo ? m_targetRoomCombo->currentText().trimmed() : QString();
  if (targetRoom == QStringLiteral("(未指定/未放置区)")) targetRoom.clear();

  int deviceId = m_nextDeviceId++;
  QString label = m_labelEdit->text().trimmed();
  if (label.isEmpty()) {
    label = QStringLiteral("设备%1").arg(deviceId);
  }

  addTableRow(deviceId, label, type, canId, byteIdx, bitIdx, defaultVal, targetView, targetRoom);
}

void CanProtocolConfigDialog::onDeleteRow() {
  int row = m_table->currentRow();
  if (row >= 0) {
    m_table->removeRow(row);
  }
}

void CanProtocolConfigDialog::onModifyRow() {
  int row = m_table->currentRow();
  if (row < 0) return;

  // 标签
  if (m_table->item(row, 1)) {
    m_table->item(row, 1)->setText(m_labelEdit->text().trimmed());
  }

  // 类型
  auto *typeCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 2));
  if (typeCombo) {
    typeCombo->setCurrentIndex(m_defaultTypeCombo->currentIndex());
  }

  // CAN ID (hex)
  if (m_table->item(row, 3)) {
    bool ok = false;
    QString canIdText = m_defaultCanIdEdit->text().trimmed();
    quint32 canId = canIdText.toUInt(&ok, 0);
    if (!ok) canId = 0x100;
    m_table->item(row, 3)->setText(QString("0x%1").arg(canId, 0, 16).toUpper());
  }

  // 字节
  auto *byteSpin = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 4));
  if (byteSpin) {
    byteSpin->setValue(m_defaultByteSpin->value());
  }

  // 位
  auto *bitSpin = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 5));
  if (bitSpin) {
    bitSpin->setValue(m_defaultBitSpin->value());
  }

  // 默认值
  auto *defaultCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 6));
  if (defaultCombo) {
    defaultCombo->setCurrentIndex(m_defaultValCombo->currentIndex());
  }

  // 所属界面
  auto *targetViewCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 7));
  if (targetViewCombo && m_targetViewCombo) {
    QString targetView = m_targetViewCombo->currentText().trimmed();
    if (targetView.isEmpty()) targetView = QStringLiteral("界面1");
    int vIdx = targetViewCombo->findText(targetView);
    if (vIdx >= 0) targetViewCombo->setCurrentIndex(vIdx);
    else targetViewCombo->setCurrentText(targetView);
  }

  // 所属房间
  auto *targetRoomComboCell = qobject_cast<QComboBox *>(m_table->cellWidget(row, 8));
  if (targetRoomComboCell && m_targetRoomCombo) {
    QString targetRoom = m_targetRoomCombo->currentText().trimmed();
    int rIdx = targetRoomComboCell->findText(targetRoom);
    if (rIdx >= 0) targetRoomComboCell->setCurrentIndex(rIdx);
    else targetRoomComboCell->setCurrentText(targetRoom);
  } else if (m_table->item(row, 8) && m_targetRoomCombo) {
    m_table->item(row, 8)->setText(m_targetRoomCombo->currentText().trimmed());
  }
}

void CanProtocolConfigDialog::onSelectionChanged() {
  int row = m_table->currentRow();
  if (row >= 0) {
    if (m_table->item(row, 1)) {
      m_labelEdit->setText(m_table->item(row, 1)->text());
    }
    auto *typeCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 2));
    if (typeCombo) {
      m_defaultTypeCombo->setCurrentIndex(typeCombo->currentIndex());
    }
    if (m_table->item(row, 3)) {
      m_defaultCanIdEdit->setText(m_table->item(row, 3)->text());
    }
    auto *byteSpin = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 4));
    if (byteSpin) {
      m_defaultByteSpin->setValue(byteSpin->value());
    }
    auto *bitSpin = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 5));
    if (bitSpin) {
      m_defaultBitSpin->setValue(bitSpin->value());
    }
    auto *defaultCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 6));
    if (defaultCombo) {
      m_defaultValCombo->setCurrentIndex(defaultCombo->currentIndex());
    }
    auto *targetViewCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 7));
    if (targetViewCombo && m_targetViewCombo) {
      int vIdx = m_targetViewCombo->findText(targetViewCombo->currentText());
      if (vIdx >= 0) m_targetViewCombo->setCurrentIndex(vIdx);
      else m_targetViewCombo->setCurrentText(targetViewCombo->currentText());
    }
    auto *targetRoomComboCell = qobject_cast<QComboBox *>(m_table->cellWidget(row, 8));
    if (targetRoomComboCell && m_targetRoomCombo) {
      int rIdx = m_targetRoomCombo->findText(targetRoomComboCell->currentText());
      if (rIdx >= 0) m_targetRoomCombo->setCurrentIndex(rIdx);
      else m_targetRoomCombo->setCurrentText(targetRoomComboCell->currentText());
    } else if (m_table->item(row, 8) && m_targetRoomCombo) {
      int rIdx = m_targetRoomCombo->findText(m_table->item(row, 8)->text());
      if (rIdx >= 0) m_targetRoomCombo->setCurrentIndex(rIdx);
      else m_targetRoomCombo->setCurrentText(m_table->item(row, 8)->text());
    }
    m_btnModify->setEnabled(true);
    m_btnDelete->setEnabled(true);
  } else {
    m_btnModify->setEnabled(false);
    m_btnDelete->setEnabled(false);
  }
}

// ---- JSON 导入/导出 ----

void CanProtocolConfigDialog::onExportJson() {
  QList<DeviceBitMapping> list = mappings();
  QJsonArray arr;
  for (const auto &m : list) {
    QJsonObject obj;
    obj["deviceId"] = m.deviceId;
    obj["label"] = m.label;
    obj["deviceType"] = m.deviceType;
    obj["canId"] = static_cast<int>(m.canId);
    obj["byteIndex"] = m.byteIndex;
    obj["bitIndex"] = m.bitIndex;
    obj["defaultVal"] = m.defaultVal;
    obj["targetView"] = m.targetView;
    arr.append(obj);
  }

  QString path = QFileDialog::getSaveFileName(
      this, QStringLiteral("导出协议配置"),
      QStringLiteral("can_protocol_config.json"),
      QStringLiteral("JSON 文件 (*.json)"));
  if (path.isEmpty()) return;

  QFile file(path);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    file.close();
    QMessageBox::information(this, QStringLiteral("导出成功"),
                             QStringLiteral("协议配置已导出到:\n%1").arg(path));
  } else {
    QMessageBox::critical(this, QStringLiteral("导出失败"),
                          QStringLiteral("无法写入文件:\n%1").arg(path));
  }
}

void CanProtocolConfigDialog::onImportJson() {
  QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("导入协议配置"), QString(),
      QStringLiteral("JSON 文件 (*.json)"));
  if (path.isEmpty()) return;

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, QStringLiteral("导入失败"),
                          QStringLiteral("无法读取文件:\n%1").arg(path));
    return;
  }

  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
  file.close();

  if (err.error != QJsonParseError::NoError || !doc.isArray()) {
    QMessageBox::critical(
        this, QStringLiteral("格式错误"),
        QStringLiteral("JSON 解析失败: %1").arg(err.errorString()));
    return;
  }

  m_table->setRowCount(0);
  int maxId = 0;
  for (const QJsonValue &val : doc.array()) {
    QJsonObject obj = val.toObject();
    int id = obj.value("deviceId").toInt(1);
    if (id > maxId) maxId = id;
    addTableRow(
        id, obj.value("label").toString(QStringLiteral("未知")),
        obj.value("deviceType").toString(QStringLiteral("detector")),
        static_cast<quint32>(obj.value("canId").toInt(0x100)),
        obj.value("byteIndex").toInt(0), obj.value("bitIndex").toInt(0),
        obj.value("defaultVal").toInt(0),
        obj.value("targetView").toString(QStringLiteral("界面1")));
  }
  m_nextDeviceId = maxId + 1;

  QMessageBox::information(this, QStringLiteral("导入成功"),
                           QStringLiteral("已导入 %1 条设备映射。")
                               .arg(doc.array().size()));
}
