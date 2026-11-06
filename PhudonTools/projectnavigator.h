#ifndef PROJECTNAVIGATOR_H
#define PROJECTNAVIGATOR_H

#include <QWidget>
#include <QHash>
#include <QSet>
#include "codeeditor.h"
#include "keilproject.h"

class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QFileSystemWatcher;
class QLabel;
class QTimer;

class ProjectNavigator : public QWidget {
    Q_OBJECT
public:
    explicit ProjectNavigator(QWidget *parent = nullptr);
    void setProject(const KeilProject &project);
    void clear();
    bool reload();
    void setSourceBuffer(const QString &path, const QString &text, bool modified = true);
    QTreeView *projectView() const { return m_projectView; }
    QTreeView *functionsView() const { return m_functionsView; }
    QString projectFilePath() const { return m_project.filePath; }
signals:
    void sourceLocationActivated(const QString &file, int line, int column);
    void loadError(const QString &message);
private:
    struct PendingFile { QString path; QStringList includes; };
    void rebuild();
    void startIndexing();
    void indexBatch();
    void enqueue(const QString &path, const QStringList &includes);
    void populate(QStandardItem *item);
    QStandardItem *fileItem(const QString &name, const QString &path);
    void activate(const QModelIndex &index);
    void watch(const QString &path);
    void refreshSymbols();
    QString sourceText(const QString &path) const;
    KeilProject m_project;
    QHash<QString, QList<QStandardItem *>> m_projectFiles;
    QHash<QString, QStandardItem *> m_functionFiles;
    QHash<QString, QList<CodeEditor::FunctionInfo>> m_symbols;
    QList<PendingFile> m_pending;
    QSet<QString> m_queued;
    QSet<QString> m_expandedFunctions;
    QString m_bufferPath;
    QString m_bufferText;
    bool m_bufferModified = false;
    QFileSystemWatcher *m_watcher;
    QTimer *m_reloadTimer;
    QTimer *m_sourceTimer;
    QTimer *m_indexTimer;
    QLabel *m_status;
    QTreeView *m_projectView;
    QTreeView *m_functionsView;
    QStandardItemModel *m_projectModel;
    QStandardItemModel *m_functionsModel;
};

#endif
