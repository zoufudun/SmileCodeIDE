#include "FlashLogModel.h"

#include <QRegularExpression>
#include <array>

namespace {
enum Stage : unsigned int {
    Connected = 1,
    ProgrammingStarted = 2,
    ProgrammingFinished = 4,
    Verified = 8,
    Resetting = 16
};

struct StageRule {
    Stage stage;
    QRegularExpression pattern;
    QString text;
    FlashLogSeverity severity;
};

const std::array<StageRule, 5> &stageRules()
{
    static const std::array<StageRule, 5> rules{{
        {Connected, QRegularExpression(
             "\\b(?:Examination succeed|Cortex-M\\d+[^\\n]*(?:processor detected|identified))\\b",
             QRegularExpression::CaseInsensitiveOption),
         QStringLiteral("连接成功"), FlashLogSeverity::Success},
        {ProgrammingStarted, QRegularExpression(
             "\\b(?:Programming Started|Download in Progress|Downloading file)\\b",
             QRegularExpression::CaseInsensitiveOption),
         QStringLiteral("开始下载…"), FlashLogSeverity::Info},
        {ProgrammingFinished, QRegularExpression(
             "\\b(?:Programming Finished|File download complete|Starting verification of write complete|Flash written and verified)\\b",
             QRegularExpression::CaseInsensitiveOption),
         QStringLiteral("编程完成"), FlashLogSeverity::Success},
        {Verified, QRegularExpression(
             "\\b(?:Verified OK|Download verified successfully|Flash written and verified)\\b",
             QRegularExpression::CaseInsensitiveOption),
         QStringLiteral("校验通过"), FlashLogSeverity::Success},
        {Resetting, QRegularExpression("\\bResetting Target\\b",
             QRegularExpression::CaseInsensitiveOption),
         QStringLiteral("正在复位…"), FlashLogSeverity::Info}
    }};
    return rules;
}
}

void FlashLogModel::reset(bool verify, bool resetAndRun)
{
    m_stdout = {};
    m_stderr = {};
    m_detailed.clear();
    m_concise.clear();
    m_nextSequence = 1;
    m_emittedStages = 0;
    m_verify = verify;
    m_resetAndRun = resetAndRun;
    m_hasError = false;
    m_finished = false;
    m_summary = {};
}

QVector<FlashLogEntry> FlashLogModel::append(const QString &chunk, bool fromStderr)
{
    QVector<FlashLogEntry> added;
    if (m_finished) return added;

    StreamBuffer &stream = fromStderr ? m_stderr : m_stdout;
    for (const QChar character : chunk) {
        // A CR is also a line boundary for tools that redraw progress. When its
        // matching LF arrives in a later chunk, do not create an extra line.
        if (stream.skipLf) {
            stream.skipLf = false;
            if (character == '\n') continue;
        }
        if (!stream.firstSequence) stream.firstSequence = m_nextSequence++;
        if (character == '\n' || character == '\r') {
            consumeLine(stream.pending, added);
            stream.pending.clear();
            stream.firstSequence = 0;
            stream.skipLf = character == '\r';
        } else {
            stream.pending.append(character);
        }
    }
    return added;
}

QVector<FlashLogEntry> FlashLogModel::flush()
{
    QVector<FlashLogEntry> added;
    if (m_finished) return added;
    std::array<StreamBuffer *, 2> streams{{&m_stdout, &m_stderr}};
    if (m_stderr.firstSequence && (!m_stdout.firstSequence ||
                                  m_stderr.firstSequence < m_stdout.firstSequence)) {
        std::swap(streams[0], streams[1]);
    }
    for (StreamBuffer *stream : streams) {
        if (!stream->pending.isEmpty()) consumeLine(stream->pending, added);
        *stream = {};
    }
    return added;
}

void FlashLogModel::consumeLine(const QString &line, QVector<FlashLogEntry> &added)
{
    const FlashLogEntry entry{line, classify(line)};
    m_detailed.append(entry);
    if (entry.severity == FlashLogSeverity::Error || entry.severity == FlashLogSeverity::Warning) {
        m_hasError = m_hasError || entry.severity == FlashLogSeverity::Error;
        m_concise.append(entry);
        added.append(entry);
        return;
    }

    for (const auto &rule : stageRules()) {
        if ((m_emittedStages & rule.stage) || (rule.stage == Verified && !m_verify) ||
            (rule.stage == Resetting && !m_resetAndRun) || !rule.pattern.match(line).hasMatch()) {
            continue;
        }
        m_emittedStages |= rule.stage;
        const FlashLogEntry stage{rule.text, rule.severity};
        m_concise.append(stage);
        added.append(stage);
    }
}

FlashLogEntry FlashLogModel::finish(bool success, const QString &message)
{
    if (m_finished) return m_summary;
    flush();
    m_summary = {message, success ? FlashLogSeverity::Success : FlashLogSeverity::Error};
    if (success && m_hasError) {
        m_summary = {QStringLiteral("下载未确认成功：进程正常退出，但日志报告错误，请查看详细日志。"),
                     FlashLogSeverity::Error};
    }
    m_detailed.append(m_summary);
    m_concise.append(m_summary);
    m_finished = true;
    return m_summary;
}

const QVector<FlashLogEntry> &FlashLogModel::detailedEntries() const { return m_detailed; }
const QVector<FlashLogEntry> &FlashLogModel::conciseEntries() const { return m_concise; }

FlashLogSeverity FlashLogModel::classify(const QString &line)
{
    // Do not treat words in filenames or reports such as "0 errors" as failures.
    static const QRegularExpression noErrors(
        "\\b(?:0\\s+errors?|no\\s+errors?|error(?:s|\\(s\\))?\\s*(?:count\\s*)?[:=]\\s*0)\\b",
        QRegularExpression::CaseInsensitiveOption);
    QString diagnostic = line.trimmed();
    diagnostic.remove(noErrors);

    static const QRegularExpression explicitError(
        "(?:^|\\s|\\])(?:error|fatal)(?:\\s*[:!]|\\s*$)|\\[(?:error|fatal)\\]|"
        "(?:^|\\s)(?:error|fatal)\\s+[\\w./\\\\-]+:\\s*|"
        "\\berror(?:s|\\(s\\))?\\s*(?:count\\s*)?[:=]\\s*[1-9]\\d*\\b|"
        "\\b[1-9]\\d*\\s+errors?\\b|错误\\s*[:：]|已手动中止|"
        "(?:烧录|下载|编程|校验|连接|擦除|启动|写入).*失败|无法(?:启动|连接|打开|创建|写入)",
        QRegularExpression::CaseInsensitiveOption);
    if (explicitError.match(diagnostic).hasMatch()) return FlashLogSeverity::Error;

    static const QRegularExpression warning(
        "(?:^|\\s|\\])warn(?:ing)?\\s*[:!]|\\[warn(?:ing)?\\]|^warning\\b|"
        "(?:^|\\s)warn(?:ing)?\\s+[\\w./\\\\-]+:\\s*|警告\\s*[:：]",
        QRegularExpression::CaseInsensitiveOption);
    if (warning.match(diagnostic).hasMatch()) return FlashLogSeverity::Warning;

    static const QRegularExpression failure(
        "\\b(?:failed to|(?:programming|verification|verify|write|download|erase|connection|initialization)\\s+failed|"
        "(?:unable|cannot|can not|could not)\\s+connect|"
        "no\\s+(?:st-?link|j-?link|cmsis-dap)(?:\\s+(?:device|probe))?\\s+(?:detected|found)|"
        "couldn't find any st-link|target not examined yet)\\b",
        QRegularExpression::CaseInsensitiveOption);
    if (failure.match(diagnostic).hasMatch()) return FlashLogSeverity::Error;

    for (const auto &rule : stageRules()) {
        if (rule.severity == FlashLogSeverity::Success && rule.pattern.match(line).hasMatch())
            return FlashLogSeverity::Success;
    }
    return FlashLogSeverity::Info;
}

QColor FlashLogModel::color(FlashLogSeverity severity)
{
    switch (severity) {
    case FlashLogSeverity::Success: return QColor("#a6e3a1");
    case FlashLogSeverity::Warning: return QColor("#f9e2af");
    case FlashLogSeverity::Error: return QColor("#f38ba8");
    default: return QColor("#cdd6f4");
    }
}
