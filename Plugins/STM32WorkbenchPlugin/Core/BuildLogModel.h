#ifndef BUILDLOGMODEL_H
#define BUILDLOGMODEL_H
#include "BuildDiagnostic.h"
#include <QColor>
#include <QVector>
enum class BuildLogFilter {
    All = 0,
    ErrorsOnly = 1,
    WarningsOnly = 2,
    ErrorsAndWarnings = 3
};

struct BuildLogEntry {
    QString text;
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    QString filePath;
    int line = 0;
    int column = 0;
    bool success = false;
};
struct BuildLogResult {
    bool success = false;
    int errorCount = 0;
    int warningCount = 0;
    QString summary;
};
class BuildLogModel {
public:
    void reset(const QString &projectDir, const QString &action);
    QVector<BuildDiagnostic> append(const QString &chunk, bool fromStderr);
    QVector<BuildDiagnostic> flush();
    BuildLogResult finish(bool success, int exitCode, int durationMs);
    const QVector<BuildLogEntry> &conciseEntries() const { return m_concise; }
    const QVector<BuildLogEntry> &detailedEntries() const { return m_detailed; }
    QVector<BuildLogEntry> filteredEntries(bool detailed, BuildLogFilter filter) const;
    static bool matchesFilter(const BuildLogEntry &entry, BuildLogFilter filter);
    const QVector<BuildDiagnostic> &diagnostics() const { return m_diagnostics; }
    int errorCount() const { return m_errors; }
    int warningCount() const { return m_warnings; }
    static QColor color(DiagnosticSeverity severity);
private:
    void consumeLine(QString text);
    void addDiagnostic(const BuildDiagnostic &diagnostic);
    QString m_projectDir, m_action, m_stdout, m_stderr;
    QVector<BuildLogEntry> m_concise, m_detailed;
    QVector<BuildDiagnostic> m_diagnostics;
    int m_errors = 0, m_warnings = 0;
    bool m_finished = false;
    BuildLogResult m_result;
};
#endif
