#ifndef APPHUBWINDOW_H
#define APPHUBWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QMap>
#include "appmanager.h"
#include "appcardwidget.h"

class AppHubWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AppHubWindow(QWidget *parent = nullptr);
    ~AppHubWindow() override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onCategorySelected(int row);
    void onSearchTextChanged(const QString &text);
    void onThemeChanged(int index);
    void onOpenPluginManagerClicked();
    void onShowAboutClicked();
    void onAppLaunched(const QString &appId, QWidget *widget);
    void onAppStatusChanged(const QString &appId, bool isRunning);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void refreshAppGrid();

private:
    void setupUi();
    void setupHeader();
    void setupSidebar();
    void setupCentralArea();
    void setupFooter();
    void setupTrayIcon();
    void applyTheme(const QString &themeName);

    // UI Elements
    QLineEdit *m_searchEdit;
    QComboBox *m_themeCombo;
    QListWidget *m_sidebarList;
    QScrollArea *m_scrollArea;
    QWidget *m_gridContainer;
    QGridLayout *m_gridLayout;
    QLabel *m_emptyStateLabel;

    // Status Labels
    QLabel *m_statusRunningLabel;
    QLabel *m_statusTotalLabel;

    // System Tray
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;

    // Current State
    AppCategory m_currentCategory = AppCategory::All;
    QString m_searchKeyword;
    QMap<QString, AppCardWidget*> m_cardMap;
    int m_lastColumnCount = 0;
};

#endif // APPHUBWINDOW_H
