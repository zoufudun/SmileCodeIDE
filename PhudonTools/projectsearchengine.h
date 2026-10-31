#ifndef PROJECTSEARCHENGINE_H
#define PROJECTSEARCHENGINE_H

#include <QString>
#include <QVector>
#include <atomic>
#include <memory>

struct ProjectSearchOptions {
    bool regularExpression = false;
    bool caseSensitive = false;
    bool wholeWord = false;
    qint64 maxFileBytes = 2 * 1024 * 1024;
    int maxResults = 2000;
};

struct ProjectSearchRequest {
    QString root;
    QString query;
    ProjectSearchOptions options;
    QString bufferPath;
    QString bufferText;
};

struct ProjectSearchMatch {
    QString file;
    QString relativeFile;
    int line = 1;
    int column = 1;
    QString preview;
};

struct ProjectSearchResult {
    QVector<ProjectSearchMatch> matches;
    QString error;
    bool cancelled = false;
    bool resultLimitReached = false;
    int skippedLargeFiles = 0;
    int skippedUnreadableFiles = 0;
    int searchedFiles = 0;
};

class ProjectSearchEngine {
public:
    static QString validate(const ProjectSearchRequest &request);
    static ProjectSearchResult search(const ProjectSearchRequest &request,
        const std::shared_ptr<std::atomic_bool> &cancel = {});
};

#endif
