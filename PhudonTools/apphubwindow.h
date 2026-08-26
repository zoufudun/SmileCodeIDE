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
    void applyTheme(const QString &themeName);

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

    // UI Elements
    QWidget *m_headerWidget;
    QLabel *m_brandLabel;
    QLabel *m_sloganLabel;
    QLineEdit *m_searchEdit;
    QComboBox *m_themeCombo;
    QListWidget *m_sidebarList;
    QScrollArea *m_scrollArea;
    QWidget *m_gridContainer;
    QFrame *m_heroFrame;
    QLabel *m_heroTitle;
    QLabel *m_heroSub;
    QPushButton *m_quickIdeBtn;
    QGridLayout *m_gridLayout;
    QLabel *m_emptyStateLabel;

    // Footer & Status Labels
    QWidget *m_footerWidget;
    QLabel *m_statusRunningLabel;
    QLabel *m_statusTotalLabel;
    QLabel *m_verLabel;

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
