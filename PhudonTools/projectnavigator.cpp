#include "projectnavigator.h"
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QStyle>
#include <QTabWidget>
#include <QTextStream>
#include <QTextCodec>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <functional>

namespace {
enum Role { PathRole = Qt::UserRole, KindRole, LineRole, ColumnRole, LoadedRole };
enum Kind { Container, File, Function };
QString pathKey(const QString &path) {
    if (path.isEmpty()) return {};
    const QFileInfo info(path);
    QString key = info.canonicalFilePath();
    if (key.isEmpty()) key = QDir::cleanPath(info.absoluteFilePath());
#ifdef Q_OS_WIN
    key = key.toCaseFolded();
#endif
    return key;
}
bool hasSymbols(const QString &path) {
    static const QStringList suffixes = {"c", "cpp", "cc", "cxx", "h", "hpp", "hh", "hxx"};
    return suffixes.contains(QFileInfo(path).suffix().toLower());
}
QStandardItem *node(const QString &text, const QIcon &icon, Kind kind) {
    auto *item = new QStandardItem(icon, text);
    item->setEditable(false);
    item->setData(kind, KindRole);
    return item;
}
}

ProjectNavigator::ProjectNavigator(QWidget *parent) : QWidget(parent) {
    setObjectName("keilProjectNavigator");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *refresh = new QPushButton(tr("刷新 Keil 工程"), this);
    refresh->setObjectName("refreshKeilProject");
    refresh->setToolTip(tr("重新读取原 .uvprojx 工程和源码函数"));
    layout->addWidget(refresh);
    auto *tabs = new QTabWidget(this);
    tabs->setObjectName("projectNavigationTabs");
    tabs->setTabPosition(QTabWidget::South);
    layout->addWidget(tabs, 1);
    m_projectView = new QTreeView(tabs);
    m_functionsView = new QTreeView(tabs);
    m_projectView->setObjectName("keilProjectTree");
    m_functionsView->setObjectName("projectFunctionsTree");
    m_projectModel = new QStandardItemModel(this);
    m_functionsModel = new QStandardItemModel(this);
    m_projectView->setModel(m_projectModel);
    m_functionsView->setModel(m_functionsModel);
    tabs->addTab(m_projectView, tr("项目"));
    tabs->addTab(m_functionsView, tr("函数"));
    for (auto *view : {m_projectView, m_functionsView}) {
        view->setHeaderHidden(true);
        view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        view->setIndentation(18);
        view->setUniformRowHeights(true);
        view->setTextElideMode(Qt::ElideNone);
        view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        view->header()->setStretchLastSection(false);
        connect(view, &QTreeView::expanded, this, [this, view](const QModelIndex &index) {
            auto *model = qobject_cast<QStandardItemModel *>(view->model());
            populate(model->itemFromIndex(index));
        });
        connect(view, &QTreeView::doubleClicked, this, &ProjectNavigator::activate);
    }
    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);
    m_watcher = new QFileSystemWatcher(this);
    m_reloadTimer = new QTimer(this);
    m_sourceTimer = new QTimer(this);
    m_indexTimer = new QTimer(this);
    m_reloadTimer->setSingleShot(true);
    m_reloadTimer->setInterval(300);
    m_sourceTimer->setSingleShot(true);
    m_sourceTimer->setInterval(200);
    m_indexTimer->setInterval(0);
    connect(refresh, &QPushButton::clicked, this, [this] { reload(); });
    connect(m_reloadTimer, &QTimer::timeout, this, [this] { reload(); });
    connect(m_sourceTimer, &QTimer::timeout, this, &ProjectNavigator::refreshSymbols);
    connect(m_indexTimer, &QTimer::timeout, this, &ProjectNavigator::indexBatch);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (pathKey(path) == pathKey(m_project.filePath)) m_reloadTimer->start();
        else { m_symbols.remove(pathKey(path)); m_sourceTimer->start(); }
        watch(path);
    });
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &path) {
        if (pathKey(path) == pathKey(QFileInfo(m_project.filePath).absolutePath())) m_reloadTimer->start();
        else { m_symbols.clear(); m_sourceTimer->start(); }
    });
}

void ProjectNavigator::watch(const QString &path) {
    if (path.isEmpty()) return;
    const QFileInfo info(path);
    const QString dir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    if (QDir(dir).exists() && !m_watcher->directories().contains(dir)) m_watcher->addPath(dir);
    if (info.isFile() && !m_watcher->files().contains(info.absoluteFilePath())) m_watcher->addPath(info.absoluteFilePath());
}
QStandardItem *ProjectNavigator::fileItem(const QString &name, const QString &path) {
    auto *item = node(name, style()->standardIcon(QStyle::SP_FileIcon), File);
    item->setData(path, PathRole);
    const bool exists = QFileInfo(path).isFile();
    item->setToolTip(exists ? path : tr("文件不存在或路径无法解析: %1").arg(path));
    if (!exists) item->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
    if (hasSymbols(path) && exists) item->appendRow(node(tr("展开以加载函数"), {}, Container));
    return item;
}
void ProjectNavigator::setProject(const KeilProject &project) {
    clear();
    m_project = project;
    rebuild();
}
void ProjectNavigator::clear() {
    m_reloadTimer->stop();
    m_sourceTimer->stop();
    m_indexTimer->stop();
    if (!m_watcher->files().isEmpty()) m_watcher->removePaths(m_watcher->files());
    if (!m_watcher->directories().isEmpty()) m_watcher->removePaths(m_watcher->directories());
    m_project = {};
    m_projectFiles.clear();
    m_functionFiles.clear();
    m_projectModel->clear();
    m_functionsModel->clear();
    m_symbols.clear();
    m_pending.clear();
    m_queued.clear();
    m_expandedFunctions.clear();
    m_bufferPath.clear();
    m_bufferText.clear();
    m_bufferModified = false;
    m_status->clear();
}
void ProjectNavigator::rebuild() {
    m_projectFiles.clear();
    m_projectModel->clear();
    const auto folder = style()->standardIcon(QStyle::SP_DirIcon);
    auto *root = node(tr("工程: %1").arg(QFileInfo(m_project.filePath).completeBaseName()), folder, Container);
    root->setToolTip(m_project.filePath);
    m_projectModel->appendRow(root);
    for (const auto &target : m_project.targets) {
        auto *targetItem = node(target.name, folder, Container);
        root->appendRow(targetItem);
        for (const auto &group : target.groups) {
            auto *groupItem = node(group.name, folder, Container);
            targetItem->appendRow(groupItem);
            for (const auto &file : group.files) {
                auto *item = fileItem(file.name, file.path);
                groupItem->appendRow(item);
                m_projectFiles[pathKey(file.path)].append(item);
            }
        }
    }
    m_projectView->expandToDepth(1);
    watch(m_project.filePath);
    startIndexing();
}
bool ProjectNavigator::reload() {
    if (m_project.filePath.isEmpty()) return false;
    KeilProject updated;
    QString error;
    watch(m_project.filePath);
    if (!updated.load(m_project.filePath, &error)) {
        m_status->setText(tr("读取工程失败，保留上次结构: %1").arg(error));
        emit loadError(error);
        return false;
    }
    if (updated.contentHash == m_project.contentHash) {
        m_symbols.clear();
        refreshSymbols();
        return true;
    }
    QStringList expanded;
    std::function<void(const QModelIndex &, QString)> remember = [&](const QModelIndex &parent, QString key) {
        for (int i = 0; i < m_projectModel->rowCount(parent); ++i) {
            const auto index = m_projectModel->index(i, 0, parent);
            const auto childKey = key + '/' + QString::number(i);
            if (m_projectView->isExpanded(index)) expanded.append(childKey);
            remember(index, childKey);
        }
    };
    remember({}, {});
    m_project = updated;
    m_symbols.clear();
    rebuild();
    for (const auto &key : expanded) {
        QModelIndex index;
        for (const auto &row : key.split('/', Qt::SkipEmptyParts)) index = m_projectModel->index(row.toInt(), 0, index);
        if (index.isValid()) m_projectView->expand(index);
    }
    return true;
}
QString ProjectNavigator::sourceText(const QString &path) const {
    if (m_bufferModified && pathKey(path) == m_bufferPath) return m_bufferText;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    const QByteArray data = file.readAll();
    QTextCodec::ConverterState state;
    const auto utf8 = QTextCodec::codecForName("UTF-8")->toUnicode(data.constData(), data.size(), &state);
    return state.invalidChars ? QTextCodec::codecForLocale()->toUnicode(data) : utf8;
}
void ProjectNavigator::populate(QStandardItem *item) {
    if (!item || item->data(KindRole).toInt() != File || item->data(LoadedRole).toBool()) return;
    const auto path = item->data(PathRole).toString();
    if (!hasSymbols(path)) return;
    item->removeRows(0, item->rowCount());
    const auto key = pathKey(path);
    if (!m_symbols.contains(key)) m_symbols.insert(key, CodeEditor::symbolsForCode(sourceText(path), path));
    for (const auto &symbol : m_symbols.value(key)) {
        if (symbol.type != CodeEditor::SymbolFunction) continue;
        QString params = symbol.params.simplified();
        if (!params.startsWith('(')) params = '(' + params + ')';
        const auto name = symbol.parentClass.isEmpty() ? symbol.scopedName : symbol.parentClass + "::" + symbol.scopedName;
        auto *function = node(name + ' ' + params, style()->standardIcon(QStyle::SP_ArrowRight), Function);
        function->setData(path, PathRole);
        function->setData(symbol.startLine + 1, LineRole);
        function->setData(symbol.startCol + 1, ColumnRole);
        function->setToolTip(tr("%1\n第 %2 行").arg(path).arg(symbol.startLine + 1));
        item->appendRow(function);
    }
    item->setData(true, LoadedRole);
    watch(path);
}
void ProjectNavigator::activate(const QModelIndex &index) {
    const int kind = index.data(KindRole).toInt();
    const auto path = index.data(PathRole).toString();
    if ((kind == File || kind == Function) && !path.isEmpty())
        emit sourceLocationActivated(path, qMax(1, index.data(LineRole).toInt()), qMax(1, index.data(ColumnRole).toInt()));
}
void ProjectNavigator::enqueue(const QString &path, const QStringList &includes) {
    if (!hasSymbols(path)) return;
    const auto contextKey = pathKey(path) + '\n' + includes.join('\n');
    if (m_queued.contains(contextKey)) return;
    m_queued.insert(contextKey);
    m_pending.append({path, includes});
}
void ProjectNavigator::startIndexing() {
    m_indexTimer->stop();
    if (!m_watcher->files().isEmpty()) m_watcher->removePaths(m_watcher->files());
    if (!m_watcher->directories().isEmpty()) m_watcher->removePaths(m_watcher->directories());
    watch(m_project.filePath);
    for (auto *item : m_functionFiles) {
        if (m_functionsView->isExpanded(item->index())) m_expandedFunctions.insert(pathKey(item->data(PathRole).toString()));
        else m_expandedFunctions.remove(pathKey(item->data(PathRole).toString()));
    }
    m_functionFiles.clear();
    m_functionsModel->clear();
    m_pending.clear();
    m_queued.clear();
    for (const auto &target : m_project.targets)
        for (const auto &group : target.groups)
            for (const auto &file : group.files) {
                watch(file.path);
                enqueue(file.path, file.includePaths);
            }
    m_status->setText(tr("正在索引本地头文件…"));
    m_indexTimer->start();
}
void ProjectNavigator::indexBatch() {
    QElapsedTimer elapsed;
    elapsed.start();
    static const QRegularExpression comments(R"(/\*[\s\S]*?\*/|//[^\r\n]*)");
    static const QRegularExpression include(R"(^\s*#\s*include\s*[<"]([^>"\r\n]+)[>"])", QRegularExpression::MultilineOption);
    while (!m_pending.isEmpty() && elapsed.elapsed() < 12) {
        const auto entry = m_pending.takeFirst();
        const auto key = pathKey(entry.path);
        if (!m_functionFiles.contains(key)) {
            auto *item = fileItem(QFileInfo(entry.path).fileName(), entry.path);
            m_functionFiles.insert(key, item);
            m_functionsModel->appendRow(item);
            if (m_expandedFunctions.contains(key)) m_functionsView->expand(item->index());
        }
        watch(entry.path);
        QString text = sourceText(entry.path);
        text.remove(comments);
        auto matches = include.globalMatch(text);
        while (matches.hasNext()) {
            QString name = matches.next().captured(1);
            name.replace('\\', '/');
            QStringList dirs = {QFileInfo(entry.path).absolutePath()};
            dirs.append(entry.includes);
            for (const auto &dir : dirs) {
                const auto candidate = QDir::cleanPath(QDir(dir).absoluteFilePath(name));
                if (QFileInfo(candidate).isFile()) { enqueue(candidate, entry.includes); break; }
            }
        }
    }
    m_functionsModel->sort(0);
    if (m_pending.isEmpty()) {
        m_indexTimer->stop();
        m_status->setText(tr("%1 个文件 · 展开文件查看函数").arg(m_functionFiles.size()));
    }
}
void ProjectNavigator::setSourceBuffer(const QString &path, const QString &text, bool modified) {
    const auto key = pathKey(path);
    if (key == m_bufferPath && text == m_bufferText && modified == m_bufferModified) return;
    m_symbols.remove(m_bufferPath);
    m_symbols.remove(key);
    m_bufferPath = key;
    m_bufferText = text;
    m_bufferModified = modified;
    if (!m_project.filePath.isEmpty()) m_sourceTimer->start();
}
void ProjectNavigator::refreshSymbols() {
    if (m_project.filePath.isEmpty()) return;
    for (const auto &items : m_projectFiles) {
        for (auto *item : items) {
            const bool expanded = m_projectView->isExpanded(item->index());
            item->setData(false, LoadedRole);
            item->removeRows(0, item->rowCount());
            const auto path = item->data(PathRole).toString();
            const bool exists = QFileInfo(path).isFile();
            item->setIcon(style()->standardIcon(exists ? QStyle::SP_FileIcon : QStyle::SP_MessageBoxWarning));
            item->setToolTip(exists ? path : tr("文件不存在或路径无法解析: %1").arg(path));
            if (expanded) populate(item);
            else if (hasSymbols(path) && exists) item->appendRow(node(tr("展开以加载函数"), {}, Container));
        }
    }
    startIndexing();
}
