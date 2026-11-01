#ifndef PROJECTSEARCHPANEL_H
#define PROJECTSEARCHPANEL_H

#include <QWidget>
#include "projectsearchengine.h"
class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;

class ProjectSearchPanel : public QWidget {
    Q_OBJECT
public:
    explicit ProjectSearchPanel(QWidget *parent = nullptr);
    ~ProjectSearchPanel() override;
    void setProjectRoot(const QString &root);
    void setCurrentBuffer(const QString &path, const QString &text);
    QString searchText() const;
    bool isSearching() const;
    void startSearch(const QString &query, const ProjectSearchOptions &options = {});
    void findReferences(const QString &symbol);
public slots:
    void focusSearch();
    void cancelSearch();
signals:
    void sourceLocationActivated(const QString &file, int line, int column);
    void searchFinished();
private:
    void runSearch();
    ProjectSearchRequest m_request;
    QLineEdit *m_query = nullptr;
    QCheckBox *m_regex = nullptr;
    QCheckBox *m_case = nullptr;
    QCheckBox *m_word = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QLabel *m_status = nullptr;
    QTreeWidget *m_results = nullptr;
    std::shared_ptr<std::atomic_bool> m_cancel;
    quint64 m_generation = 0;
    bool m_searching = false;
};
#endif
