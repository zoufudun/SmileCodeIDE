#ifndef NORMALSENDDIALOG_H
#define NORMALSENDDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QList>
#include <QPointer>
#include "caninterface.h"

// 报文发送执行器，用于在后台定时发送某一行定义的报文（支持次数、ID/数据递增、以及速度倍率）
class RowSender : public QObject {
  Q_OBJECT
public:
  RowSender(CanInterface *can, int channel, int rowId, quint32 id, bool ext, bool remote, bool fd, bool brs,
            const QByteArray &data, int framesPerSend, int sendCount, int intervalMs,
            bool incId, bool incData, double speedMultiplier, QObject *parent = nullptr);

  void start();
  void stop();
  int rowId() const { return m_rowId; }

signals:
  void statusChanged(int rowId, const QString &statusText);
  void finished(int rowId, bool success);

private slots:
  void onTimeout();

private:
  void sendOnce();

  CanInterface *m_can;
  int m_channel;
  int m_rowId;
  quint32 m_id;
  bool m_ext;
  bool m_remote;
  bool m_fd;
  bool m_brs;
  QByteArray m_data;
  int m_framesPerSend;
  int m_sendCount;
  int m_intervalMs;
  bool m_incId;
  bool m_incData;
  double m_speedMultiplier;
  QTimer *m_timer;
  int m_currentSend = 0;
};

// 单个 Tab 工作空间对应的页面 Widget
class NormalSendPage : public QWidget {
  Q_OBJECT
public:
  explicit NormalSendPage(CanInterface *can, QWidget *parent = nullptr);
  ~NormalSendPage() override;

  void applyThemeStyle(const QString &qss);

private slots:
  // 帧发送
  void onImmediateSend();
  void onAddToList();
  void onImmediateSendTimerTick();

  // 列表操作
  void onSelectAll();
  void onInvertSelection();
  void onMoveUp();
  void onMoveDown();
  void onDeleteSelected();
  void onClearList();
  void onImportList();
  void onExportList();

  // 列表发送
  void onListSendStart();
  void onListSendStop();
  void onListSendTick();
  void onRowSenderFinished(int rowId, bool success);
  void onRowSenderStatus(int rowId, const QString &statusText);
  void onUIRefreshTimer();

private:
  void setupUi();
  void setControlsEnabled(bool enabled);
  void executeNextSerial();
  void startParallelSend();
  void cleanRowSenders();

  CanInterface *m_can;

  // 帧发送 UI
  QComboBox *m_channelCombo;
  QComboBox *m_protocolCombo;
  QCheckBox *m_brsCheck;
  QComboBox *m_frameTypeCombo;
  QComboBox *m_frameFormatCombo;
  QLineEdit *m_idEdit;
  QComboBox *m_lengthCombo;
  QLineEdit *m_dataEdit;
  QLineEdit *m_framesPerSendEdit;
  QLineEdit *m_sendCountEdit;
  QLineEdit *m_intervalEdit;
  QCheckBox *m_incIdCheck;
  QCheckBox *m_incDataCheck;
  QLineEdit *m_nameEdit;
  QPushButton *m_addToListButton;
  QPushButton *m_immediateSendButton;
  QLabel *m_immediateSendTimeLabel;

  // 列表发送 UI
  QTableWidget *m_tableWidget;
  QPushButton *m_selectAllBtn;
  QPushButton *m_invertBtn;
  QPushButton *m_moveUpBtn;
  QPushButton *m_moveDownBtn;
  QPushButton *m_deleteBtn;
  QPushButton *m_clearBtn;
  QPushButton *m_importBtn;
  QPushButton *m_exportBtn;

  QComboBox *m_sendModeCombo;
  QComboBox *m_failBehaviorCombo;
  QLineEdit *m_listSendCountEdit;
  QLineEdit *m_listIntervalEdit;
  QComboBox *m_speedCombo;
  QPushButton *m_listSendButton;

  // 发送状态控制
  bool m_sending = false;
  QDateTime m_listSendStartTime;
  QTimer *m_uiRefreshTimer;
  QTimer *m_listIntervalTimer; // 用于列表发送次数循环之间的延迟

  // 列表发送模式运行状态
  int m_currentListCycle = 0;
  int m_totalListCycles = 1;
  int m_activeSendersCount = 0;
  QList<int> m_checkedRows;
  int m_serialIndex = 0;
  QList<RowSender*> m_activeSenders;

  // 单独即时发送定时控制（如果发送次数 > 1 且间隔 > 0）
  QTimer *m_immTimer;
  quint32 m_immId;
  QByteArray m_immData;
  int m_immRemainingCount;
  int m_immFramesPerSend;
  int m_immInterval;
  bool m_immIncId;
  bool m_immIncData;
  QDateTime m_immStartTime;
};

// 顶层多标签页“普通发送”窗口
class NormalSendDialog : public QDialog {
  Q_OBJECT
public:
  explicit NormalSendDialog(CanInterface *can, QWidget *parent = nullptr);
  ~NormalSendDialog() override;

  void applyThemeStyle(const QString &qss);

private slots:
  void onTabChanged(int index);
  void onCloseTab(int index);

private:
  void createNewTab();

  CanInterface *m_can;
  QTabWidget *m_tabWidget;
  int m_tabCounter = 1;
  QString m_currentQss;
};

#endif // NORMALSENDDIALOG_H
