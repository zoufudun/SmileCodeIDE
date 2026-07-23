#ifndef CANBUSUTILIZATIONDIALOG_H
#define CANBUSUTILIZATIONDIALOG_H

#include <QDialog>
#include <QPointer>
#include <QHash>
#include <QVector>
#include <QElapsedTimer>
#include <QTimer>
#include "../qcustomplot/qcustomplot.h"
#include "caninterface.h"

class QComboBox;
class QLabel;
class QPushButton;

class CanBusUtilizationDialog : public QDialog {
  Q_OBJECT
public:
  explicit CanBusUtilizationDialog(CanInterface *can, QWidget *parent = nullptr);
  ~CanBusUtilizationDialog() override;

private slots:
  void onTimerTimeout();
  void onRefreshPeriodChanged(const QString &text);
  void onSaveClicked();
  void onFrameReceived(const CanFrame &frame);
  void onFrameSent(const CanFrame &frame);
  void refreshChannels();

private:
  void setupUi();
  void setupPlot(QCustomPlot *plot);
  
  CanInterface *m_can;
  QTimer *m_timer;
  QElapsedTimer m_elapsedTimer;

  // Left Plot Components
  QComboBox *m_leftChannelCombo;
  QLabel *m_leftRateLabel;
  QLabel *m_leftUsageLabel;
  QCustomPlot *m_leftPlot;
  QVector<double> m_leftX, m_leftY;

  // Right Plot Components
  QComboBox *m_rightChannelCombo;
  QLabel *m_rightRateLabel;
  QLabel *m_rightUsageLabel;
  QCustomPlot *m_rightPlot;
  QVector<double> m_rightX, m_rightY;

  // Bottom Settings
  QPushButton *m_saveBtn;
  QComboBox *m_refreshCombo;

  // Counters
  QHash<int, int> m_frameCount;
  QHash<int, int> m_bitCount;
  int m_pointCounter = 0;

  // Saved points cache for Export CSV
  struct ExportData {
    int pointIndex;
    int leftCh;
    double leftVal;
    int rightCh;
    double rightVal;
  };
  QVector<ExportData> m_exportHistory;
};

#endif // CANBUSUTILIZATIONDIALOG_H
