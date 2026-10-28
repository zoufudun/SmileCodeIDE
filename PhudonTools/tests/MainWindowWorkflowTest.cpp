#include <QtTest>
#include <QSettings>
#include <QTemporaryDir>
#include "mainwindow.h"
#include "ideworkflowicons.h"

class MainWindowWorkflowTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setApplicationVersion("2.2.0");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    }

    void controllerExistsAndActionsFollowState() {
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
        QVERIFY(window.grab().save("../build/workflow-window.png"));
        QVERIFY(QMetaObject::invokeMethod(&window, "closeProject"));
        QVERIFY(!build->isEnabled());
        QVERIFY(!window.findChild<QAction *>("rebuildProject")->isEnabled());
    }

    void renderWorkflowIcons() {
        using namespace IDEWorkflowIcons;
        const QList<Kind> kinds = {Kind::Build, Kind::Rebuild, Kind::Clean, Kind::Flash,
            Kind::Debug, Kind::Stop, Kind::Pause, Kind::Continue, Kind::StepOver,
            Kind::StepInto, Kind::StepOut, Kind::Breakpoint};
        QImage sheet(720, 240, QImage::Format_ARGB32_Premultiplied);
        sheet.fill(QColor("#1e293b"));
        QPainter painter(&sheet);
        for (int row = 0; row < 3; ++row) {
            if (row == 2) painter.fillRect(0, 160, 720, 80, QColor("#f8fafc"));
            for (int i = 0; i < kinds.size(); ++i) {
                const QIcon glyph = icon(kinds[i]);
                QVERIFY(!glyph.pixmap(QSize(22, 22)).isNull());
                glyph.paint(&painter, QRect(i * 60 + 10, row * 80 + 12, 40, 40),
                    Qt::AlignCenter, row == 1 ? QIcon::Disabled : QIcon::Normal);
            }
        }
        painter.end();
        QVERIFY(sheet.save("../build/workflow-icons.png"));
    }
private:
    QTemporaryDir m_settings;
};

QTEST_MAIN(MainWindowWorkflowTest)
#include "MainWindowWorkflowTest.moc"
