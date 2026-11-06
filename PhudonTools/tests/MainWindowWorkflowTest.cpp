#include "MainWindowWorkflowTest.h"
#include <QMessageBox>
#include <QTimer>



void MainWindowWorkflowTest::initTestCase() {

    QCoreApplication::setApplicationVersion("2.2.0");

    QSettings::setDefaultFormat(QSettings::IniFormat);

    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QSettings reg(QSettings::defaultFormat(), QSettings::UserScope, "PhudonTools", "Settings");
    reg.clear();
    reg.sync();

}



void MainWindowWorkflowTest::controllerExistsAndActionsFollowState() {

        MainWindow window;

        auto controller = window.findChild<STM32DebugController *>();

        QVERIFY(controller);

        auto build = window.findChild<QAction *>(QStringLiteral("编译"));

        auto pause = window.findChild<QAction *>("pauseDebug");

        auto cancel = window.findChild<QAction *>("cancelCurrentTask");

        auto step = window.findChild<QAction *>(QStringLiteral("单步进入"));

        QVERIFY(build && pause && cancel && step);

        QVERIFY(!build->isEnabled());

        QVERIFY(!pause->isEnabled());

        QVERIFY(!cancel->isEnabled());



        QTemporaryDir project;

        QFile source(project.filePath("main.c"));

        QVERIFY(source.open(QIODevice::WriteOnly));

        source.write("int main(void) { return 0; }\n");

        source.close();

        window.findChild<CodeEditor *>()->currentEditor()->setModified(false);

        QVERIFY(window.openSourceLocation(source.fileName(), 1));

        controller->openProject(project.path());

        QVERIFY(build->isEnabled());

        QVERIFY(!pause->isEnabled());

        auto codeEditor = window.findChild<CodeEditor *>();

        QSignalSpy toggled(codeEditor, &CodeEditor::breakpointToggled);

        QVERIFY(QMetaObject::invokeMethod(codeEditor->currentEditor(), "marginClicked",

            Q_ARG(int, 1), Q_ARG(int, 0),

            Q_ARG(Qt::KeyboardModifiers, Qt::KeyboardModifiers(Qt::NoModifier))));

        QCOMPARE(toggled.count(), 1);

        QVERIFY(QMetaObject::invokeMethod(controller, "onGdbTargetRunning"));

        QVERIFY(!build->isEnabled());

        QVERIFY(pause->isEnabled());

        QVERIFY(cancel->isEnabled());

        QVERIFY(!step->isEnabled());



        QVERIFY(QMetaObject::invokeMethod(controller, "onGdbTargetStopped",

            Q_ARG(QString, source.fileName()), Q_ARG(int, 1),

            Q_ARG(QString, QString("main")), Q_ARG(QString, QString("test"))));

        QVERIFY(!pause->isEnabled());

        QVERIFY(step->isEnabled());

        controller->stopDebug();

        QVERIFY(build->isEnabled());

        QVERIFY(!cancel->isEnabled());

        QVERIFY(!step->isEnabled());

        controller->addBreakpoint(source.fileName(), 1);

        QVERIFY(!window.findChild<CodeEditor *>()->getAllBreakpoints().isEmpty());

        auto firstPane = codeEditor->currentEditor();

        const auto breakpointMarker = firstPane->markersAtLine(0);

        QVERIFY(breakpointMarker != 0);

        codeEditor->createSplitView();

        controller->addBreakpoint(source.fileName(), 2);

        QCOMPARE(firstPane->markersAtLine(0), breakpointMarker);

        controller->setAllBreakpoints({});

        QVERIFY(window.findChild<CodeEditor *>()->getAllBreakpoints().isEmpty());

        QCOMPARE(firstPane->markersAtLine(0), 0U);

        codeEditor->closeSplitView();

        window.resize(1500, 900);

        window.show();

        QTest::qWait(100);

        QDir().mkpath("../build"); QVERIFY(window.grab().save("../build/workflow-window.png"));

        QVERIFY(QMetaObject::invokeMethod(&window, "closeProject"));

        QVERIFY(!build->isEnabled());

        QVERIFY(!window.findChild<QAction *>("rebuildProject")->isEnabled());

    }



void MainWindowWorkflowTest::renderWorkflowIcons() {

        using namespace IDEWorkflowIcons;

        const QList<Kind> kinds = {Kind::Build, Kind::Rebuild, Kind::Clean, Kind::Flash,

            Kind::Debug, Kind::Stop, Kind::Pause, Kind::Continue, Kind::StepOver,

            Kind::StepInto, Kind::StepOut, Kind::Breakpoint,
            Kind::ChipConfig, Kind::FlashConfig, Kind::ToolchainConfig};

        QImage sheet(900, 240, QImage::Format_ARGB32_Premultiplied);

        sheet.fill(QColor("#1e293b"));

        QPainter painter(&sheet);

        for (int row = 0; row < 3; ++row) {

            if (row == 2) painter.fillRect(0, 160, 900, 80, QColor("#f8fafc"));

            for (int i = 0; i < kinds.size(); ++i) {

                const QIcon glyph = icon(kinds[i]);

                QVERIFY(!glyph.pixmap(QSize(22, 22)).isNull());

                glyph.paint(&painter, QRect(i * 60 + 10, row * 80 + 12, 40, 40),

                    Qt::AlignCenter, row == 1 ? QIcon::Disabled : QIcon::Normal);

            }

        }

        painter.end();

        QDir().mkpath("../build"); QVERIFY(sheet.save("../build/workflow-icons.png"));

    }



void MainWindowWorkflowTest::compactWorkspaceHasExplicitConfiguration() {

        MainWindow window;

        window.show();

        QTest::qWait(50);

        const auto toolbars = window.findChildren<QToolBar *>(QString(), Qt::FindDirectChildrenOnly);

        QCOMPARE(toolbars.size(), 1);

        auto editor = window.findChild<CodeEditor *>();

        QVERIFY(editor->findChildren<QToolBar *>().isEmpty());

        QVERIFY(window.findChild<QAction *>("configureChip"));

        QVERIFY(window.findChild<QAction *>("configureFlash"));

        QVERIFY(window.findChild<QAction *>("configureToolchain"));

        QVERIFY(window.findChild<QComboBox *>("mainBuildLogModeCombo"));
        QVERIFY(window.findChild<QComboBox *>("mainBuildLogFilterCombo"));
        QVERIFY(window.findChild<QComboBox *>("mainProblemsFilterCombo"));

        const auto docks = window.findChildren<QDockWidget *>();

        for (auto dock : docks) QVERIFY(!dock->isVisible());
        window.close();
        QTest::qWait(50);

    }



void MainWindowWorkflowTest::cancelledConfigurationDialogsKeepSavedState() {

        MainWindow window;

        auto controller = window.findChild<STM32DebugController *>();

        QVERIFY(controller);

        QTemporaryDir project;

        controller->openProject(project.path());

        FlashOptions saved;

        saved.driver = FlashDriverType::OpenOCD_CMSISDAP;

        saved.targetAddress = "0x08004000";

        saved.swdSpeedKhz = 1200;

        saved.probeSerial = "keep-serial";

        controller->setFlashOptions(saved);

        const QString chip = controller->targetChip();

        const ToolchainPaths tools = controller->toolchainPaths();

        QFile config(project.filePath(".smilecode/project.json"));

        QVERIFY(config.open(QIODevice::ReadOnly));

        const QByteArray before = config.readAll();

        config.close();

        QSettings userSettings;

        const QStringList userKeys = userSettings.allKeys();



        for (const char *name : {"configureChip", "configureFlash", "configureToolchain"}) {

            bool sawDialog = false;

            QTimer::singleShot(0, this, [&sawDialog]() { rejectModal(sawDialog); });

            window.findChild<QAction *>(name)->trigger();

            QVERIFY2(sawDialog, name);

        }



        QCOMPARE(controller->targetChip(), chip);

        QCOMPARE(controller->flashOptions().targetAddress, saved.targetAddress);

        QCOMPARE(controller->flashOptions().swdSpeedKhz, saved.swdSpeedKhz);

        QCOMPARE(controller->flashOptions().probeSerial, saved.probeSerial);

        QCOMPARE(controller->toolchainPaths().gccPath, tools.gccPath);

        QVERIFY(config.open(QIODevice::ReadOnly));

        QCOMPARE(config.readAll(), before);

        config.close();

        userSettings.sync();

        QCOMPARE(userSettings.allKeys(), userKeys);

    }





void MainWindowWorkflowTest::testMenuUniquenessAndBuildSafety() {
    MainWindow window;
    auto bar = window.menuBar();
    QVERIFY(bar);
    QStringList menuTitles;
    for (auto action : bar->actions()) {
        if (auto m = action->menu()) {
            menuTitles << m->title().remove("&").trimmed();
        }
    }
    qDebug() << "Menu bar titles in order:" << menuTitles;
    // 验证“视图”只出现一次，绝不重复
    QCOMPARE(menuTitles.count("视图"), 1);
    QCOMPARE(menuTitles.count("文件"), 1);
    QCOMPARE(menuTitles.count("编辑"), 1);
    QCOMPARE(menuTitles.count("搜索"), 1);
    QCOMPARE(menuTitles.count("导航"), 1);
    QCOMPARE(menuTitles.count("构建"), 1);
    QCOMPARE(menuTitles.count("调试"), 1);
    QCOMPARE(menuTitles.count("工具"), 1);
    QCOMPARE(menuTitles.count("终端"), 1);
    QCOMPARE(menuTitles.count("帮助"), 1);

    // 验证在没有项目时触发编译，不崩溃且友好处理
    auto build = window.findChild<QAction *>(QStringLiteral("编译"));
    QVERIFY(build);
    QTimer::singleShot(100, []() {
        if (auto active = QApplication::activeModalWidget()) {
            active->close();
        }
    });
    build->trigger();
    QTest::qWait(200);
}

void MainWindowWorkflowTest::testBuildTriggerCrash() {
    MainWindow window;
    auto controller = window.findChild<STM32DebugController *>();
    QVERIFY(controller);

    QTemporaryDir project;
    QFile source(project.filePath("main.c"));
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("int main(void) { return 0; }\n");
    source.close();

    controller->openProject(project.path());
    auto build = window.findChild<QAction *>(QStringLiteral("编译"));
    QVERIFY(build);
    QVERIFY(build->isEnabled());

    qDebug() << "Triggering build with valid project...";
    QTimer::singleShot(100, []() {
        if (auto active = QApplication::activeModalWidget()) {
            active->close();
        }
    });
    build->trigger();
    QTest::qWait(3500);
    qDebug() << "Build wait completed, state:" << static_cast<int>(controller->state());

    // Scenario A: Close project, so no project exists
    QVERIFY(QMetaObject::invokeMethod(&window, "closeProject"));
    qDebug() << "Closed project, now triggering build without project...";
    QTimer::singleShot(100, []() {
        if (auto active = QApplication::activeModalWidget()) {
            active->close();
        }
    });
    build->trigger();
    QTest::qWait(500);
    qDebug() << "Build without project handled safely!";

    // Scenario B: Open a single file that is not a project
    QTemporaryDir singleFileDir;
    QString singleFile = singleFileDir.filePath("test_single.c");
    QFile sf(singleFile);
    sf.open(QIODevice::WriteOnly);
    sf.write("int main() { while(1); }\n");
    sf.close();

    window.openSourceLocation(singleFile, 1);
    qDebug() << "Opened single file, triggering build...";
    QTimer::singleShot(100, []() {
        if (auto active = QApplication::activeModalWidget()) {
            active->close();
        }
    });
    build->trigger();
    QTest::qWait(3500);
    qDebug() << "Build with single file handled safely!";
}

void MainWindowWorkflowTest::acceptedFlashDialogWritesOnlyCurrentProject() {

        MainWindow window;

        auto controller = window.findChild<STM32DebugController *>();

        QTemporaryDir first;

        QTemporaryDir second;

        controller->openProject(second.path());

        FlashOptions secondOptions;

        secondOptions.targetAddress = "0x08008000";

        controller->setFlashOptions(secondOptions);

        controller->openProject(first.path());



        bool sawDialog = false;

        QTimer::singleShot(0, this, [&sawDialog]() {

            withModal(sawDialog, [](QDialog *dialog) {

                dialog->findChild<QLineEdit *>("flashAddress")->setText("0x08002000");

                dialog->findChild<QSpinBox *>("flashSpeed")->setValue(2000);

                // A warning (e.g. several attached probes) must not hang the test.

                QTimer::singleShot(300, dialog, [dialog]() {

                    auto box = qobject_cast<QDialog *>(QApplication::activeModalWidget());

                    if (box && box != dialog) box->reject();

                });

                dialog->findChild<QPushButton *>("saveFlashConfiguration")->click();

                if (dialog->isVisible()) dialog->reject();

            });

        });

        window.findChild<QAction *>("configureFlash")->trigger();

        QVERIFY(sawDialog);

        QCOMPARE(controller->flashOptions().targetAddress, QString("0x08002000"));

        QCOMPARE(controller->flashOptions().swdSpeedKhz, 2000);



        controller->openProject(second.path());

        QCOMPARE(controller->flashOptions().targetAddress, secondOptions.targetAddress);

        controller->openProject(first.path());

        QCOMPARE(controller->flashOptions().targetAddress, QString("0x08002000"));

    }



void MainWindowWorkflowTest::withModal(bool &sawDialog, std::function<void(QDialog *)> action, int attempts) {

    if (auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {

        sawDialog = true;

        action(dialog);

        return;

    }

    if (attempts > 0)

        QTimer::singleShot(20, [&sawDialog, action, attempts]() {

            withModal(sawDialog, action, attempts - 1);

        });

}



void MainWindowWorkflowTest::rejectModal(bool &sawDialog) {

    withModal(sawDialog, [](QDialog *dialog) { dialog->reject(); });

}






void MainWindowWorkflowTest::testRecentProjectDirectoryResolution() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString projectDir = QDir::toNativeSeparators(tempDir.path());

    MainWindow window;
    window.closeProject();

    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope, "PhudonTools", "Settings");
    settings.beginGroup("MainWindow");
    settings.setValue("lastOpenDir", projectDir);
    settings.endGroup();
    settings.sync();

    QString resolved = window.getRecentProjectDirectory();
    QCOMPARE(QDir::cleanPath(resolved), QDir::cleanPath(projectDir));
}

void MainWindowWorkflowTest::keilImportNavigationAndDirectorySwitch() {
    QTemporaryDir dir;
    QDir().mkpath(dir.filePath("MDK"));
    QFile source(dir.filePath("main.c"));
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("\nint main(void) { return 0; }\n");
    source.close();
    QFile project(dir.filePath("MDK/Fire.uvprojx"));
    QVERIFY(project.open(QIODevice::WriteOnly));
    project.write("<Project><Targets><Target><TargetName>Fire_H7</TargetName><Groups><Group><GroupName>USER</GroupName><Files><File><FileName>main.c</FileName><FilePath>../main.c</FilePath></File></Files></Group></Groups></Target></Targets></Project>");
    project.close();
    {
        MainWindow window;
        bool imported = false;
        QVERIFY(QMetaObject::invokeMethod(&window, "importKeilProject", Q_RETURN_ARG(bool, imported), Q_ARG(QString, project.fileName())));
        QVERIFY(imported);
        auto *nav = window.findChild<QWidget *>("keilProjectNavigator");
        QVERIFY(nav && !nav->isHidden());
        auto *tree = window.findChild<QTreeView *>("keilProjectTree");
        QVERIFY(tree);
        auto *model = tree->model();
        QCOMPARE(model->index(0, 0, model->index(0, 0)).data().toString(), QString("Fire_H7"));
        QVERIFY(window.openSourceLocation(source.fileName(), 2));
        auto *editor = window.findChild<CodeEditor *>();
        editor->currentEditor()->insertAt("// unsaved\n", 0, 0);
        const auto text = editor->text();
        // Same-file navigation must keep the unsaved buffer.
        QVERIFY(window.openSourceLocation(source.fileName(), 3));
        QCOMPARE(editor->text(), text);
        QFile other(dir.filePath("other.c"));
        QVERIFY(other.open(QIODevice::WriteOnly));
        other.write("void other(void) {}\n");
        other.close();
        int prompts = 0;
        QMessageBox::StandardButton answer = QMessageBox::Cancel;
        QTimer dialogResponder;
        dialogResponder.setInterval(5);
        connect(&dialogResponder, &QTimer::timeout, &window, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                ++prompts;
                box->button(answer)->click();
            }
        });
        dialogResponder.start();
        QVERIFY(!window.openSourceLocation(other.fileName(), 1));
        QCOMPARE(prompts, 1);
        QCOMPARE(editor->text(), text);
        prompts = 0;
        answer = QMessageBox::Discard;
        QVERIFY(window.openSourceLocation(other.fileName(), 1));
        QCOMPARE(prompts, 1);
        editor->currentEditor()->insertAt("// saved\n", 0, 0);
        const QString savedText = editor->text();
        prompts = 0;
        answer = QMessageBox::Save;
        QVERIFY(window.openSourceLocation(source.fileName(), 1));
        QCOMPARE(prompts, 1);
        dialogResponder.stop();
        QVERIFY(other.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(QString::fromUtf8(other.readAll()), savedText);
        editor->currentEditor()->setModified(false);
        imported = true;
        QVERIFY(QMetaObject::invokeMethod(&window, "importKeilProject", Q_RETURN_ARG(bool, imported), Q_ARG(QString, dir.filePath("missing.uvprojx"))));
        QVERIFY(!imported);
        QCOMPARE(model->rowCount(), 1);
    }
    {
        MainWindow restored;
        auto *nav = restored.findChild<QWidget *>("keilProjectNavigator");
        QVERIFY(nav && !nav->isHidden());
        QVERIFY(QMetaObject::invokeMethod(&restored, "updateProjectTree", Q_ARG(QString, dir.path())));
        QVERIFY(nav->isHidden());
        restored.closeProject();
        QCOMPARE(restored.findChild<QTreeView *>("projectFunctionsTree")->model()->rowCount(), 0);
        QSettings settings(QSettings::defaultFormat(), QSettings::UserScope, "PhudonTools", "Settings");
        QVERIFY(settings.value("MainWindow/keilProjectFile").toString().isEmpty());
    }
}

void MainWindowWorkflowTest::keilBufferUndoRedoTracksSavePoint() {
    QTemporaryDir dir;
    QFile source(dir.filePath("main.c"));
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("int main(void) { return 0; }\n");
    source.close();
    QFile project(dir.filePath("Fire.uvprojx"));
    QVERIFY(project.open(QIODevice::WriteOnly));
    const QByteArray original = "<Project><Targets><Target><TargetName>Fire_H7</TargetName><Groups><Group><GroupName>USER</GroupName><Files><File><FileName>main.c</FileName><FilePath>main.c</FilePath></File></Files></Group></Groups></Target></Targets></Project>";
    project.write(original);
    project.close();
    MainWindow window;
    bool imported = false;
    QVERIFY(QMetaObject::invokeMethod(&window, "importKeilProject", Q_RETURN_ARG(bool, imported), Q_ARG(QString, project.fileName())));
    QVERIFY(imported);
    QVERIFY(window.openSourceLocation(source.fileName(), 1));
    auto *editor = window.findChild<CodeEditor *>()->currentEditor();
    auto *tree = window.findChild<QTreeView *>("keilProjectTree");
    auto *m = tree->model();
    const auto file = m->index(0, 0, m->index(0, 0, m->index(0, 0, m->index(0, 0))));
    tree->expand(file);
    editor->insertAt("void added(void) {}\n", 1, 0);
    QVERIFY(editor->isModified());
    QTRY_COMPARE(m->rowCount(file), 2);
    editor->undo();
    QVERIFY(!editor->isModified());
    QTRY_COMPARE(m->rowCount(file), 1);
    QVERIFY(source.open(QIODevice::WriteOnly | QIODevice::Truncate));
    source.write("void external(void) {}\n");
    source.close();
    QTRY_VERIFY(m->index(0, 0, file).data().toString().startsWith("external"));
    editor->redo();
    QVERIFY(editor->isModified());
    QTRY_COMPARE(m->rowCount(file), 2);
    QVERIFY(m->index(0, 0, file).data().toString().startsWith("main"));
    QVERIFY(project.open(QIODevice::ReadOnly));
    QCOMPARE(project.readAll(), original);
    editor->setModified(false);
    window.closeProject();
}

QTEST_MAIN(MainWindowWorkflowTest)

