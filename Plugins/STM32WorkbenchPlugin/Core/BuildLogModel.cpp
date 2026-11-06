#include "BuildLogModel.h"
#include <QDir>
#include <QRegularExpression>

void BuildLogModel::reset(const QString &projectDir, const QString &action)
{
    m_projectDir = QDir::fromNativeSeparators(projectDir);
    m_action = action;
    m_stdout.clear(); m_stderr.clear();
    m_concise.clear(); m_detailed.clear(); m_diagnostics.clear();
    m_errors = m_warnings = 0;
    m_finished = false;
    m_result = {};
}

QVector<BuildDiagnostic> BuildLogModel::append(const QString &chunk, bool fromStderr)
{
    if (m_finished) return {};
    const auto first = m_diagnostics.size();
    auto &buffer = fromStderr ? m_stderr : m_stdout;
    buffer += chunk;
    // Keep a trailing CR until the next chunk, since CRLF can straddle reads.
    qsizetype start = 0;
    for (qsizetype i = 0; i < buffer.size(); ++i) {
        if (buffer[i] != '\n' && buffer[i] != '\r') continue;
        if (buffer[i] == '\r' && i + 1 == buffer.size()) break;
        consumeLine(buffer.mid(start, i - start));
        if (buffer[i] == '\r' && i + 1 < buffer.size() && buffer[i + 1] == '\n') ++i;
        start = i + 1;
    }
    buffer.remove(0, start);
    return m_diagnostics.mid(first);
}

QVector<BuildDiagnostic> BuildLogModel::flush()
{
    const auto first = m_diagnostics.size();
    for (auto *buffer : {&m_stdout, &m_stderr}) {
        if (!buffer->isEmpty()) {
            if (buffer->endsWith('\r')) buffer->chop(1);
            consumeLine(*buffer);
            buffer->clear();
        }
    }
    return m_diagnostics.mid(first);
}

void BuildLogModel::addDiagnostic(const BuildDiagnostic &diagnostic)
{
    m_diagnostics.append(diagnostic);
    if (diagnostic.severity == DiagnosticSeverity::Warning) ++m_warnings;
    if (diagnostic.severity == DiagnosticSeverity::Error || diagnostic.severity == DiagnosticSeverity::Fatal) ++m_errors;
}

void BuildLogModel::consumeLine(QString text)
{
    static const QRegularExpression ansi(QStringLiteral("\\x1b(?:\\[[0-?]*[ -/]*[@-~]|\\][^\\x07]*(?:\\x07|\\x1b\\\\))"));
    text.remove(ansi);
    if (text.isEmpty()) return;
    BuildLogEntry entry; entry.text = text;
    const auto trimmed = text.trimmed();
    // Recipe announcements and command echoes are data, even when an argument
    // or a source filename happens to contain diagnostic-looking words.
    static const QRegularExpression stage(QStringLiteral("^\\[BUILD:(CC|CXX|AS|LD|HEX|BIN)\\]\\s*(.*)$"));
    auto match = stage.match(trimmed);
    if (match.hasMatch()) {
        m_detailed.append(entry);
        auto path = match.captured(2).trimmed();
        if (path.startsWith('"') && path.endsWith('"')) path = path.mid(1,path.size()-2);
        path.replace('\\','/'); path = path.section('/',-1);
        const auto kind = match.captured(1);
        entry.text = kind == "CC" || kind == "CXX" ? QString("compiling %1...").arg(path) : kind == "AS" ? QString("assembling %1...").arg(path) : kind == "LD" ? QString("linking %1...").arg(path) : QString("creating %1 %2...").arg(kind,path);
        m_concise.append(entry); return;
    }
    static const QRegularExpression command(QStringLiteral("^(?:\"[^\"]*(?:gcc|g\\+\\+|cc|c\\+\\+|as|ld|objcopy|size)(?:\\.exe)?\"|'[^']*(?:gcc|g\\+\\+|cc|c\\+\\+|as|ld|objcopy|size)(?:\\.exe)?'|[^\\s\"']*(?:gcc|g\\+\\+|cc|c\\+\\+|as|ld|objcopy|size)(?:\\.exe)?)(?:\\s|$)"), QRegularExpression::CaseInsensitiveOption);
    if (command.match(trimmed).hasMatch()) {
        m_detailed.append(entry);
        return;
    }
    static const QRegularExpression excerpt(QStringLiteral("^\\s*(?:\\d+\\s*\\||\\||\\^|~)"));
    if (excerpt.match(text).hasMatch()) {
        m_detailed.append(entry);
        return;
    }
    static const QRegularExpression located(QStringLiteral("^(.+?):(\\d+)(?::(\\d+))?:\\s*(?:(fatal error|error|warning|note):\\s*(.*)|(undefined reference to.*|multiple definition of.*))$"));
    match = located.match(trimmed);
    if (match.hasMatch()) {
        BuildDiagnostic d;
        d.filePath = QDir::fromNativeSeparators(match.captured(1));
        if ((d.filePath.startsWith('"') && d.filePath.endsWith('"')) ||
            (d.filePath.startsWith('\'') && d.filePath.endsWith('\'')))
            d.filePath = d.filePath.mid(1, d.filePath.size() - 2);
        static const QRegularExpression drive(QStringLiteral("^[A-Za-z]:[/\\\\]"));
        if (QDir::isRelativePath(d.filePath) && !drive.match(d.filePath).hasMatch())
            d.filePath = QDir(m_projectDir).absoluteFilePath(d.filePath);
        d.filePath = QDir::cleanPath(d.filePath);
        d.line = match.captured(2).toInt(); d.column = match.captured(3).toInt();
        const auto kind = match.captured(4);
        d.severity = kind == "warning" ? DiagnosticSeverity::Warning : kind == "note" ? DiagnosticSeverity::Info : kind == "fatal error" ? DiagnosticSeverity::Fatal : DiagnosticSeverity::Error;
        d.message = match.captured(6).isEmpty() ? match.captured(5) : match.captured(6);
        addDiagnostic(d);
        entry.severity = d.severity; entry.filePath = d.filePath; entry.line = d.line; entry.column = d.column;
        m_detailed.append(entry);
        if (d.severity != DiagnosticSeverity::Info) m_concise.append(entry);
        return;
    }
    static const QRegularExpression summary(QStringLiteral("(?:collect2(?:\\.exe)?:.*(?:error|exit status)|(?:^|[/\\\\])(?:mingw32-)?make(?:\\.exe)?(?:\\[\\d+\\])?:.*\\*\\*\\*)"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression failure(QStringLiteral("^(?:(?:(?:\"[^\"]+\"|[^\"'()]+?):\\s+)?(?:fatal error:|error:|warning:|undefined reference to|multiple definition of|region .*overflowed by|.*will not fit in region|cannot find |cannot open |No rule to make target|No such file or directory|unrecognized (?:command.line )?option)|错误[:：]|无法启动|(?:Makefile\\s*)?生成失败|已手动终止)"), QRegularExpression::CaseInsensitiveOption);
    const bool followOn = summary.match(trimmed).hasMatch();
    if (failure.match(trimmed).hasMatch() || followOn) {
        const bool warning = trimmed.contains(QRegularExpression(QStringLiteral("\\bwarning:"), QRegularExpression::CaseInsensitiveOption));
        entry.severity = warning ? DiagnosticSeverity::Warning : trimmed.contains("fatal error:") ? DiagnosticSeverity::Fatal : DiagnosticSeverity::Error;
        // Retain summary lines in both views, but count only underlying errors.
        if (!followOn || m_errors == 0) {
            BuildDiagnostic d; d.severity = entry.severity; d.message = trimmed;
            addDiagnostic(d);
        }
        m_detailed.append(entry); m_concise.append(entry); return;
    }
    m_detailed.append(entry);
    if (trimmed.contains("Nothing to be done", Qt::CaseInsensitive) || trimmed.contains("is up to date", Qt::CaseInsensitive)) {
        entry.text = QStringLiteral("已是最新，无需重新编译"); m_concise.append(entry); return;
    }
    static const QRegularExpression size(QStringLiteral("^\\s*(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+\\d+\\s+[0-9a-fA-F]+\\s+.+$"));
    match = size.match(text);
    if (match.hasMatch()) {
        const auto t=match.captured(1).toULongLong(), d=match.captured(2).toULongLong(), b=match.captured(3).toULongLong();
        entry.text = QString("Program Size: Code=%1 RO-data=0 RW-data=%2 ZI-data=%3 (text: %1, data: %2, bss: %3; Flash: %4 bytes, RAM: %5 bytes)").arg(t).arg(d).arg(b).arg(t+d).arg(d+b);
        m_concise.append(entry);
    }
}

BuildLogResult BuildLogModel::finish(bool success, int exitCode, int durationMs)
{
    if (m_finished) return m_result;
    flush();
    success = success && exitCode == 0 && m_errors == 0;
    if (!success && m_errors == 0) {
        BuildDiagnostic d; d.message = QStringLiteral("构建进程失败或已取消 (exit code %1)").arg(exitCode);
        addDiagnostic(d);
        BuildLogEntry e; e.text=d.message; e.severity=d.severity;
        m_concise.append(e); m_detailed.append(e);
    }
    const bool clean = m_action.compare("clean",Qt::CaseInsensitive)==0 || m_action.contains(QStringLiteral("清理"));
    const auto status = clean ? (success ? QStringLiteral("清理完成") : QStringLiteral("清理失败")) : (success ? QStringLiteral("构建完成") : QStringLiteral("构建失败"));
    m_result = {success, m_errors, m_warnings, QStringLiteral("%1 · %2 Error(s), %3 Warning(s) (错误 %2，警告 %3，耗时 %4 秒)").arg(status).arg(m_errors).arg(m_warnings).arg(qMax(0,durationMs)/1000.0,0,'f',2)};
    BuildLogEntry e; e.text=m_result.summary; e.success=success; e.severity=success ? DiagnosticSeverity::Info : DiagnosticSeverity::Error;
    m_concise.append(e); m_detailed.append(e); m_finished=true;
    return m_result;
}

bool BuildLogModel::matchesFilter(const BuildLogEntry &entry, BuildLogFilter filter)
{
    switch (filter) {
    case BuildLogFilter::ErrorsOnly:
        return entry.severity == DiagnosticSeverity::Error || entry.severity == DiagnosticSeverity::Fatal;
    case BuildLogFilter::WarningsOnly:
        return entry.severity == DiagnosticSeverity::Warning;
    case BuildLogFilter::ErrorsAndWarnings:
        return entry.severity == DiagnosticSeverity::Warning ||
               entry.severity == DiagnosticSeverity::Error ||
               entry.severity == DiagnosticSeverity::Fatal;
    case BuildLogFilter::All:
    default:
        return true;
    }
}

QVector<BuildLogEntry> BuildLogModel::filteredEntries(bool detailed, BuildLogFilter filter) const
{
    const auto &source = detailed ? m_detailed : m_concise;
    if (filter == BuildLogFilter::All) {
        return source;
    }
    QVector<BuildLogEntry> result;
    result.reserve(source.size());
    for (const BuildLogEntry &entry : source) {
        if (matchesFilter(entry, filter)) {
            result.append(entry);
        }
    }
    return result;
}

QColor BuildLogModel::color(DiagnosticSeverity severity)
{
    switch (severity) {
    case DiagnosticSeverity::Warning: return QColor("#f9e2af");
    case DiagnosticSeverity::Error:
    case DiagnosticSeverity::Fatal: return QColor("#f38ba8");
    default: return QColor("#cdd6f4");
    }
}
