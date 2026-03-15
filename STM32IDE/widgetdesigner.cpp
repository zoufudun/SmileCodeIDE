#include "widgetdesigner.h"
#include "customwidget.h"
#include "../qcustomplot/qcustomplot.h"
#include <QApplication>
#include <QCheckBox>
#include <QDrag>
#include <QDragEnterEvent>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

// ============ WidgetDesignerArea 实现 ============

WidgetDesignerArea::WidgetDesignerArea(QWidget *parent) : QWidget(parent) {
  setMinimumSize(600, 400);
  setStyleSheet("WidgetDesignerArea {"
                "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                "    stop:0 #f5f5f5, stop:1 #e0e0e0);"
                "  border: 2px dashed #9E9E9E;"
                "  border-radius: 8px;"
                "}");

  // 使用网格布局实现可自由拖动的控件放置
  // 不设置固定布局，允许控件自由定位
  setAcceptDrops(true);
}

void WidgetDesignerArea::addProtocolButton(const QPoint &pos) {
  CustomProtocolButton *btn = new CustomProtocolButton(this);
  btn->setText(QString("协议按钮 %1").arg(m_nextButtonId++));

  ProtocolConfig config;
  config.name = btn->text();
  config.useFrameHeader = true;
  config.frameHeader = QByteArray::fromHex("AA55");
  config.useFrameTail = true;
  config.frameTail = QByteArray::fromHex("0D0A");
  btn->setProtocolConfig(config);

  // 自由定位控件
  if (pos.isNull()) {
    // 默认位置：按网格排列
    int col = (m_protocolButtons.size() % 2);
    int row = (m_protocolButtons.size() / 2);
    btn->move(20 + col * 220, 20 + row * 80);
  } else {
    btn->move(pos);
  }

  btn->show();
  m_protocolButtons.append(btn);

  connect(btn, &CustomProtocolButton::sendData, this,
          &WidgetDesignerArea::sendData);
}

void WidgetDesignerArea::addScopeWidget(const QPoint &pos) {
  CustomScopeWidget *scope = new CustomScopeWidget(this);
  scope->setName(QString("示波器 %1").arg(m_nextScopeId++));

  // 自由定位控件
  if (pos.isNull()) {
    // 默认位置：在协议按钮下方
    int row = (m_protocolButtons.size() + 1) / 2;
    scope->move(20, 20 + row * 80 + m_scopeWidgets.size() * 220);
  } else {
    scope->move(pos);
  }

  scope->show();
  m_scopeWidgets.append(scope);

  connect(this, &WidgetDesignerArea::scopeDataReceived, scope,
          &CustomScopeWidget::onDataReceived);
}

void WidgetDesignerArea::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasText()) {
    event->acceptProposedAction();
  }
}

void WidgetDesignerArea::dragMoveEvent(QDragMoveEvent *event) {
  event->acceptProposedAction();
}

void WidgetDesignerArea::dropEvent(QDropEvent *event) {
  QString widgetType = event->mimeData()->text();
  QPoint pos = event->pos();

  if (widgetType == "ProtocolButton") {
    addProtocolButton(pos);
  } else if (widgetType == "ScopeWidget") {
    addScopeWidget(pos);
  }

  event->acceptProposedAction();
}

void WidgetDesignerArea::paintEvent(QPaintEvent *event) {
  QWidget::paintEvent(event);

  if (m_protocolButtons.isEmpty() && m_scopeWidgets.isEmpty()) {
    QPainter painter(this);
    painter.setPen(QColor(158, 158, 158));
    painter.setFont(QFont("Arial", 12));
    painter.drawText(rect(), Qt::AlignCenter,
                     "从左侧工具箱拖放控件到此处\n或点击工具箱按钮添加控件");
  }
}

// ============ WidgetToolbox 实现 ============

WidgetToolbox::WidgetToolbox(QWidget *parent) : QWidget(parent) {
  setFixedWidth(200);
  setStyleSheet("WidgetToolbox {"
                "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                "    stop:0 #FAFAFA, stop:1 #F5F5F5);"
                "  border-right: 2px solid #E0E0E0;"
                "}");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(15, 15, 15, 15);
  layout->setSpacing(15);

  QLabel *title = new QLabel("控件库");
  title->setStyleSheet("font-weight: bold; font-size: 16px; color: #424242;");
  layout->addWidget(title);

  // 协议按钮
  m_btnProtocol = new QPushButton("协议按钮");
  m_btnProtocol->setMinimumHeight(80);
  m_btnProtocol->setStyleSheet(
      "QPushButton {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #667eea, stop:1 #764ba2);"
      "  color: white;"
      "  border: none;"
      "  border-radius: 8px;"
      "  padding: 15px;"
      "  font-size: 14pt;"
      "  font-weight: bold;"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #7b8ff0, stop:1 #8a5cb8);"
      "}");
  layout->addWidget(m_btnProtocol);

  // 示波器组件
  m_btnScope = new QPushButton("示波器");
  m_btnScope->setMinimumHeight(80);
  m_btnScope->setStyleSheet(
      "QPushButton {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #11998e, stop:1 #38ef7d);"
      "  color: white;"
      "  border: none;"
      "  border-radius: 8px;"
      "  padding: 15px;"
      "  font-size: 14pt;"
      "  font-weight: bold;"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #1aaa9e, stop:1 #48ff8d);"
      "}");
  layout->addWidget(m_btnScope);

  layout->addStretch();

  QLabel *hint = new QLabel("点击按钮添加控件\n或拖放到右侧设计区");
  hint->setStyleSheet("color: #757575; font-size: 11px;");
  hint->setWordWrap(true);
  hint->setAlignment(Qt::AlignCenter);
  layout->addWidget(hint);

  // 连接点击信号
  connect(m_btnProtocol, &QPushButton::clicked, this,
          &WidgetToolbox::addProtocolButton);
  connect(m_btnScope, &QPushButton::clicked, this,
          &WidgetToolbox::addScopeWidget);
}

void WidgetToolbox::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_dragStartPos = event->pos();

    // 检查点击的是哪个按钮
    QWidget *child = childAt(event->pos());
    if (child == m_btnProtocol || child == m_btnScope) {
      m_draggedButton = qobject_cast<QPushButton *>(child);
    }
  }
  QWidget::mousePressEvent(event);
}

void WidgetToolbox::mouseMoveEvent(QMouseEvent *event) {
  if (!(event->buttons() & Qt::LeftButton)) {
    return;
  }

  if ((event->pos() - m_dragStartPos).manhattanLength() <
      QApplication::startDragDistance()) {
    return;
  }

  if (!m_draggedButton) {
    return;
  }

  // 创建拖放操作
  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData();

  if (m_draggedButton == m_btnProtocol) {
    mimeData->setText("ProtocolButton");
  } else if (m_draggedButton == m_btnScope) {
    mimeData->setText("ScopeWidget");
  }

  drag->setMimeData(mimeData);

  // 创建拖放预览图
  QPixmap pixmap(m_draggedButton->size());
  m_draggedButton->render(&pixmap);
  drag->setPixmap(pixmap);
  drag->setHotSpot(event->pos() - m_draggedButton->pos());

  drag->exec(Qt::CopyAction);
  m_draggedButton = nullptr;
}

// ============ CustomScopeWidget 实现 ============

CustomScopeWidget::CustomScopeWidget(QWidget *parent) : QWidget(parent) {
  setFixedSize(400, 200);
  setStyleSheet("CustomScopeWidget {"
                "  background: white;"
                "  border: 2px solid #2196F3;"
                "  border-radius: 6px;"
                "}");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(5, 5, 5, 5);

  // 标题栏
  QLabel *titleLabel = new QLabel(m_name);
  titleLabel->setStyleSheet(
      "background: #2196F3; color: white; padding: 4px; border-radius: 3px;");
  layout->addWidget(titleLabel);

  // 图表
  m_plot = new QCustomPlot(this);
  m_plot->setMinimumHeight(150);
  m_plot->addGraph();
  m_plot->graph(0)->setPen(QPen(QColor(33, 150, 243), 2));
  m_plot->xAxis->setLabel("时间");
  m_plot->yAxis->setLabel("数值");
  m_plot->xAxis->setRange(0, 100);
  m_plot->yAxis->setRange(0, 255);

  // 设置暗黑主题
  m_plot->setBackground(QColor(30, 30, 30));
  m_plot->xAxis->setBasePen(QPen(Qt::white));
  m_plot->yAxis->setBasePen(QPen(Qt::white));
  m_plot->xAxis->setTickPen(QPen(Qt::white));
  m_plot->yAxis->setTickPen(QPen(Qt::white));
  m_plot->xAxis->setSubTickPen(QPen(Qt::white));
  m_plot->yAxis->setSubTickPen(QPen(Qt::white));
  m_plot->xAxis->setTickLabelColor(Qt::white);
  m_plot->yAxis->setTickLabelColor(Qt::white);
  m_plot->xAxis->setLabelColor(Qt::white);
  m_plot->yAxis->setLabelColor(Qt::white);
  m_plot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
  m_plot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));

  layout->addWidget(m_plot);
}

void CustomScopeWidget::setName(const QString &name) {
  m_name = name;
  QLabel *titleLabel = findChild<QLabel *>();
  if (titleLabel) {
    titleLabel->setText(name);
  }
}

void CustomScopeWidget::setFrameConfig(const QByteArray &header,
                                        const QByteArray &tail) {
  m_frameHeader = header;
  m_frameTail = tail;
}

void CustomScopeWidget::bindData(const QByteArray &data) {
  parseFrameData(data);
}

void CustomScopeWidget::onDataReceived(const QByteArray &data) {
  parseFrameData(data);
}

void CustomScopeWidget::parseFrameData(const QByteArray &data) {
  QByteArray payload = data;

  // 去除帧头
  if (!m_frameHeader.isEmpty() && data.startsWith(m_frameHeader)) {
    payload = payload.mid(m_frameHeader.size());
  }

  // 去除帧尾
  if (!m_frameTail.isEmpty() && payload.endsWith(m_frameTail)) {
    payload = payload.left(payload.size() - m_frameTail.size());
  }

  // 解析数据并添加到图表
  for (int i = 0; i < payload.size(); ++i) {
    quint8 value = static_cast<quint8>(payload[i]);
    m_xData.append(m_xValue++);
    m_yData.append(value);

    // 限制数据点数量
    if (m_xData.size() > m_maxPoints) {
      m_xData.removeFirst();
      m_yData.removeFirst();
    }
  }

  // 更新图表
  m_plot->graph(0)->setData(m_xData, m_yData);
  if (!m_xData.isEmpty()) {
    m_plot->xAxis->setRange(m_xData.first(), m_xData.last());
  }
  m_plot->replot();
}

void CustomScopeWidget::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu(this);
  QAction *configAction = menu.addAction("配置");
  QAction *clearAction = menu.addAction("清空数据");
  QAction *deleteAction = menu.addAction("删除");

  QAction *selected = menu.exec(event->globalPos());
  if (selected == configAction) {
    showConfigDialog();
  } else if (selected == clearAction) {
    m_xData.clear();
    m_yData.clear();
    m_xValue = 0;
    m_plot->graph(0)->setData(m_xData, m_yData);
    m_plot->replot();
  } else if (selected == deleteAction) {
    deleteLater();
  }
}

void CustomScopeWidget::showConfigDialog() {
  ScopeConfigDialog dialog(this);
  dialog.setName(m_name);
  dialog.setFrameHeader(m_frameHeader);
  dialog.setFrameTail(m_frameTail);

  if (dialog.exec() == QDialog::Accepted) {
    setName(dialog.getName());
    m_frameHeader = dialog.getFrameHeader();
    m_frameTail = dialog.getFrameTail();
  }
}

void CustomScopeWidget::paintEvent(QPaintEvent *event) {
  QWidget::paintEvent(event);
}

// ============ ScopeConfigDialog 实现 ============

ScopeConfigDialog::ScopeConfigDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle("示波器配置");
  setMinimumWidth(400);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  QGroupBox *basicGroup = new QGroupBox("基本设置");
  QFormLayout *basicLayout = new QFormLayout(basicGroup);

  m_editName = new QLineEdit();
  basicLayout->addRow("名称:", m_editName);

  m_spinMaxPoints = new QSpinBox();
  m_spinMaxPoints->setRange(100, 10000);
  m_spinMaxPoints->setValue(1000);
  basicLayout->addRow("最大数据点:", m_spinMaxPoints);

  mainLayout->addWidget(basicGroup);

  QGroupBox *frameGroup = new QGroupBox("帧格式配置");
  QFormLayout *frameLayout = new QFormLayout(frameGroup);

  m_editFrameHeader = new QLineEdit();
  m_editFrameHeader->setPlaceholderText("例如: AA 55");
  frameLayout->addRow("帧头 (HEX):", m_editFrameHeader);

  m_editFrameTail = new QLineEdit();
  m_editFrameTail->setPlaceholderText("例如: 0D 0A");
  frameLayout->addRow("帧尾 (HEX):", m_editFrameTail);

  mainLayout->addWidget(frameGroup);

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  QPushButton *btnOk = new QPushButton("确定");
  QPushButton *btnCancel = new QPushButton("取消");
  buttonLayout->addWidget(btnOk);
  buttonLayout->addWidget(btnCancel);
  mainLayout->addLayout(buttonLayout);

  connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
  connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void ScopeConfigDialog::setName(const QString &name) {
  m_editName->setText(name);
}

QString ScopeConfigDialog::getName() const { return m_editName->text(); }

void ScopeConfigDialog::setFrameHeader(const QByteArray &header) {
  m_editFrameHeader->setText(header.toHex(' ').toUpper());
}

void ScopeConfigDialog::setFrameTail(const QByteArray &tail) {
  m_editFrameTail->setText(tail.toHex(' ').toUpper());
}

QByteArray ScopeConfigDialog::getFrameHeader() const {
  return QByteArray::fromHex(m_editFrameHeader->text().toLatin1());
}

QByteArray ScopeConfigDialog::getFrameTail() const {
  return QByteArray::fromHex(m_editFrameTail->text().toLatin1());
}
