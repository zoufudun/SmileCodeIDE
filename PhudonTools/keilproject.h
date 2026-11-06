#ifndef KEILPROJECT_H
#define KEILPROJECT_H

#include <QList>
#include <QStringList>
#include <QByteArray>

struct KeilProjectFile {
    QString name;
    QString path;
    QStringList includePaths;
};

struct KeilProjectGroup {
    QString name;
    QList<KeilProjectFile> files;
    QStringList includePaths;
};

struct KeilProjectTarget {
    QString name;
    QString device;
    QStringList includePaths;
    QList<KeilProjectGroup> groups;
};

class KeilProject {
public:
    bool load(const QString &path, QString *error = nullptr);
    QString filePath;
    QByteArray contentHash;
    QList<KeilProjectTarget> targets;
};

#endif
