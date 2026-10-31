#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include "projectsearchengine.h"

namespace {
QRegularExpression expression(const ProjectSearchRequest &request) {
    QString pattern = request.options.regularExpression ? request.query
        : QRegularExpression::escape(request.query);
    if (request.options.wholeWord)
        pattern = "(?<![\\p{L}\\p{N}\\p{M}_])(?:" + pattern + ")(?![\\p{L}\\p{N}\\p{M}_])";
    auto flags = QRegularExpression::UseUnicodePropertiesOption;
    QRegularExpression::PatternOptions options(flags);
    if (!request.options.caseSensitive)
        options |= QRegularExpression::CaseInsensitiveOption;
    return QRegularExpression(pattern, options);
}
bool searchable(const QFileInfo &file) {
    static const QSet<QString> suffixes = {"c", "cc", "cpp", "cxx", "h", "hh", "hpp", "hxx",
        "s", "asm", "inc", "ld", "lds", "icf", "sct", "pro", "pri", "prf", "cmake",
        "txt", "json", "xml", "yaml", "yml", "ini", "cfg", "mk", "make", "qrc", "ui"};
    return suffixes.contains(file.suffix().toLower())
        || file.fileName().compare("Makefile", Qt::CaseInsensitive) == 0;
}
}

QString ProjectSearchEngine::validate(const ProjectSearchRequest &request) {
    if (request.query.isEmpty())
        return QStringLiteral("请输入查找内容。");
    if (request.root.isEmpty() || !QFileInfo(request.root).isDir())
        return QStringLiteral("请先打开有效工程。");
    const auto regex = expression(request);
    if (!regex.isValid())
        return QStringLiteral("正则表达式无效：%1").arg(regex.errorString());
    if (request.options.maxFileBytes <= 0 || request.options.maxResults <= 0)
        return QStringLiteral("查找限制必须大于零。");
    return {};
}

ProjectSearchResult ProjectSearchEngine::search(const ProjectSearchRequest &request,
    const std::shared_ptr<std::atomic_bool> &cancel) {
    ProjectSearchResult result;
    result.error = validate(request);
    if (!result.error.isEmpty())
        return result;
    const auto regex = expression(request);
    const QDir root(QFileInfo(request.root).canonicalFilePath());
    const QString bufferPath = QFileInfo(request.bufferPath).absoluteFilePath();
    auto stopped = [&]() {
        if (cancel && cancel->load(std::memory_order_relaxed))
            result.cancelled = true;
        return result.cancelled || result.resultLimitReached;
    };
    auto searchFile = [&](const QFileInfo &info) {
        if (stopped() || !searchable(info))
            return;
        QString content;
        if (info.absoluteFilePath() == bufferPath) {
            if (request.bufferText.toUtf8().size() > request.options.maxFileBytes) {
                ++result.skippedLargeFiles;
                return;
            }
            content = request.bufferText;
        } else {
            QFile file(info.absoluteFilePath());
            if (info.size() > request.options.maxFileBytes) {
                ++result.skippedLargeFiles;
                return;
            }
            if (!file.open(QIODevice::ReadOnly)) {
                ++result.skippedUnreadableFiles;
                return;
            }
            const QByteArray data = file.read(request.options.maxFileBytes + 1);
            if (data.size() > request.options.maxFileBytes) {
                ++result.skippedLargeFiles;
                return;
            }
            if (data.contains('\0'))
                return;
            content = QString::fromUtf8(data);
        }
        content.replace("\r\n", "\n");
        ++result.searchedFiles;
        auto matches = regex.globalMatch(content);
        qsizetype scanned = 0;
        int line = 1;
        qsizetype lineStart = 0;
        while (!stopped() && matches.hasNext()) {
            const auto match = matches.next();
            const qsizetype offset = match.capturedStart();
            for (; scanned < offset; ++scanned) {
                if (content[scanned] == '\n') {
                    ++line;
                    lineStart = scanned + 1;
                    if (stopped())
                        return;
                }
            }
            const auto end = content.indexOf('\n', lineStart);
            result.matches.append({info.absoluteFilePath(), root.relativeFilePath(info.absoluteFilePath()),
                line, int(offset - lineStart + 1), content.mid(lineStart,
                    end < 0 ? -1 : end - lineStart).left(400)});
            if (result.matches.size() >= request.options.maxResults)
                result.resultLimitReached = true;
        }
    };
    static const QSet<QString> excluded = {".git", ".smilecode", "build", "debug", "release",
        "node_modules", "cache", ".cache", ".svn", ".hg", "__pycache__"};
    QStringList pending{root.absolutePath()};
    while (!pending.isEmpty() && !stopped()) {
        const QDir dir(pending.takeLast());
        const auto entries = dir.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot,
            QDir::Name | QDir::DirsFirst);
        for (const auto &entry : entries) {
            if (stopped())
                break;
            // Skipping all links also prevents cycles and Windows directory junction escapes.
            if (entry.isSymLink() || entry.isJunction())
                continue;
            if (entry.isDir()) {
                const QString name = entry.fileName().toLower();
                if (!excluded.contains(name) && !name.startsWith("build-")
                    && !name.startsWith("cmake-build-"))
                    pending.append(entry.absoluteFilePath());
            } else if (entry.isFile()) {
                searchFile(entry);
            }
        }
    }
    return result;
}
