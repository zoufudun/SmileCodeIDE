#ifndef ABOUTUPDATEDIALOG_H
#define ABOUTUPDATEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QFrame>
#include <QTabWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>

class AboutUpdateDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutUpdateDialog(QWidget *parent = nullptr);
    ~AboutUpdateDialog() override = default;

private slots:
    void onCheckContainerUpdate();
    void onCheckAllPluginsUpdate();
    void onCheckSinglePluginUpdate(const QString &pluginId);
    void onOpenWebsite();
    void onOpenChangelog();
    void onOpenGitHubStar();
    void onDismissStarBanner();

private:
    void setupUi();
    QWidget* createContainerCard();
    QWidget* createStarBanner();
    QWidget* createPluginsCard();
    QWidget* createChangelogView();
    void applyCustomStyles();

    // UI Controls
    QLabel *m_containerVerLabel;
    QLabel *m_containerStatusLabel;
    QPushButton *m_checkContainerBtn;
    QProgressBar *m_containerProgressBar;
    QFrame *m_starBannerFrame;

    struct PluginRowWidgets {
        QLabel *nameLabel;
        QLabel *descLabel;
        QLabel *verLabel;
        QLabel *statusLabel;
        QPushButton *checkBtn;
    };
    QMap<QString, PluginRowWidgets> m_pluginRows;
    QPushButton *m_checkAllPluginsBtn;

    QTabWidget *m_mainTabs;
};

#endif // ABOUTUPDATEDIALOG_H

