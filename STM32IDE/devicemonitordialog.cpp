#include "devicemonitordialog.h"
#include <QVBoxLayout>

DeviceMonitorDialog::DeviceMonitorDialog(CanInterface *can, QWidget *parent)
    : QDialog(parent) {
  setWindowTitle("设备状态监控 (Status Monitor)");
  resize(850, 600);
  setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
  setAttribute(Qt::WA_DeleteOnClose);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);

  m_panel = new DeviceMonitorPanel(can, this);
  layout->addWidget(m_panel, 1);
}

DeviceMonitorDialog::~DeviceMonitorDialog() = default;

void DeviceMonitorDialog::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss);
}
