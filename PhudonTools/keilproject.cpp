#include "keilproject.h"
#include <QDir>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

namespace {
QString absolutePath(const QDir &base, QString path) {
    path = path.trimmed();
    path.replace('\\', '/');
    return path.isEmpty() ? QString() : QDir::cleanPath(base.absoluteFilePath(path));
}

void appendPaths(QStringList &paths, const QStringList &extra) {
    for (const auto &path : extra)
        if (!paths.contains(path)) paths.append(path);
}

void readOptions(QXmlStreamReader &xml, const QDir &base, QStringList &paths, QString *device = nullptr) {
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("IncludePath")) {
            for (const auto &path : xml.readElementText().split(';', Qt::SkipEmptyParts)) {
                const auto resolved = absolutePath(base, path);
                if (!resolved.isEmpty() && !paths.contains(resolved)) paths.append(resolved);
            }
        } else if (xml.name() == QLatin1String("Device")) {
            const auto value = xml.readElementText().trimmed();
            if (device) *device = value;
        } else {
            readOptions(xml, base, paths, device);
        }
    }
}

KeilProjectFile readFile(QXmlStreamReader &xml, const QDir &base) {
    KeilProjectFile file;
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("FileName")) file.name = xml.readElementText();
        else if (xml.name() == QLatin1String("FilePath")) file.path = absolutePath(base, xml.readElementText());
        else if (xml.name() == QLatin1String("FileOption")) readOptions(xml, base, file.includePaths);
        else xml.skipCurrentElement();
    }
    if (file.name.isEmpty()) file.name = QFileInfo(file.path).fileName();
    return file;
}

KeilProjectGroup readGroup(QXmlStreamReader &xml, const QDir &base) {
    KeilProjectGroup group;
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("GroupName")) group.name = xml.readElementText();
        else if (xml.name() == QLatin1String("GroupOption")) readOptions(xml, base, group.includePaths);
        else if (xml.name() == QLatin1String("Files")) {
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("File")) group.files.append(readFile(xml, base));
                else xml.skipCurrentElement();
            }
        } else xml.skipCurrentElement();
    }
    return group;
}

KeilProjectTarget readTarget(QXmlStreamReader &xml, const QDir &base) {
    KeilProjectTarget target;
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("TargetName")) target.name = xml.readElementText();
        else if (xml.name() == QLatin1String("TargetOption")) readOptions(xml, base, target.includePaths, &target.device);
        else if (xml.name() == QLatin1String("Groups")) {
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("Group")) target.groups.append(readGroup(xml, base));
                else xml.skipCurrentElement();
            }
        } else xml.skipCurrentElement();
    }
    for (auto &group : target.groups) {
        for (auto &file : group.files) {
            appendPaths(file.includePaths, group.includePaths);
            appendPaths(file.includePaths, target.includePaths);
        }
    }
    return target;
}
}

bool KeilProject::load(const QString &path, QString *error) {
    if (error) error->clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    const QByteArray contents = file.readAll();
    QXmlStreamReader xml(contents);
    QList<KeilProjectTarget> parsed;
    const QDir base(QFileInfo(path).absolutePath());
    if (xml.readNextStartElement() && xml.name() == QLatin1String("Project")) {
        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("Targets")) {
                while (xml.readNextStartElement()) {
                    if (xml.name() == QLatin1String("Target")) parsed.append(readTarget(xml, base));
                    else xml.skipCurrentElement();
                }
            } else xml.skipCurrentElement();
        }
        while (!xml.atEnd()) xml.readNext();
    } else xml.raiseError(QStringLiteral("不是有效的 Keil Project 文档"));
    if (xml.hasError() || parsed.isEmpty()) {
        if (error) *error = xml.hasError()
            ? QStringLiteral("XML 第 %1 行: %2").arg(xml.lineNumber()).arg(xml.errorString())
            : QStringLiteral("工程中没有 Target");
        return false;
    }
    filePath = QFileInfo(path).absoluteFilePath();
    contentHash = QCryptographicHash::hash(contents, QCryptographicHash::Sha256);
    targets = parsed;
    return true;
}
