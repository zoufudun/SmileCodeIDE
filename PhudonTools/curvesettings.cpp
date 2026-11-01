#include "curvesettings.h"
#include "../qcustomplot/qcustomplot.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

CurveSettingsDialog::CurveSettingsDialog(QCustomPlot *customPlot,
                                         QWidget *parent)
    : QDialog(parent), m_customPlot(customPlot) {
  setupUi();
  loadCurrentSettings();

  connect(m_btnApply, &QPushButton::clicked, this,
          &CurveSettingsDialog::applySettings);
  connect(m_btnOk, &QPushButton::clicked, [this]() {
    applySettings();
    accept();
  });
  connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

CurveSettingsDialog::~CurveSettingsDialog() {}

void CurveSettingsDialog::setupUi() {
  setWindowTitle("曲线设置");
  resize(500, 300);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  m_tableWidget = new QTableWidget(this);
  m_tableWidget->setColumnCount(5);
  m_tableWidget->setHorizontalHeaderLabels(
      {"通道名称", "颜色", "线宽", "线型", "显示"});
  m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_tableWidget->verticalHeader()->setVisible(false);
  m_tableWidget->setSelectionMode(QAbstractItemView::NoSelection);

  mainLayout->addWidget(m_tableWidget);

  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();

  m_btnApply = new QPushButton("应用", this);
  m_btnOk = new QPushButton("确定", this);
  m_btnCancel = new QPushButton("取消", this);

  btnLayout->addWidget(m_btnApply);
  btnLayout->addWidget(m_btnOk);
  btnLayout->addWidget(m_btnCancel);

  mainLayout->addLayout(btnLayout);
}

void CurveSettingsDialog::pickColor(int row) {
  QPushButton *btnColor =
      qobject_cast<QPushButton *>(m_tableWidget->cellWidget(row, 1));
  if (!btnColor)
    return;

  QColor currentColor = btnColor->palette().color(QPalette::Button);
  QColor newColor = QColorDialog::getColor(currentColor, this, "选择曲线颜色");

  if (newColor.isValid()) {
    btnColor->setStyleSheet(
        QString("background-color: %1; border: 1px solid gray;")
            .arg(newColor.name()));
    btnColor->setProperty("CurveColor", newColor);
  }
}

void CurveSettingsDialog::loadCurrentSettings() {
  if (!m_customPlot)
    return;

  int graphCount = m_customPlot->graphCount();
  m_tableWidget->setRowCount(graphCount);

  for (int i = 0; i < graphCount; ++i) {
    QCPGraph *graph = m_customPlot->graph(i);
    QPen pen = graph->pen();

    // 1. Channel Name
    QTableWidgetItem *nameItem = new QTableWidgetItem(graph->name());
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    m_tableWidget->setItem(i, 0, nameItem);

    // 2. Color picker button
    QPushButton *btnColor = new QPushButton();
    QColor color = pen.color();
    btnColor->setStyleSheet(
        QString("background-color: %1; border: 1px solid gray;")
            .arg(color.name()));
    btnColor->setProperty("CurveColor", color);
    connect(btnColor, &QPushButton::clicked, [this, i]() { pickColor(i); });
    m_tableWidget->setCellWidget(i, 1, btnColor);

    // 3. Line width
    QDoubleSpinBox *spinWidth = new QDoubleSpinBox();
    spinWidth->setRange(0.1, 10.0);
    spinWidth->setSingleStep(0.5);
    spinWidth->setValue(pen.widthF() > 0 ? pen.widthF() : 1.0);
    m_tableWidget->setCellWidget(i, 2, spinWidth);

    // 4. Line style
    QComboBox *comboStyle = new QComboBox();
    comboStyle->addItem("实线 (Solid)", static_cast<int>(Qt::SolidLine));
    comboStyle->addItem("虚线 (Dash)", static_cast<int>(Qt::DashLine));
    comboStyle->addItem("点线 (Dot)", static_cast<int>(Qt::DotLine));
    comboStyle->addItem("点划线 (DashDot)", static_cast<int>(Qt::DashDotLine));
    comboStyle->addItem("双点划线 (DashDotDot)",
                        static_cast<int>(Qt::DashDotDotLine));

    int styleIndex = comboStyle->findData(static_cast<int>(pen.style()));
    if (styleIndex >= 0) {
      comboStyle->setCurrentIndex(styleIndex);
    }
    m_tableWidget->setCellWidget(i, 3, comboStyle);

    // 5. Visibility check
    QWidget *checkWidget = new QWidget();
    QHBoxLayout *checkLayout = new QHBoxLayout(checkWidget);
    checkLayout->setContentsMargins(0, 0, 0, 0);
    checkLayout->setAlignment(Qt::AlignCenter);
    QCheckBox *chkVisible = new QCheckBox();
    chkVisible->setChecked(graph->visible());
    checkLayout->addWidget(chkVisible);
    m_tableWidget->setCellWidget(i, 4, checkWidget);
  }
}

void CurveSettingsDialog::applySettings() {
  if (!m_customPlot)
    return;

  for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
    if (i >= m_customPlot->graphCount())
      break;

    QCPGraph *graph = m_customPlot->graph(i);

    // Retrive values from table
    QPushButton *btnColor =
        qobject_cast<QPushButton *>(m_tableWidget->cellWidget(i, 1));
    QDoubleSpinBox *spinWidth =
        qobject_cast<QDoubleSpinBox *>(m_tableWidget->cellWidget(i, 2));
    QComboBox *comboStyle =
        qobject_cast<QComboBox *>(m_tableWidget->cellWidget(i, 3));

    QWidget *checkWidget = m_tableWidget->cellWidget(i, 4);
    QCheckBox *chkVisible =
        checkWidget ? checkWidget->findChild<QCheckBox *>() : nullptr;

    if (btnColor && spinWidth && comboStyle && chkVisible) {
      QColor newColor = btnColor->property("CurveColor").value<QColor>();
      Qt::PenStyle newStyle =
          static_cast<Qt::PenStyle>(comboStyle->currentData().toInt());
      double newWidth = spinWidth->value();
      bool visible = chkVisible->isChecked();

      QPen newPen = graph->pen();
      newPen.setColor(newColor);
      newPen.setStyle(newStyle);
      newPen.setWidthF(newWidth);

      graph->setPen(newPen);
      graph->setVisible(visible);
    }
  }

  m_customPlot->replot();
}
