#ifndef APPCARDWIDGET_H
#define APPCARDWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "appmanager.h"

class AppCardWidget : public QFrame {
    Q_OBJECT

public:
    explicit AppCardWidget(const AppInfo &info, QWidget *parent = nullptr);
    ~AppCardWidget() override = default;

    QString getAppId() const { return m_info.id; }
    void updateAppInfo(const AppInfo &info);
    void setRunningState(bool isRunning);

signals:
    void launchRequested(const QString &appId);
    void favoriteToggled(const QString &appId, bool isFavorite);
    void detailsRequested(const QString &appId);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void setupUi();
    void updateStyles();

    AppInfo m_info;
    bool m_isRunning = false;
    bool m_isHovered = false;

    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QLabel *m_versionLabel;
    QLabel *m_categoryBadge;
    QLabel *m_subtitleLabel;
    QLabel *m_descLabel;
    QLabel *m_tagsContainer;
    QLabel *m_statusDot;
    QLabel *m_statusText;
    QToolButton *m_favBtn;
    QPushButton *m_launchBtn;
};

#endif // APPCARDWIDGET_H
