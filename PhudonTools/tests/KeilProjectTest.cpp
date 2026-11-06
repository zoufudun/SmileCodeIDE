#include <QtTest>
#include <QFile>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QTreeView>
#include <QFileSystemWatcher>
#include <QSaveFile>
#include <QTabWidget>
#include "keilproject.h"
#include "projectnavigator.h"
#include "codeeditor.h"

class KeilProjectTest : public QObject {
    Q_OBJECT
private:
    static void write(const QString &path, const QByteArray &data) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), qint64(data.size()));
    }
    static QByteArray xml(const QString &groups) {
        return ("<Project><Targets><Target><TargetName>Fire_H7</TargetName>"
                "<TargetOption><TargetCommonOption><Device>STM32H743IITx</Device>"
                "</TargetCommonOption><TargetArmAds><Cads><VariousControls>"
                "<IncludePath>..\\USER</IncludePath></VariousControls></Cads>"
                "</TargetArmAds></TargetOption><Groups>" + groups +
                "</Groups></Target></Targets></Project>").toUtf8();
    }
    static QString group() {
        return QString::fromUtf8(
            "<Group><GroupName>USER</GroupName><Files>"
            "<File><FileName>main.c</FileName><FileType>1</FileType><FilePath>..\\USER\\main.c</FilePath></File>"
            "<File><FileName>缺失.h</FileName><FileType>5</FileType><FilePath>..\\USER\\缺失.h</FilePath></File>"
            "</Files></Group><Group><GroupName>DOC</GroupName><Files>"
            "<File><FileName>必读.txt</FileName><FileType>5</FileType><FilePath>..\\必读.txt</FilePath></File>"
            "</Files></Group>");
    }
private slots:
    void initTestCase() {
        QApplication::setFont(QFont("Segoe UI", 10));
    }
    void orderedTargetsGroupsAndPaths() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QString::fromUtf8("工程/MDK/Fire.uvprojx"));
        QByteArray data = xml(group());
        data.replace("</Targets>", "<Target><TargetName>Release</TargetName><Groups><Group><GroupName>USER</GroupName></Group></Groups></Target></Targets>");
        write(path, data);
        write(QFileInfo(path).dir().filePath("../USER/main.c"), "int main(void) { return 0; }");
        KeilProject project;
        QString error;
        QVERIFY2(project.load(path, &error), qPrintable(error));
        QCOMPARE(project.targets.size(), 2);
        QCOMPARE(project.targets[0].name, QString("Fire_H7"));
        QCOMPARE(project.targets[0].device, QString("STM32H743IITx"));
        QCOMPARE(project.targets[1].name, QString("Release"));
        QCOMPARE(project.targets[0].groups.size(), 2);
        QCOMPARE(project.targets[0].groups[0].name, QString("USER"));
        QCOMPARE(project.targets[0].groups[1].name, QString("DOC"));
        const auto files = project.targets[0].groups[0].files;
        QCOMPARE(files[0].name, QString("main.c"));
        QCOMPARE(files[0].path, QDir::cleanPath(QFileInfo(path).dir().filePath("../USER/main.c")));
        QVERIFY(QFileInfo::exists(files[0].path));
        QVERIFY(!QFileInfo::exists(files[1].path));
        QCOMPARE(project.targets[0].includePaths.first(), QFileInfo(files[0].path).absolutePath());
    }
    void invalidXmlPreservesPreviousModel() {
        QTemporaryDir dir;
        const QString path = dir.filePath("Fire.uvprojx");
        write(path, xml(group()));
        KeilProject project;
        QVERIFY(project.load(path));
        write(path, "<Project><Targets><Target>");
        QString error;
        QVERIFY(!project.load(path, &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(project.targets[0].name, QString("Fire_H7"));
        write(path, "<Other><Target><TargetName>invalid</TargetName></Target></Other>");
        QVERIFY(!project.load(path, &error));
        write(path, "<Project><Targets/></Project>");
        QVERIFY(!project.load(path, &error));
    }
    void symbolsIgnoreCommentsCallsAndFunctionPointers() {
        const QString code = "// void phantom(void) {}\n"
            "#define CALL(x) fake(x)\n"
            "static void led(int pin)\n{ puts(\"fake()\"); if (pin) led(pin - 1); }\n"
            "int main(void) { led(1); return 0; }\n"
            "void (*callback)(int);\n";
        const auto symbols = CodeEditor::symbolsForCode(code, "main.c");
        QStringList names;
        for (const auto &symbol : symbols)
            if (symbol.type == CodeEditor::SymbolFunction) names << symbol.scopedName;
        QCOMPARE(names, QStringList({"led", "main"}));
        const auto headers = CodeEditor::symbolsForCode("void declared(int pin);\nstatic inline int get(void) { return 1; }", "test.h");
        QStringList declarations;
        for (const auto &symbol : headers)
            if (symbol.type == CodeEditor::SymbolFunction) declarations << symbol.scopedName;
        QVERIFY(declarations.contains("declared"));
        QVERIFY(declarations.contains("get"));
    }
    void navigatorFunctionsDependenciesAndNavigation() {
        QTemporaryDir dir;
        const QString path = dir.filePath("MDK/Fire.uvprojx");
        write(path, xml(group()));
        const QString source = dir.filePath("USER/main.c");
        write(source, "#include \"main.h\"\nint main(void) { return 0; }\n");
        write(dir.filePath("USER/main.h"), "#include \"nested.h\"\nvoid led(int pin);\n");
        write(dir.filePath("USER/nested.h"), "#include \"main.h\"\nvoid nested(void);\n");
        KeilProject project;
        QVERIFY(project.load(path));
        ProjectNavigator nav;
        nav.setProject(project);
        auto *tree = nav.projectView();
        auto *model = tree->model();
        auto root = model->index(0, 0);
        auto target = model->index(0, 0, root);
        auto user = model->index(0, 0, target);
        QCOMPARE(user.data().toString(), QString("USER"));
        auto file = model->index(0, 0, user);
        QCOMPARE(file.data().toString(), QString("main.c"));
        tree->expand(file);
        QTRY_COMPARE(model->rowCount(file), 1);
        auto function = model->index(0, 0, file);
        QVERIFY(function.data().toString().startsWith("main"));
        QSignalSpy activated(&nav, &ProjectNavigator::sourceLocationActivated);
        QVERIFY(QMetaObject::invokeMethod(tree, "doubleClicked", Q_ARG(QModelIndex, function)));
        QCOMPARE(activated.count(), 1);
        QCOMPARE(activated[0][0].toString(), source);
        QCOMPARE(activated[0][1].toInt(), 2);
        auto *functions = nav.functionsView();
        QTRY_COMPARE(functions->model()->rowCount(), 4); // missing header retained + local include closure
        QStringList files;
        for (int i = 0; i < functions->model()->rowCount(); ++i)
            files << functions->model()->index(i, 0).data().toString();
        QVERIFY(files.contains("main.h"));
        QVERIFY(files.contains("nested.h"));
        nav.setSourceBuffer(source, "\n\nvoid changed(int value) {}\n");
        QTRY_VERIFY(model->index(0, 0, file).data().toString().startsWith("changed"));
        nav.clear();
        QCOMPARE(model->rowCount(), 0);
        QCOMPARE(functions->model()->rowCount(), 0);
    }
    void refreshReadsOriginalProject() {
        QTemporaryDir dir;
        const QString path = dir.filePath("Fire.uvprojx");
        write(path, xml(group()));
        KeilProject project;
        QVERIFY(project.load(path));
        ProjectNavigator nav;
        nav.setProject(project);
        write(path, xml("<Group><GroupName>UPDATED</GroupName></Group>"));
        QVERIFY(nav.reload());
        const auto *m = nav.projectView()->model();
        QCOMPARE(m->index(0, 0, m->index(0, 0, m->index(0, 0))).data().toString(), QString("UPDATED"));
        write(path, "broken");
        QVERIFY(!nav.reload());
        QCOMPARE(m->index(0, 0, m->index(0, 0, m->index(0, 0))).data().toString(), QString("UPDATED"));
    }
    void originalAtomicSaveAndRecovery() {
        QTemporaryDir dir;
        const QString path = dir.filePath("Fire.uvprojx");
        write(path, xml(group()));
        KeilProject project;
        QVERIFY(project.load(path));
        ProjectNavigator nav;
        nav.setProject(project);
        auto groupName = [&] {
            auto *m = nav.projectView()->model();
            return m->index(0, 0, m->index(0, 0, m->index(0, 0))).data().toString();
        };
        QSaveFile replacement(path);
        QVERIFY(replacement.open(QIODevice::WriteOnly));
        replacement.write(xml("<Group><GroupName>ATOMIC</GroupName></Group>"));
        QVERIFY(replacement.commit());
        QTRY_COMPARE(groupName(), QString("ATOMIC"));
        QVERIFY(QFile::remove(path));
        QTest::qWait(400);
        QCOMPARE(groupName(), QString("ATOMIC"));
        write(path, xml("<Group><GroupName>RECOVERED</GroupName></Group>"));
        QTRY_COMPARE(groupName(), QString("RECOVERED"));
    }
    void cleanBufferRefreshAndWatchPruning() {
        QTemporaryDir dir;
        const QString path = dir.filePath("MDK/Fire.uvprojx");
        const QString source = dir.filePath("USER/main.c");
        write(path, xml(group()));
        write(source, "void original(void) {}\n");
        KeilProject project;
        QVERIFY(project.load(path));
        ProjectNavigator nav;
        nav.setProject(project);
        auto *tree = nav.projectView();
        auto *m = tree->model();
        auto file = m->index(0, 0, m->index(0, 0, m->index(0, 0, m->index(0, 0))));
        nav.setSourceBuffer(source, "void original(void) {}\n", false);
        tree->expand(file);
        QVERIFY(m->index(0, 0, file).data().toString().startsWith("original"));
        write(source, "void external(void) {}\n");
        QTRY_VERIFY(m->index(0, 0, file).data().toString().startsWith("external"));
        nav.setSourceBuffer(source, "void unsaved(void) {}\n", true);
        QTRY_VERIFY(m->index(0, 0, file).data().toString().startsWith("unsaved"));
        write(source, "void disk(void) {}\n");
        QTest::qWait(350);
        QVERIFY(m->index(0, 0, file).data().toString().startsWith("unsaved"));
        write(path, xml("<Group><GroupName>EMPTY</GroupName></Group>"));
        QVERIFY(nav.reload());
        auto *watcher = nav.findChild<QFileSystemWatcher *>();
        QVERIFY(watcher);
        QVERIFY(!watcher->files().contains(source));
        QVERIFY(!watcher->directories().contains(QFileInfo(source).absolutePath()));
    }
    void newDocumentDoesNotOverridePreviousFile() {
        QTemporaryDir dir;
        const auto path = dir.filePath("main.c");
        write(path, "int main(void) { return 0; }\n");
        CodeEditor editor;
        QVERIFY(editor.openFile(path));
        QSignalSpy buffer(&editor, &CodeEditor::currentBufferChanged);
        QVERIFY(editor.newDocument());
        QVERIFY(editor.currentFilePath().isEmpty());
        QVERIFY(editor.text().isEmpty());
        QVERIFY(!buffer.isEmpty());
        QVERIFY(buffer.last()[0].toString().isEmpty());
    }
    void readsRealProjectWhenProvided() {
        const auto path = qEnvironmentVariable("KEIL_TEST_PROJECT");
        if (path.isEmpty()) QSKIP("No external sample requested");
        KeilProject project;
        QString error;
        QVERIFY2(project.load(path, &error), qPrintable(error));
        QCOMPARE(project.targets[0].name, QString("Fire_H7"));
        QVERIFY(project.targets[0].groups.size() >= 5);
        ProjectNavigator nav;
        nav.resize(380, 800);
        nav.setProject(project);
        auto *projectView = nav.projectView();
        auto *pm = projectView->model();
        const auto target = pm->index(0, 0, pm->index(0, 0));
        for (int i = 0; i < pm->rowCount(target); ++i) {
            const auto group = pm->index(i, 0, target);
            if (pm->rowCount(group) <= 12) projectView->expand(group);
        }
        nav.show();
        QTest::qWait(1000);
        QTRY_VERIFY_WITH_TIMEOUT(nav.functionsView()->model()->rowCount() > 80, 20000);
        const auto out = qEnvironmentVariable("KEIL_TEST_ARTIFACTS");
        if (!out.isEmpty()) {
            QDir().mkpath(out);
            QVERIFY(nav.grab().save(QDir(out).filePath("keil-project.png")));
            auto *functions = nav.functionsView();
            auto *fm = functions->model();
            for (int i = 0; i < fm->rowCount(); ++i) {
                const auto file = fm->index(i, 0);
                if (file.data().toString() == "main.c" || file.data().toString() == "bsp_led.c") {
                    functions->expand(file);
                    QVERIFY(fm->rowCount(file) > 0);
                }
            }
            nav.findChild<QTabWidget *>()->setCurrentIndex(1);
            for (int i = 0; i < fm->rowCount(); ++i)
                if (fm->index(i, 0).data().toString() == "main.c")
                    functions->scrollTo(fm->index(i, 0), QAbstractItemView::PositionAtCenter);
            QTest::qWait(100);
            QVERIFY(nav.grab().save(QDir(out).filePath("keil-functions.png")));
        }
    }
    void cleanReloadKeepsRedoWhenDiskIsUnchanged() {
        QTemporaryDir dir;
        const auto path = dir.filePath("main.c");
        write(path, "int main(void) { return 0; }\n");
        CodeEditor editor;
        QVERIFY(editor.openFile(path));
        editor.currentEditor()->insertAt("// unsaved\n", 0, 0);
        editor.currentEditor()->undo();
        QVERIFY(!editor.currentEditor()->isModified());
        QVERIFY(editor.openFile(path, true));
        editor.currentEditor()->redo();
        QVERIFY(editor.text().startsWith("// unsaved"));
        editor.currentEditor()->setModified(false);
    }
};
QTEST_MAIN(KeilProjectTest)
#include "KeilProjectTest.moc"
