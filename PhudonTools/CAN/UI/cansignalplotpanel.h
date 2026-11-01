#ifndef CANSIGNALPLOTPANEL_H
#define CANSIGNALPLOTPANEL_H

#include <QWidget>
#include <QMap>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include "../qcustomplot/qcustomplot.h"
#include "caninterface.h"
#include "../CAN/Engine/dbcparserengine.h"

class CanSignalPlotPanel : public QWidget {
  Q_OBJECT
public:
  explicit CanSignalPlotPanel(CanInterface *can, QWidget *parent = nullptr);
  ~CanSignalPlotPanel() override;

  void applyThemeStyle(const QString &qss);

signals:
  void closeRequested(QWidget *panel);

private slots:
  void onLoadDbcClicked();
  void onFrameReceived(const CanFrame &frame);
  void onSignalSelectionChanged();
  void onPauseResumeClicked();
  void onClearDataClicked();

private:
  void setupUi();
  void updateCurveColors();

  CanInterface *m_can = nullptr;
  DbcParserEngine *m_dbc = nullptr;

  QWidget *m_headerWidget = nullptr;
  QLabel *m_titleLabel = nullptr;
  QPushButton *m_closeBtn = nullptr;
  QCustomPlot *m_plot = nullptr;
  QListWidget *m_signalListWidget = nullptr;
  QPushButton *m_btnLoadDbc = nullptr;
  QPushButton *m_btnPause = nullptr;
  QPushButton *m_btnClear = nullptr;
  QLabel *m_statusLabel = nullptr;

  bool m_paused = false;
  qint64 m_startTimeMs = 0;

  // 选中的信号映射: signalName -> QCPGraph*
  QMap<QString, QCPGraph*> m_graphs;
  QMap<QString, int> m_signalIdMap; // signalName -> msgId
};

#endif // CANSIGNALPLOTPANEL_H
