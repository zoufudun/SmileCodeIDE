#include <QCheckBox>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QtConcurrent>
#include "projectsearchpanel.h"

ProjectSearchPanel::ProjectSearchPanel(QWidget *parent) : QWidget(parent) {
    setObjectName("projectSearchPanel");
    auto *layout = new QVBoxLayout(this);
    auto *controls = new QHBoxLayout;
    m_query = new QLineEdit(this);
    m_query->setObjectName("projectSearchQuery");
    m_query->setPlaceholderText(tr("在工程中查找…"));
    m_query->setClearButtonEnabled(true);
    m_regex = new QCheckBox(tr("正则"), this);
    m_regex->setObjectName("projectSearchRegex");
    m_case = new QCheckBox(tr("区分大小写"), this);
    m_case->setObjectName("projectSearchCaseSensitive");
    m_word = new QCheckBox(tr("全词"), this);
    m_word->setObjectName("projectSearchWholeWord");
    auto *search = new QPushButton(tr("查找"), this);
    search->setObjectName("projectSearchStart");
    m_cancelButton = new QPushButton(tr("取消"), this);
    m_cancelButton->setObjectName("projectSearchCancel");
    m_cancelButton->setEnabled(false);
    controls->addWidget(m_query, 1);
    controls->addWidget(m_regex);
    controls->addWidget(m_case);
    controls->addWidget(m_word);
    controls->addWidget(search);
    controls->addWidget(m_cancelButton);
    layout->addLayout(controls);
    m_status = new QLabel(tr("请输入查找内容；仅扫描工程文本源码和配置文件。"), this);
    m_status->setObjectName("projectSearchStatus");
    m_status->setWordWrap(true);
    layout->addWidget(m_status);
    m_results = new QTreeWidget(this);
    m_results->setObjectName("projectSearchResults");
    m_results->setColumnCount(3);
    m_results->setHeaderLabels({tr("文件"), tr("行:列"), tr("内容")});
    m_results->setRootIsDecorated(false);
    m_results->setAlternatingRowColors(true);
    m_results->setUniformRowHeights(true);
    m_results->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_results->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_results->header()->setStretchLastSection(true);
    layout->addWidget(m_results, 1);
    connect(search, &QPushButton::clicked, this, &ProjectSearchPanel::runSearch);
    connect(m_query, &QLineEdit::returnPressed, this, &ProjectSearchPanel::runSearch);
    connect(m_cancelButton, &QPushButton::clicked, this, &ProjectSearchPanel::cancelSearch);
    // itemActivated handles both a double click and keyboard Enter.
    connect(m_results, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item) {
        emit sourceLocationActivated(item->data(0, Qt::UserRole).toString(),
            item->data(1, Qt::UserRole).toInt(), item->data(2, Qt::UserRole).toInt());
    });
}

ProjectSearchPanel::~ProjectSearchPanel() {
    if (m_cancel)
        m_cancel->store(true, std::memory_order_relaxed);
}

void ProjectSearchPanel::setProjectRoot(const QString &root) {
    if (m_request.root == root)
        return;
    cancelSearch();
    ++m_generation;
    m_searching = false;
    m_request.root = root;
    m_request.bufferPath.clear();
    m_request.bufferText.clear();
    m_results->clear();
    m_status->setText(root.isEmpty() ? tr("请先打开工程。") : tr("工程：%1").arg(root));
}

void ProjectSearchPanel::setCurrentBuffer(const QString &path, const QString &text) {
    m_request.bufferPath = path;
    m_request.bufferText = text;
}

QString ProjectSearchPanel::searchText() const { return m_query->text(); }
bool ProjectSearchPanel::isSearching() const { return m_searching; }

void ProjectSearchPanel::startSearch(const QString &query, const ProjectSearchOptions &options) {
    m_query->setText(query);
    m_regex->setChecked(options.regularExpression);
    m_case->setChecked(options.caseSensitive);
    m_word->setChecked(options.wholeWord);
    m_request.options = options;
    runSearch();
}

void ProjectSearchPanel::findReferences(const QString &symbol) {
    ProjectSearchOptions options;
    options.wholeWord = true;
    options.caseSensitive = true;
    startSearch(symbol, options);
}

void ProjectSearchPanel::focusSearch() {
    m_query->setFocus(Qt::ShortcutFocusReason);
    m_query->selectAll();
}

void ProjectSearchPanel::cancelSearch() {
    if (m_cancel && m_searching) {
        m_cancel->store(true, std::memory_order_relaxed);
        m_status->setText(tr("正在取消查找…"));
    }
    m_cancelButton->setEnabled(false);
}

void ProjectSearchPanel::runSearch() {
    m_request.query = m_query->text();
    m_request.options.regularExpression = m_regex->isChecked();
    m_request.options.caseSensitive = m_case->isChecked();
    m_request.options.wholeWord = m_word->isChecked();
    const QString error = ProjectSearchEngine::validate(m_request);
    if (!error.isEmpty()) {
        m_status->setText(error + tr("（保留已有结果）"));
        return;
    }
    if (m_cancel)
        m_cancel->store(true, std::memory_order_relaxed);
    const quint64 generation = ++m_generation;
    const auto token = std::make_shared<std::atomic_bool>(false);
    m_cancel = token;
    const auto request = m_request;
    auto *watcher = new QFutureWatcher<ProjectSearchResult>(this);
    m_searching = true;
    m_cancelButton->setEnabled(true);
    m_status->setText(tr("正在查找“%1”… 单文件上限 %2 MiB，最多 %3 条结果；跳过构建目录及符号链接。")
        .arg(request.query).arg(double(request.options.maxFileBytes) / (1024 * 1024), 0, 'g', 3)
        .arg(request.options.maxResults));
    connect(watcher, &QFutureWatcher<ProjectSearchResult>::finished, this,
        [this, watcher, generation, request]() {
            const auto result = watcher->result();
            watcher->deleteLater();
            if (generation != m_generation)
                return;
            m_searching = false;
            m_cancelButton->setEnabled(false);
            m_results->clear();
            for (const auto &match : result.matches) {
                auto *item = new QTreeWidgetItem(m_results,
                    {match.relativeFile, QString("%1:%2").arg(match.line).arg(match.column), match.preview});
                item->setData(0, Qt::UserRole, match.file);
                item->setData(1, Qt::UserRole, match.line);
                item->setData(2, Qt::UserRole, match.column);
                item->setToolTip(0, match.file);
            }
            QString status = tr("%1：%2 条结果，已扫描 %3 个文件。")
                .arg(result.cancelled ? tr("已取消") : tr("查找完成"))
                .arg(result.matches.size()).arg(result.searchedFiles);
            if (!result.error.isEmpty())
                status = result.error;
            if (result.resultLimitReached)
                status += tr(" 已达到 %1 条结果上限，请缩小查找范围。").arg(request.options.maxResults);
            status += tr(" 单文件上限 %1 MiB；跳过 %2 个大文件、%3 个无法读取的文件。")
                .arg(double(request.options.maxFileBytes) / (1024 * 1024), 0, 'g', 3)
                .arg(result.skippedLargeFiles).arg(result.skippedUnreadableFiles);
            m_status->setText(status);
            emit searchFinished();
        });
    // No QWidget/QObject is captured by the worker. Destruction only requests cancellation.
    watcher->setFuture(QtConcurrent::run([request, token]() {
        return ProjectSearchEngine::search(request, token);
    }));
}
