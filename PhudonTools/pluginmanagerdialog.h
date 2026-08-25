#ifndef PLUGINMANAGERDIALOG_H
#define PLUGINMANAGERDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include "appmanager.h"

class PluginManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit PluginManagerDialog(QWidget *parent = nullptr);
    ~PluginManagerDialog() override = default;

private slots:
    void refreshPluginList();
    void onInstallJsonClicked();
    void onCreateCustomToolClicked();
    void onOpenPluginsDirClicked();
    void onUninstallClicked(const QString &appId);
    void onEnableToggled(const QString &appId, bool enabled);

private:
    void setupUi();

    QTableWidget *m_tableWidget;
    QPushButton *m_installJsonBtn;
    QPushButton *m_createToolBtn;
    QPushButton *m_refreshBtn;
    QPushButton *m_openDirBtn;
    QLabel *m_countLabel;
};

#endif // PLUGINMANAGERDIALOG_H
