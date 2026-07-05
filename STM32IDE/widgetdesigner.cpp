#include "widgetdesigner.h"
#include "customwidget.h"
#include "../qcustomplot/qcustomplot.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
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
  setMinimumSize(400, 300);
  // Light grid background
  setStyleSheet("WidgetDesignerArea {"
                "  background-color: #F8F9FA;"
                "  border: none;"
                "}");
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

void WidgetDesignerArea::addLedWidget(const QPoint &pos) {
  CustomLedWidget *led = new CustomLedWidget(this);
  led->setName(QString("LED灯 %1").arg(m_nextLedId++));

  if (pos.isNull()) {
    int row = (m_protocolButtons.size() + m_scopeWidgets.size() + 1) / 2;
    led->move(20, 20 + row * 80 + m_ledWidgets.size() * 120);
  } else {
    led->move(pos);
  }

  led->show();
  m_ledWidgets.append(led);

  connect(this, &WidgetDesignerArea::ledDataReceived, led,
          &CustomLedWidget::onDataReceived);
  connect(this, &WidgetDesignerArea::ledStatesReceived, led,
          &CustomLedWidget::onLedStatesReceived);
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
  } else if (widgetType == "LedWidget") {
    addLedWidget(pos);
  }

  event->acceptProposedAction();
}

void WidgetDesignerArea::paintEvent(QPaintEvent *event) {
  QWidget::paintEvent(event);

  // Subtle light dot-grid overlay
  QPainter gridPainter(this);
  gridPainter.setRenderHint(QPainter::Antialiasing, false);
  gridPainter.setPen(Qt::NoPen);
  gridPainter.setBrush(QColor(180, 190, 200, 80));
  const int gridStep = 24;
  for (int x = gridStep; x < width(); x += gridStep) {
    for (int y = gridStep; y < height(); y += gridStep) {
      gridPainter.drawEllipse(QPoint(x, y), 1, 1);
    }
  }

  if (m_protocolButtons.isEmpty() && m_scopeWidgets.isEmpty() && m_ledWidgets.isEmpty()) {
    QPainter painter(this);
    painter.setPen(QColor(156, 163, 175));
    QFont hintFont = font();
    hintFont.setPointSize(11);
    painter.setFont(hintFont);
    painter.drawText(rect(), Qt::AlignCenter,
                     "从左侧工具箱拖放控件到此处\n或点击工具箱按钮添加控件");
  }
}

// ============ WidgetToolbox 实现 ============

WidgetToolbox::WidgetToolbox(QWidget *parent) : QWidget(parent) {
  setMinimumWidth(180);
  setMaximumWidth(320);
  // Clean sidebar: light gray, border on the right
  setStyleSheet("WidgetToolbox {"
                "  background: #F1F3F4;"
                "  border-right: 1px solid #DADCE0;"
                "}");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(14, 18, 14, 18);
  layout->setSpacing(18);

  // Title
  QLabel *title = new QLabel("控件库");
  title->setStyleSheet(
      "font-weight: 700; font-size: 14px; color: #3C4043; letter-spacing: 0.5px;");
  layout->addWidget(title);

  // Helper to build keyboard-style buttons
  auto makeKeyBtn = [](const QString &text, const QString &icon,
                       const QString &color) -> QPushButton * {
    QPushButton *btn = new QPushButton();
    btn->setMinimumHeight(62);
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    btn->setCursor(Qt::OpenHandCursor);
    btn->setToolTip(text);

    // Label inside: icon + text stacked
    QString label = icon.isEmpty() ? text : icon + "\n" + text;
    btn->setText(label);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFFFFF, stop:1 #E8EAED);"
        "  color: #3C4043;"
        "  border: 1px solid #C5C8CE;"
        "  border-bottom: 3px solid %1;"
        "  border-radius: 8px;"
        "  padding: 8px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  text-align: center;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F8F9FA, stop:1 #DFE1E5);"
        "  border-bottom-color: %1;"
        "}"
        "QPushButton:pressed {"
        "  background: #E8EAED;"
        "  border-bottom-width: 1px;"
        "  margin-top: 2px;"
        "}").arg(color));
    return btn;
  };

  // 协议按钮 (keyboard key style)
  m_btnProtocol = makeKeyBtn("命令控件", "📡", "#1A73E8");
  layout->addWidget(m_btnProtocol);

  // 示波器组件 (keyboard key style)
  m_btnScope = makeKeyBtn("波形控件", "📈", "#188038");
  layout->addWidget(m_btnScope);

  // LED控件
  m_btnLed = makeKeyBtn("LED控件", "💡", "#F44336");
  layout->addWidget(m_btnLed);

  layout->addStretch();

  // Hint
  QLabel *hint = new QLabel("拖放到右侧放置区");
  hint->setStyleSheet("color: #9AA0A6; font-size: 11px;");
  hint->setWordWrap(true);
  hint->setAlignment(Qt::AlignCenter);
  layout->addWidget(hint);

  connect(m_btnScope, &QPushButton::clicked, this, &WidgetToolbox::addScopeWidget);
  connect(m_btnProtocol, &QPushButton::clicked, this, &WidgetToolbox::addProtocolButton);
  connect(m_btnLed, &QPushButton::clicked, this, &WidgetToolbox::addLedWidget);
}

void WidgetToolbox::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_dragStartPos = event->pos();
    m_draggedButton = nullptr;

    // Check which button was clicked using geometry
    if (m_btnProtocol && m_btnProtocol->geometry().contains(event->pos())) {
      m_draggedButton = m_btnProtocol;
    } else if (m_btnScope && m_btnScope->geometry().contains(event->pos())) {
      m_draggedButton = m_btnScope;
    } else if (m_btnLed && m_btnLed->geometry().contains(event->pos())) {
      m_draggedButton = m_btnLed;
    }
  }
  QWidget::mousePressEvent(event);
}

void WidgetToolbox::mouseMoveEvent(QMouseEvent *event) {
  if (!(event->buttons() & Qt::LeftButton)) {
    return;
  }

  if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
    return;
  }

  if (!m_draggedButton) {
    return;
  }

  // Create drag operation
  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData();

  if (m_draggedButton == m_btnProtocol) {
    mimeData->setText("ProtocolButton");
  } else if (m_draggedButton == m_btnScope) {
    mimeData->setText("ScopeWidget");
  } else if (m_draggedButton == m_btnLed) {
    mimeData->setText("LedWidget");
  }

  drag->setMimeData(mimeData);

  // Drag preview pixmap
  QPixmap pixmap(m_draggedButton->size());
  m_draggedButton->render(&pixmap);
  drag->setPixmap(pixmap);
  drag->setHotSpot(event->pos() - m_draggedButton->pos());

  drag->exec(Qt::CopyAction);
  m_draggedButton = nullptr;
}

// ============ CustomScopeWidget Implementation ============

CustomScopeWidget::CustomScopeWidget(QWidget *parent) : QWidget(parent) {
  setFixedSize(400, 200);
  setStyleSheet("CustomScopeWidget {"
                "  background: white;"
                "  border: 2px solid #2196F3;"
                "  border-radius: 6px;"
                "}");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(5, 5, 5, 5);

  // Title bar
  QLabel *titleLabel = new QLabel(m_name);
  titleLabel->setStyleSheet("background: #2196F3; color: white; padding: 4px; border-radius: 3px; font-weight: bold;");
  layout->addWidget(titleLabel);

  // Plot
  m_plot = new QCustomPlot(this);
  m_plot->setMinimumHeight(150);
  m_plot->addGraph();
  m_plot->graph(0)->setPen(QPen(QColor(33, 150, 243), 2));
  m_plot->xAxis->setLabel("Time");
  m_plot->yAxis->setLabel("Value");
  m_plot->xAxis->setRange(0, 100);
  m_plot->yAxis->setRange(0, 255);

  // Dark theme for plot
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

void CustomScopeWidget::setFrameConfig(const QByteArray &header, const QByteArray &tail) {
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

// ============ CustomLedWidget Implementation ============

CustomLedWidget::CustomLedWidget(QWidget *parent) : QWidget(parent) {
  setFixedSize(100, 120);
  setStyleSheet("CustomLedWidget { background: transparent; }");
}

void CustomLedWidget::setConfig(const QByteArray &onData, const QByteArray &offData, const QString &color) {
  m_onData = onData;
  m_offData = offData;
  m_color = color;
  update();
}

void CustomLedWidget::setBindChannel(int channel) {
  m_bindChannel = channel;
}

void CustomLedWidget::bindData(const QByteArray &data) {
  onDataReceived(data);
}

void CustomLedWidget::setName(const QString &name) {
  m_name = name;
  update();
}

void CustomLedWidget::onDataReceived(const QByteArray &data) {
  if (m_bindChannel != -1) return; // If channel bound, skip raw byte content match
  if (!m_onData.isEmpty() && data.contains(m_onData)) {
    if (!m_isOn) {
      m_isOn = true;
      update();
    }
  } else if (!m_offData.isEmpty() && data.contains(m_offData)) {
    if (m_isOn) {
      m_isOn = false;
      update();
    }
  }
}

void CustomLedWidget::onLedStatesReceived(const QVector<int> &states) {
  if (m_bindChannel >= 0 && m_bindChannel < states.size()) {
    bool newState = (states[m_bindChannel] != 0);
    if (m_isOn != newState) {
      m_isOn = newState;
      update();
    }
  }
}

void CustomLedWidget::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu(this);
  QAction *configAction = menu.addAction("配置");
  QAction *toggleAction = menu.addAction(m_isOn ? "切换状态(强制熄灭)" : "切换状态(强制点亮)");
  menu.addSeparator();
  QAction *deleteAction = menu.addAction("删除");

  QAction *selected = menu.exec(event->globalPos());
  if (selected == configAction) {
    showConfigDialog();
  } else if (selected == toggleAction) {
    m_isOn = !m_isOn;
    update();
  } else if (selected == deleteAction) {
    deleteLater();
  }
}

void CustomLedWidget::showConfigDialog() {
  LedConfigDialog dialog(this);
  dialog.setName(m_name);
  dialog.setOnData(m_onData);
  dialog.setOffData(m_offData);
  dialog.setColor(m_color);
  dialog.setBindChannel(m_bindChannel);

  if (dialog.exec() == QDialog::Accepted) {
    setName(dialog.getName());
    setConfig(dialog.getOnData(), dialog.getOffData(), dialog.getColor());
    setBindChannel(dialog.getBindChannel());
  }
}

void CustomLedWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  int ledSize = 60;
  QRect ledRect((width() - ledSize) / 2, 10, ledSize, ledSize);

  QColor baseColor(m_color);
  if (!m_isOn) {
    // 熄灭状态变暗
    baseColor = baseColor.darker(300);
  }

  // 绘制发光光晕
  if (m_isOn) {
    for (int i = 0; i < 3; ++i) {
      int glowSize = ledSize + (i * 10);
      QRect glowRect((width() - glowSize) / 2, 10 - (i * 5), glowSize, glowSize);
      QColor glowColor = baseColor;
      glowColor.setAlpha(50 - (i * 15));
      painter.setPen(Qt::NoPen);
      painter.setBrush(glowColor);
      painter.drawEllipse(glowRect);
    }
  }

  // 绘制LED本体
  QRadialGradient gradient(ledRect.center(), ledSize / 2, ledRect.topLeft() + QPoint(ledSize/3, ledSize/3));
  if (m_isOn) {
    gradient.setColorAt(0, baseColor.lighter(150));
    gradient.setColorAt(0.7, baseColor);
    gradient.setColorAt(1, baseColor.darker(150));
  } else {
    gradient.setColorAt(0, baseColor.lighter(120));
    gradient.setColorAt(1, baseColor.darker(200));
  }

  painter.setPen(QPen(baseColor.darker(300), 2));
  painter.setBrush(gradient);
  painter.drawEllipse(ledRect);

  // 绘制高光边缘
  if (!m_isOn) {
    QPainterPath highlight;
    highlight.addEllipse(ledRect.adjusted(3, 3, -3, -3));
    painter.setPen(QPen(QColor(255, 255, 255, 40), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(highlight);
  }

  // 绘制文本
  painter.setPen(QPen(QColor(60, 64, 67)));
  QFont f = font();
  f.setPointSize(10);
  f.setBold(true);
  painter.setFont(f);
  painter.drawText(QRect(0, ledRect.bottom() + 10, width(), 30), Qt::AlignCenter | Qt::TextWordWrap, m_name);
}

// ============ LedConfigDialog Implementation ============

LedConfigDialog::LedConfigDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle("LED配置");
  setMinimumWidth(350);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  QGroupBox *basicGroup = new QGroupBox("配置");
  QFormLayout *formLayout = new QFormLayout(basicGroup);

  m_editName = new QLineEdit();
  formLayout->addRow("名称:", m_editName);

  m_comboColor = new QComboBox();
  m_comboColor->addItem("红色", "#F44336");
  m_comboColor->addItem("绿色", "#4CAF50");
  m_comboColor->addItem("蓝色", "#2196F3");
  m_comboColor->addItem("黄色", "#FFEB3B");
  m_comboColor->addItem("橙色", "#FF9800");
  m_comboColor->addItem("紫色", "#9C27B0");
  formLayout->addRow("颜色:", m_comboColor);

  m_editOnData = new QLineEdit();
  m_editOnData->setPlaceholderText("HEX格式，如: 01");
  formLayout->addRow("点亮匹配数据(HEX):", m_editOnData);

  m_editOffData = new QLineEdit();
  m_editOffData->setPlaceholderText("HEX格式，如: 00");
  formLayout->addRow("熄灭匹配数据(HEX):", m_editOffData);

  m_spinBindChannel = new QSpinBox();
  m_spinBindChannel->setRange(-1, 255);
  m_spinBindChannel->setValue(-1);
  m_spinBindChannel->setSpecialValueText("未绑定 (-1)");
  formLayout->addRow("绑定通道:", m_spinBindChannel);

  mainLayout->addWidget(basicGroup);

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

void LedConfigDialog::setOnData(const QByteArray &data) {
  m_editOnData->setText(data.toHex(' ').toUpper());
}

void LedConfigDialog::setOffData(const QByteArray &data) {
  m_editOffData->setText(data.toHex(' ').toUpper());
}

void LedConfigDialog::setColor(const QString &color) {
  for (int i = 0; i < m_comboColor->count(); ++i) {
    if (m_comboColor->itemData(i).toString().toUpper() == color.toUpper()) {
      m_comboColor->setCurrentIndex(i);
      return;
    }
  }
}

void LedConfigDialog::setName(const QString &name) {
  m_editName->setText(name);
}

void LedConfigDialog::setBindChannel(int channel) {
  m_spinBindChannel->setValue(channel);
}

QByteArray LedConfigDialog::getOnData() const {
  return QByteArray::fromHex(m_editOnData->text().toLatin1());
}

QByteArray LedConfigDialog::getOffData() const {
  return QByteArray::fromHex(m_editOffData->text().toLatin1());
}

QString LedConfigDialog::getColor() const {
  return m_comboColor->currentData().toString();
}

QString LedConfigDialog::getName() const {
  return m_editName->text();
}

int LedConfigDialog::getBindChannel() const {
  return m_spinBindChannel->value();
}
