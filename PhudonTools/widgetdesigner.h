#ifndef WIDGETDESIGNER_H
#define WIDGETDESIGNER_H

#include "customwidget.h"
#include <QScrollArea>
#include <QWidget>

// 控件设计器区域 - 支持拖放控件
class WidgetDesignerArea : public QWidget {
  Q_OBJECT

public:
  explicit WidgetDesignerArea(QWidget *parent = nullptr);

  // 添加自定义协议按钮
  void addProtocolButton(const QPoint &pos = QPoint());

  // 添加示波器组件
  void addScopeWidget(const QPoint &pos = QPoint());

  // 添加LED组件
  void addLedWidget(const QPoint &pos = QPoint());

signals:
  void sendData(const QByteArray &data);
  void scopeDataReceived(const QByteArray &data);
  void ledDataReceived(const QByteArray &data);
  void ledStatesReceived(const QVector<int> &states);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  QVector<CustomProtocolButton *> m_protocolButtons;
  QVector<class CustomScopeWidget *> m_scopeWidgets;
  QVector<class CustomLedWidget *> m_ledWidgets;
  int m_nextButtonId = 1;
  int m_nextScopeId = 1;
  int m_nextLedId = 1;
};

// 控件工具箱 - 提供可拖放的控件模板
class WidgetToolbox : public QWidget {
  Q_OBJECT

public:
  explicit WidgetToolbox(QWidget *parent = nullptr);

signals:
  void addProtocolButton();
  void addScopeWidget();
  void addLedWidget();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

private:
  class QPushButton *m_btnProtocol;
  class QPushButton *m_btnScope;
  class QPushButton *m_btnLed;
  QPoint m_dragStartPos;
  QPushButton *m_draggedButton = nullptr;
};

// 自定义示波器组件
class CustomScopeWidget : public QWidget {
  Q_OBJECT

public:
  explicit CustomScopeWidget(QWidget *parent = nullptr);

  void setFrameConfig(const QByteArray &header, const QByteArray &tail);
  void bindData(const QByteArray &data);

  QString getName() const { return m_name; }
  void setName(const QString &name);

public slots:
  void onDataReceived(const QByteArray &data);

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  void showConfigDialog();
  void parseFrameData(const QByteArray &data);

  QString m_name = "示波器";
  QByteArray m_frameHeader;
  QByteArray m_frameTail;
  class QCustomPlot *m_plot;
  QVector<double> m_xData;
  QVector<double> m_yData;
  double m_xValue = 0;
  int m_maxPoints = 1000;
};

// 示波器配置对话框
class ScopeConfigDialog : public QDialog {
  Q_OBJECT

public:
  explicit ScopeConfigDialog(QWidget *parent = nullptr);

  void setFrameHeader(const QByteArray &header);
  void setFrameTail(const QByteArray &tail);
  QByteArray getFrameHeader() const;
  QByteArray getFrameTail() const;

  void setName(const QString &name);
  QString getName() const;

private:
  class QLineEdit *m_editName;
  class QLineEdit *m_editFrameHeader;
  class QLineEdit *m_editFrameTail;
  class QSpinBox *m_spinMaxPoints;
};

// 自定义LED控件
class CustomLedWidget : public QWidget {
  Q_OBJECT

public:
  explicit CustomLedWidget(QWidget *parent = nullptr);

  void setConfig(const QByteArray &onData, const QByteArray &offData, const QString &color);
  void bindData(const QByteArray &data);

  QString getName() const { return m_name; }
  void setName(const QString &name);
  
  int getBindChannel() const { return m_bindChannel; }
  void setBindChannel(int channel);

public slots:
  void onDataReceived(const QByteArray &data);
  void onLedStatesReceived(const QVector<int> &states);

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  void showConfigDialog();

  QString m_name = "LED灯";
  QByteArray m_onData = QByteArray::fromHex("01");
  QByteArray m_offData = QByteArray::fromHex("00");
  QString m_color = "#F44336"; // 默认红色
  bool m_isOn = false;
  int m_bindChannel = -1; // -1表示未绑定通道
};

// LED配置对话框
class LedConfigDialog : public QDialog {
  Q_OBJECT

public:
  explicit LedConfigDialog(QWidget *parent = nullptr);

  void setOnData(const QByteArray &data);
  void setOffData(const QByteArray &data);
  void setColor(const QString &color);
  void setName(const QString &name);
  void setBindChannel(int channel);

  QByteArray getOnData() const;
  QByteArray getOffData() const;
  QString getColor() const;
  QString getName() const;
  int getBindChannel() const;

private:
  class QLineEdit *m_editName;
  class QLineEdit *m_editOnData;
  class QLineEdit *m_editOffData;
  class QComboBox *m_comboColor;
  class QSpinBox *m_spinBindChannel;
};

#endif // WIDGETDESIGNER_H
