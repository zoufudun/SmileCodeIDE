#ifndef CANFLASHPANEL_H
#define CANFLASHPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include "caninterface.h"
#include "../CAN/Engine/isotpengine.h"
#include "../CAN/Engine/udsengine.h"
#include "../CAN/Service/flashservice.h"

class CanFlashPanel : public QWidget {
  Q_OBJECT
public:
  explicit CanFlashPanel(CanInterface *can, QWidget *parent = nullptr);
  ~CanFlashPanel() override;

  void applyThemeStyle(const QString &qss);

signals:
  void closeRequested(QWidget *panel);

private slots:
  void onBrowseFileClicked();
  void onStartFlashClicked();
  void onAbortFlashClicked();

  void onStepChanged(FlashStep step, const QString &stepDesc);
  void onProgressChanged(int percent, double speedKBs, int bytesTransferred, int totalBytes);
  void onFlashFinished(bool success, const QString &message);
  void onLogMessage(const QString &msg);

private:
  void setupUi();
  void log(const QString &msg);

  CanInterface *m_can = nullptr;
  IsoTpEngine *m_isoTp = nullptr;
  UdsEngine *m_uds = nullptr;
  FlashService *m_flashService = nullptr;

  QWidget *m_headerWidget = nullptr;
  QLabel *m_titleLabel = nullptr;
  QPushButton *m_closeBtn = nullptr;

  QLineEdit *m_editFilePath = nullptr;
  QPushButton *m_btnBrowse = nullptr;
  QTreeWidget *m_segmentTree = nullptr;

  QLineEdit *m_editBaseAddr = nullptr;
  QLineEdit *m_editBlockSize = nullptr;
  QLineEdit *m_editSecurityKey = nullptr;

  QPushButton *m_btnStartFlash = nullptr;
  QPushButton *m_btnAbortFlash = nullptr;

  QLabel *m_stepLabel = nullptr;
  QProgressBar *m_progressBar = nullptr;
  QLabel *m_speedLabel = nullptr;

  QPlainTextEdit *m_logEdit = nullptr;
};

#endif // CANFLASHPANEL_H
