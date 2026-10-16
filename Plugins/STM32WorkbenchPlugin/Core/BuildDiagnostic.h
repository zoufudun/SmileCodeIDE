#ifndef BUILDDIAGNOSTIC_H
#define BUILDDIAGNOSTIC_H
#include <QString>
enum class DiagnosticSeverity { Info, Warning, Error, Fatal };
struct BuildDiagnostic {
    QString filePath;
    int line = 0;
    int column = 0;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    QString message;
};
#endif
