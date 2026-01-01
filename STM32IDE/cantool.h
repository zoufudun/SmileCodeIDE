#ifndef CANTOOL_H
#define CANTOOL_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTreeWidget>
#include <QVBoxLayout>


class CANTool : public QDialog {
  Q_OBJECT
public:
  explicit CANTool(QWidget *parent = nullptr);

private:
  void setupUi();

  QComboBox *m_baudRateComboBox;
  QComboBox *m_modeComboBox;
  QPushButton *m_connectButton;
  QTreeWidget *m_receiveTreeWidget;
  QLineEdit *m_idLineEdit;
  QLineEdit *m_dataLineEdit;
  QPushButton *m_sendButton;
  QRadioButton *m_stdFrameBtn;
  QRadioButton *m_extFrameBtn;
};

#endif // CANTOOL_H
