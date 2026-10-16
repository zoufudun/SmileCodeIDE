#ifndef SOURCENAVIGATOR_H
#define SOURCENAVIGATOR_H

#include <QString>

class QWidget;

class SourceNavigator {
public:
    // Diagnostic line and column are 1-based; zero column means unspecified.
    static bool open(QWidget *parent, QWidget *host, const QString &filePath,
                     int line, int column = 0);
};

#endif // SOURCENAVIGATOR_H
