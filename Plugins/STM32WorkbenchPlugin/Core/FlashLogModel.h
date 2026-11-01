#ifndef FLASHLOGMODEL_H
#define FLASHLOGMODEL_H

#include <QColor>
#include <QString>
#include <QVector>

enum class FlashLogSeverity { Info, Success, Warning, Error };

struct FlashLogEntry {
    QString text;
    FlashLogSeverity severity = FlashLogSeverity::Info;
};

class FlashLogModel
{
public:
    void reset(bool verify = true, bool resetAndRun = true);
    // Return only newly visible concise entries. The stream flag selects a line
    // buffer; severity is always determined by the completed line's contents.
    QVector<FlashLogEntry> append(const QString &chunk, bool fromStderr);
    QVector<FlashLogEntry> flush();
    // Flushes final partial lines and retains one summary in both views. A real
    // error in the log prevents a contradictory successful presentation.
    FlashLogEntry finish(bool success, const QString &message);
    const QVector<FlashLogEntry> &detailedEntries() const;
    const QVector<FlashLogEntry> &conciseEntries() const;
    static FlashLogSeverity classify(const QString &line);
    static QColor color(FlashLogSeverity severity);

private:
    struct StreamBuffer {
        QString pending;
        quint64 firstSequence = 0;
        bool skipLf = false;
    };

    void consumeLine(const QString &line, QVector<FlashLogEntry> &added);

    StreamBuffer m_stdout;
    StreamBuffer m_stderr;
    QVector<FlashLogEntry> m_detailed;
    QVector<FlashLogEntry> m_concise;
    quint64 m_nextSequence = 1;
    unsigned int m_emittedStages = 0;
    bool m_verify = true;
    bool m_resetAndRun = true;
    bool m_hasError = false;
    bool m_finished = false;
    FlashLogEntry m_summary;
};

#endif // FLASHLOGMODEL_H
