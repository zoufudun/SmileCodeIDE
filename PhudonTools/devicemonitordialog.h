#ifndef DEVICEMONITORDIALOG_H
#define DEVICEMONITORDIALOG_H

#include <QDialog>
#include "devicemonitorpanel.h"
#include "caninterface.h"

class DeviceMonitorDialog : public QDialog {
  Q_OBJECT
public:
  explicit DeviceMonitorDialog(CanInterface *can, QWidget *parent = nullptr);
  ~DeviceMonitorDialog() override;

  void applyThemeStyle(const QString &qss);

private:
  DeviceMonitorPanel *m_panel;
};

#endif // DEVICEMONITORDIALOG_H
