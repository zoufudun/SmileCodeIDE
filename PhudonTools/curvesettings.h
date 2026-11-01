#ifndef CURVESETTINGS_H
#define CURVESETTINGS_H

#include <QDialog>
#include <QPushButton>
#include <QTableWidget>


class QCustomPlot;

class CurveSettingsDialog : public QDialog {
  Q_OBJECT

public:
  explicit CurveSettingsDialog(QCustomPlot *customPlot,
                               QWidget *parent = nullptr);
  ~CurveSettingsDialog();

private slots:
  void applySettings();
  void pickColor(int row);

private:
  void setupUi();
  void loadCurrentSettings();

  QCustomPlot *m_customPlot;
  QTableWidget *m_tableWidget;
  QPushButton *m_btnApply;
  QPushButton *m_btnOk;
  QPushButton *m_btnCancel;
};

#endif // CURVESETTINGS_H
