#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QLabel>
#include <QTreeWidget>
#include <QtTest>
#include "projectsearchengine.h"
#include "projectsearchpanel.h"

class ProjectSearchTest : public QObject {
    Q_OBJECT
private:
    static void write(const QString &path, const QByteArray &text) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QCOMPARE(f.write(text), text.size());
    }
private slots:
    void panelRepeatedQueriesAndNavigation() {
        QTemporaryDir dir;
        write(dir.filePath("a.c"), "first\n second\n");
        ProjectSearchPanel panel;
        panel.setProjectRoot(dir.path());
        QSignalSpy finished(&panel, &ProjectSearchPanel::searchFinished);
        QSignalSpy navigation(&panel, &ProjectSearchPanel::sourceLocationActivated);
        panel.startSearch("first");
        panel.startSearch("second");
        QTRY_VERIFY_WITH_TIMEOUT(!panel.isSearching(), 10000);
        auto *rows = panel.findChild<QTreeWidget *>("projectSearchResults");
        QVERIFY(rows);
        QCOMPARE(rows->topLevelItemCount(), 1);
        QCOMPARE(rows->topLevelItem(0)->text(1), QString("2:2"));
        QMetaObject::invokeMethod(rows, "itemActivated", Qt::DirectConnection,
            Q_ARG(QTreeWidgetItem *, rows->topLevelItem(0)), Q_ARG(int, 0));
        QCOMPARE(navigation.size(), 1);
        QCOMPARE(navigation[0][1].toInt(), 2);
        QCOMPARE(navigation[0][2].toInt(), 2);
        QVERIFY(finished.size() >= 1);
        ProjectSearchOptions options; options.regularExpression = true;
        panel.startSearch("[", options);
        QCOMPARE(rows->topLevelItemCount(), 1);
        QVERIFY(panel.findChild<QLabel *>("projectSearchStatus")->text().contains(QStringLiteral("无效")));
        panel.setCurrentBuffer(dir.filePath("a.c"), "second\nsecond");
        panel.startSearch("second");
        QTRY_VERIFY_WITH_TIMEOUT(!panel.isSearching(), 10000);
        QCOMPARE(rows->topLevelItemCount(), 2);
    }
    void destroyRunningPanel() {
        QTemporaryDir dir;
        write(dir.filePath("a.cpp"), QByteArray(1000000, 'x'));
        auto *panel = new ProjectSearchPanel;
        panel->setProjectRoot(dir.path());
        panel->startSearch("x");
        delete panel;
        QTest::qWait(100);
    }
    void panelCancellationAndProjectChange() {
        QTemporaryDir dir;
        write(dir.filePath("a.c"), "hit hit hit");
        ProjectSearchPanel panel;
        panel.setProjectRoot(dir.path());
        panel.startSearch("hit");
        panel.cancelSearch();
        QTRY_VERIFY_WITH_TIMEOUT(!panel.isSearching(), 10000);
        QVERIFY(panel.findChild<QLabel *>("projectSearchStatus")->text().contains(QStringLiteral("已取消")));
        panel.startSearch("hit");
        panel.setProjectRoot({});
        QTest::qWait(100);
        QCOMPARE(panel.findChild<QTreeWidget *>("projectSearchResults")->topLevelItemCount(), 0);
        panel.startSearch("hit");
        QVERIFY(panel.findChild<QLabel *>("projectSearchStatus")->text().contains(QStringLiteral("工程")));
    }
    void unicodeIdentifiersAndCrLf() {
        QTemporaryDir dir;
        write(dir.filePath("a.cpp"), QString::fromUtf8("变量 变量后缀 _变量\r\n  变量").toUtf8());
        ProjectSearchRequest r; r.root = dir.path(); r.query = QString::fromUtf8("变量");
        r.options.wholeWord = true;
        const auto result = ProjectSearchEngine::search(r);
        QCOMPARE(result.matches.size(), 2);
        QCOMPARE(result.matches[1].line, 2);
        QCOMPARE(result.matches[1].column, 3);
    }
    void matchingAndBuffer() {
        QTemporaryDir dir;
        write(dir.filePath("main.cpp"), "int needle;\nNEEDLE needle2 _needle needle\n");
        ProjectSearchRequest r;
        r.root = dir.path(); r.query = "needle"; r.options.wholeWord = true;
        auto result = ProjectSearchEngine::search(r);
        QCOMPARE(result.matches.size(), 3);
        QCOMPARE(result.matches[1].line, 2);
        QCOMPARE(result.matches[1].column, 1);
        r.options.caseSensitive = true;
        QCOMPARE(ProjectSearchEngine::search(r).matches.size(), 2);
        r.bufferPath = dir.filePath("main.cpp"); r.bufferText = "\n  needle";
        result = ProjectSearchEngine::search(r);
        QCOMPARE(result.matches.size(), 1);
        QCOMPARE(result.matches[0].line, 2);
        QCOMPARE(result.matches[0].column, 3);
    }
    void invalidAndMultiline() {
        QTemporaryDir dir;
        write(dir.filePath("a.c"), "abc\nxyz\n");
        ProjectSearchRequest r; r.root = dir.path(); r.query = "[";
        r.options.regularExpression = true;
        QVERIFY(!ProjectSearchEngine::search(r).error.isEmpty());
        r.query = "abc\\nxyz";
        auto result = ProjectSearchEngine::search(r);
        QCOMPARE(result.matches.size(), 1);
        QCOMPARE(result.matches[0].line, 1);
        QCOMPARE(result.matches[0].column, 1);
    }
    void boundariesAndCancellation() {
        QTemporaryDir dir;
        write(dir.filePath("src/a.h"), "hit hit hit");
        for (const auto &name : {".git/a.c", "build/a.cpp", ".smilecode/a.h", "debug/a.c", "release/a.c", "node_modules/a.c", "cache/a.c", "a.png"})
            write(dir.filePath(name), "hit");
        write(dir.filePath("binary.c"), QByteArray("hit\0binary", 10));
        write(dir.filePath("large.c"), QByteArray(100, 'x'));
        ProjectSearchRequest r; r.root = dir.path(); r.query = "hit";
        r.options.maxFileBytes = 32;
        auto result = ProjectSearchEngine::search(r);
        QCOMPARE(result.matches.size(), 3);
        QCOMPARE(result.skippedLargeFiles, 1);
        r.options.maxResults = 2;
        QVERIFY(ProjectSearchEngine::search(r).resultLimitReached);
        auto cancel = std::make_shared<std::atomic_bool>(true);
        QVERIFY(ProjectSearchEngine::search(r, cancel).cancelled);
    }
};
QTEST_MAIN(ProjectSearchTest)
#include "ProjectSearchTest.moc"
