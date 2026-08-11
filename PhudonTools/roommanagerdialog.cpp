#include "roommanagerdialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

RoomManagerDialog::RoomManagerDialog(const QList<RoomRegion> &rooms,
                                       const QStringList &viewNames,
                                       QWidget *parent)
    : QDialog(parent), m_rooms(rooms), m_viewNames(viewNames) {
  setWindowTitle(QStringLiteral("🏠 已建房间列表管理"));
  resize(750, 480);
  setStyleSheet(
      "QDialog { background-color: #0F172A; color: #E2E8F0; font-family: "
      "'Microsoft YaHei'; }"
      "QLabel { color: #94A3B8; font-size: 12px; }"
      "QTableWidget { background-color: #1E293B; color: #E2E8F0; border: 1px "
      "solid #334155; gridline-color: #334155; font-size: 12px; "
      "selection-background-color: #1E3A5F; }"
      "QHeaderView::section { background-color: #0F172A; color: #00D4FF; "
      "font-weight: bold; padding: 6px; border: 1px solid #334155; font-size: "
      "12px; }"
      "QLineEdit, QComboBox { background: #0F172A; color: #E2E8F0; border: 1px "
      "solid #475569; padding: 3px 6px; border-radius: 3px; font-size: 12px; }"
      "QLineEdit:focus, QComboBox:focus { border-color: #00D4FF; }"
      "QPushButton { background: #1E3A5F; color: #00D4FF; border: 1px solid "
      "#00D4FF; padding: 5px 12px; border-radius: 4px; font-weight: bold; "
      "font-size: 12px; }"
      "QPushButton:hover { background: #00D4FF; color: #0F172A; }"
      "QCheckBox { color: #E2E8F0; font-size: 12px; }");

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(12);

  // 标题说明
  auto *headerLabel = new QLabel(
      QStringLiteral("提示：在下方列表中可配置房间显隐、固定不动(防拖拽)、修改名称、形状及主题颜色。"),
      this);
  headerLabel->setStyleSheet("color: #94A3B8; font-size: 12px;");
  mainLayout->addWidget(headerLabel);

  // 表格控件
  m_table = new QTableWidget(this);
  m_table->setColumnCount(7);
  m_table->setHorizontalHeaderLabels({
      QStringLiteral("房间名称"),
      QStringLiteral("所属界面"),
      QStringLiteral("👁️ 显示"),
      QStringLiteral("🔒 固定"),
      QStringLiteral("形状"),
      QStringLiteral("主题颜色"),
      QStringLiteral("操作"),
  });

  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setAlternatingRowColors(true);
  mainLayout->addWidget(m_table);

  populateTable();

  // 底部工具条与按键
  auto *btmLayout = new QHBoxLayout();
  m_btnAdd = new QPushButton(QStringLiteral("➕ 新建房间"), this);
  connect(m_btnAdd, &QPushButton::clicked, this, &RoomManagerDialog::onAddRoom);

  m_btnSelectAll = new QPushButton(QStringLiteral("👁 全选显示"), this);
  connect(m_btnSelectAll, &QPushButton::clicked, this, [this]() { onToggleSelectAll(true); });

  m_btnUnselectAll = new QPushButton(QStringLiteral("🙈 全选隐藏"), this);
  connect(m_btnUnselectAll, &QPushButton::clicked, this, [this]() { onToggleSelectAll(false); });

  btmLayout->addWidget(m_btnAdd);
  btmLayout->addWidget(m_btnSelectAll);
  btmLayout->addWidget(m_btnUnselectAll);
  btmLayout->addStretch();

  auto *btnBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(btnBox, &QDialogButtonBox::accepted, this, [this]() {
    // 保存表格修改至 m_rooms
    for (int r = 0; r < m_table->rowCount(); ++r) {
      if (r >= m_rooms.size()) break;
      auto *nameEdit = qobject_cast<QLineEdit *>(m_table->cellWidget(r, 0));
      auto *viewCombo = qobject_cast<QComboBox *>(m_table->cellWidget(r, 1));
      
      auto *visWidget = m_table->cellWidget(r, 2);
      auto *visCheck = visWidget ? visWidget->findChild<QCheckBox *>() : nullptr;

      auto *lockWidget = m_table->cellWidget(r, 3);
      auto *lockCheck = lockWidget ? lockWidget->findChild<QCheckBox *>() : nullptr;

      auto *shapeCombo = qobject_cast<QComboBox *>(m_table->cellWidget(r, 4));
      auto *colorBtn = qobject_cast<QPushButton *>(m_table->cellWidget(r, 5));

      if (nameEdit && !nameEdit->text().trimmed().isEmpty()) {
        m_rooms[r].name = nameEdit->text().trimmed();
      }
      if (viewCombo) {
        m_rooms[r].targetView = viewCombo->currentText().trimmed();
      }
      if (visCheck) {
        m_rooms[r].visible = visCheck->isChecked();
      }
      if (lockCheck) {
        m_rooms[r].isLocked = lockCheck->isChecked();
      }
      if (shapeCombo) {
        m_rooms[r].shape = shapeCombo->currentData().toInt();
      }
      if (colorBtn) {
        QVariant colVar = colorBtn->property("roomColor");
        if (colVar.isValid() && colVar.canConvert<QColor>()) {
          m_rooms[r].color = colVar.value<QColor>();
        }
      }
    }
    accept();
  });
  connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  btmLayout->addWidget(btnBox);

  mainLayout->addLayout(btmLayout);
}

void RoomManagerDialog::populateTable() {
  m_table->setRowCount(0);
  for (int i = 0; i < m_rooms.size(); ++i) {
    const auto &r = m_rooms[i];
    int row = m_table->rowCount();
    m_table->insertRow(row);

    // 0. 房间名称
    auto *nameEdit = new QLineEdit(r.name, this);
    m_table->setCellWidget(row, 0, nameEdit);

    // 1. 所属界面
    auto *viewCombo = new QComboBox(this);
    for (const QString &vName : m_viewNames) {
      viewCombo->addItem(vName);
    }
    int vIdx = viewCombo->findText(r.targetView);
    if (vIdx >= 0) viewCombo->setCurrentIndex(vIdx);
    else if (!r.targetView.isEmpty()) viewCombo->addItem(r.targetView);
    m_table->setCellWidget(row, 1, viewCombo);

    // 2. 显示/隐藏
    auto *visCheck = new QCheckBox(this);
    visCheck->setChecked(r.visible);
    visCheck->setText(r.visible ? QStringLiteral("👁 显示") : QStringLiteral("🙈 隐藏"));
    connect(visCheck, &QCheckBox::toggled, visCheck, [visCheck](bool checked) {
      visCheck->setText(checked ? QStringLiteral("👁 显示") : QStringLiteral("🙈 隐藏"));
    });
    auto *visWidget = new QWidget(this);
    auto *visLayout = new QHBoxLayout(visWidget);
    visLayout->addWidget(visCheck);
    visLayout->setAlignment(Qt::AlignCenter);
    visLayout->setContentsMargins(0, 0, 0, 0);
    m_table->setCellWidget(row, 2, visWidget);

    // 3. 固定不动
    auto *lockCheck = new QCheckBox(this);
    lockCheck->setChecked(r.isLocked);
    lockCheck->setText(r.isLocked ? QStringLiteral("🔒 锁定") : QStringLiteral("🔓 自由"));
    connect(lockCheck, &QCheckBox::toggled, lockCheck, [lockCheck](bool checked) {
      lockCheck->setText(checked ? QStringLiteral("🔒 锁定") : QStringLiteral("🔓 自由"));
    });
    auto *lockWidget = new QWidget(this);
    auto *lockLayout = new QHBoxLayout(lockWidget);
    lockLayout->addWidget(lockCheck);
    lockLayout->setAlignment(Qt::AlignCenter);
    lockLayout->setContentsMargins(0, 0, 0, 0);
    m_table->setCellWidget(row, 3, lockWidget);

    // 4. 形状
    auto *shapeCombo = new QComboBox(this);
    shapeCombo->addItem(QStringLiteral("矩形"), 0);
    shapeCombo->addItem(QStringLiteral("圆形"), 1);
    shapeCombo->addItem(QStringLiteral("菱形"), 2);
    int sIdx = shapeCombo->findData(r.shape);
    if (sIdx >= 0) shapeCombo->setCurrentIndex(sIdx);
    m_table->setCellWidget(row, 4, shapeCombo);

    // 5. 颜色
    auto *colorBtn = new QPushButton(this);
    QColor c = r.color.isValid() ? r.color : QColor(0, 212, 255);
    colorBtn->setProperty("roomColor", c);
    colorBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: #0F172A; border: 1px solid #E2E8F0; "
        "border-radius: 3px; font-weight: bold; min-width: 60px; }").arg(c.name()));
    colorBtn->setText(c.name().toUpper());
    connect(colorBtn, &QPushButton::clicked, this, [this, row]() { onSelectColor(row); });
    m_table->setCellWidget(row, 5, colorBtn);

    // 6. 操作 (删除)
    auto *delBtn = new QPushButton(QStringLiteral("✕ 删除"), this);
    delBtn->setStyleSheet("QPushButton{color:#F87171;background:#3E1E1E;border:1px solid #F87171;}QPushButton:hover{background:#F87171;color:#1E1E1E;}");
    connect(delBtn, &QPushButton::clicked, this, [this, row]() {
      m_table->removeRow(row);
      if (row < m_rooms.size()) {
        m_rooms.removeAt(row);
      }
      populateTable();
    });
    m_table->setCellWidget(row, 6, delBtn);
  }
}

void RoomManagerDialog::onAddRoom() {
  RoomRegion newRoom;
  int count = m_rooms.size() + 1;
  newRoom.id = QStringLiteral("room_%1").arg(QDateTime::currentMSecsSinceEpoch());
  newRoom.name = QStringLiteral("%1号机房").arg(count);
  newRoom.targetView = m_viewNames.value(0, QStringLiteral("界面1"));
  newRoom.shape = 0;
  newRoom.geom = QRect(50 + (count % 3) * 380, 50 + (count / 3) * 280, 350, 250);
  newRoom.visible = true;
  newRoom.isLocked = false;
  newRoom.color = QColor(0, 212, 255);

  m_rooms.append(newRoom);
  populateTable();
}

void RoomManagerDialog::onDeleteRoomRow() {
  int row = m_table->currentRow();
  if (row >= 0 && row < m_rooms.size()) {
    m_rooms.removeAt(row);
    populateTable();
  }
}

void RoomManagerDialog::onSelectColor(int row) {
  auto *colorBtn = qobject_cast<QPushButton *>(m_table->cellWidget(row, 5));
  if (!colorBtn) return;

  QColor curColor = colorBtn->property("roomColor").value<QColor>();
  QColor newColor = QColorDialog::getColor(curColor, this, QStringLiteral("选择房间主题颜色"));
  if (newColor.isValid()) {
    colorBtn->setProperty("roomColor", newColor);
    colorBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: #0F172A; border: 1px solid #E2E8F0; "
        "border-radius: 3px; font-weight: bold; min-width: 60px; }").arg(newColor.name()));
    colorBtn->setText(newColor.name().toUpper());
    if (row < m_rooms.size()) {
      m_rooms[row].color = newColor;
    }
  }
}

void RoomManagerDialog::onToggleSelectAll(bool select) {
  for (int r = 0; r < m_table->rowCount(); ++r) {
    auto *visWidget = m_table->cellWidget(r, 2);
    if (visWidget) {
      auto *visCheck = visWidget->findChild<QCheckBox *>();
      if (visCheck) {
        visCheck->setChecked(select);
      }
    }
  }
}

QList<RoomRegion> RoomManagerDialog::rooms() const {
  return m_rooms;
}
