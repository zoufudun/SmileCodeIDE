#include <QtTest>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QThread>
#include <QTreeWidget>
#include <cstdio>
#include "../UI/STM32WorkbenchWidget.h"

class BuildExperienceWidgetTest : public QObject {
    Q_OBJECT
private slots:
    void conciseOutputAndDetails();
    void warningsAndErrorsCanBeFilteredAndNavigated();
    void backendCountsLinkerFailures();
    void backendRejectsErrorWithZeroExit();
    void reportsNoWorkAndCleanCorrectly();
    void cancelledBuildCannotSucceed();
    void generatedRecipesAnnounceStages();
    void reportsStartupFailure();
    void rebuildHasOneFinalResult();
    void newBuildClearsOldProblems();
    void downloadViewSurvivesLateBuildOutput();
    void repeatedBuildClicksPreserveOutput();
    void decodesSplitChinesePathBytes();
    void realCompilerBuildAndIncremental();
    void capturesPreviews();
};

static bool prepareProject(const QString &root)
{
    QDir().mkpath(root + "/User");
    QFile source(root + "/User/main.c");
    if (!source.open(QIODevice::WriteOnly)) return false;
    source.write("int main(void) { return 0; }\n");
    source.close();
    QFile linker(root + "/flash.ld");
    if (!linker.open(QIODevice::WriteOnly)) return false;
    linker.write("SECTIONS {}\n");
    return true;
}

static void configure(QSettings &settings)
{
    settings.setValue("STM32Toolchain/makePath", QCoreApplication::applicationFilePath());
    settings.setValue("STM32Toolchain/selectedChip", "STM32F429IGTx");
}

static bool startFixture(STM32WorkbenchWidget &widget, const QString &project, int mode)
{
    qputenv("BUILD_OUTPUT_FIXTURE", QByteArray::number(mode));
    widget.setProjectPath(project);
    return widget.buildManager()->startBuild(project, BuildConfiguration::Debug, 2);
}

static QString outputText(STM32WorkbenchWidget &widget)
{
    return widget.findChild<QTextBrowser *>()->toPlainText();
}

void BuildExperienceWidgetTest::conciseOutputAndDetails()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(startFixture(widget, temporary.path(), 0));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QVERIFY(finished.first().first().toBool());
    QVERIFY(outputText(widget).contains("compiling main.c..."));
    QVERIFY(outputText(widget).contains("linking"));
    QVERIFY(outputText(widget).contains("Flash"));
    QVERIFY(!outputText(widget).contains("-mcpu=cortex-m4"));
    auto *details = widget.findChild<QCheckBox *>("flashDetailedLog");
    QVERIFY(details && details->isEnabled());
    details->setChecked(true);
    QVERIFY(outputText(widget).contains("-mcpu=cortex-m4"));
    details->setChecked(false);
    QVERIFY(!outputText(widget).contains("-mcpu=cortex-m4"));
}

void BuildExperienceWidgetTest::warningsAndErrorsCanBeFilteredAndNavigated()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QSignalSpy jump(&widget, &STM32WorkbenchWidget::jumpToSourceLine);
    QVERIFY(startFixture(widget, temporary.path(), 2));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    auto *problems = widget.findChild<QTreeWidget *>("buildProblems");
    auto *filter = widget.findChild<QComboBox *>("buildProblemFilter");
    QVERIFY(problems && filter);
    QCOMPARE(problems->topLevelItemCount(), 2);
    filter->setCurrentIndex(1);
    QCOMPARE(problems->topLevelItemCount(), 1);
    QVERIFY(problems->topLevelItem(0)->text(0).contains(QStringLiteral("错误")));
    filter->setCurrentIndex(2);
    QCOMPARE(problems->topLevelItemCount(), 1);
    QVERIFY(problems->topLevelItem(0)->text(0).contains(QStringLiteral("警告")));
    auto *next = widget.findChild<QPushButton *>("nextBuildProblem");
    QVERIFY(next);
    next->click();
    QCOMPARE(jump.size(), 1);
    QVERIFY(jump.first().at(0).toString().endsWith("User/main.c"));
    QCOMPARE(jump.first().at(1).toInt(), 12);
    auto *output = widget.findChild<QTextBrowser *>();
    const auto warning = output->document()->find("unused variable");
    QVERIFY(!warning.isNull());
    QCOMPARE(warning.charFormat().foreground().color(), QColor("#f9e2af"));
}

void BuildExperienceWidgetTest::backendCountsLinkerFailures()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(startFixture(widget, temporary.path(), 3));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QVERIFY(!finished.first().first().toBool());
    QVERIFY(outputText(widget).contains(QStringLiteral("错误 1")));
    QVERIFY(outputText(widget).contains("undefined reference"));
}

void BuildExperienceWidgetTest::backendRejectsErrorWithZeroExit()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(startFixture(widget, temporary.path(), 2));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QVERIFY(!finished.first().first().toBool());
}

void BuildExperienceWidgetTest::reportsNoWorkAndCleanCorrectly()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(startFixture(widget, temporary.path(), 5));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QVERIFY(outputText(widget).contains(QStringLiteral("无需重新编译")));
    finished.clear();
    QVERIFY(widget.buildManager()->startClean(temporary.path()));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QVERIFY(outputText(widget).contains(QStringLiteral("清理完成")));
    QVERIFY(!outputText(widget).contains(QStringLiteral("构建成功")));
}

void BuildExperienceWidgetTest::cancelledBuildCannotSucceed()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(startFixture(widget, temporary.path(), 4));
    widget.buildManager()->stopBuild();
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QVERIFY(!finished.first().first().toBool());
    QVERIFY(outputText(widget).contains(QStringLiteral("终止")) || outputText(widget).contains(QStringLiteral("取消")));
}

void BuildExperienceWidgetTest::generatedRecipesAnnounceStages()
{
    const auto profile = STM32ProfileManager::instance().getProfile("STM32F429IGTx");
    const QString makefile = MakefileGenerator::generateMakefileContent(".", profile, {},
        BuildConfiguration::Debug, "flash.ld", {"Src/main.c"}, {}, {"startup.s"}, {}, {});
    for (const QString stage : {"CC", "AS", "LD", "HEX", "BIN"})
        QVERIFY2(makefile.contains("[BUILD:" + stage + "]"), qPrintable(stage));
    QVERIFY(makefile.contains("$(CC) -c $(CFLAGS) \"$<\" -o \"$@\""));
    QVERIFY(makefile.contains(".o: Src/main.c | $(BUILD_DIR)"));
}

void BuildExperienceWidgetTest::reportsStartupFailure()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    STM32WorkbenchWidget widget;
    widget.buildManager()->setChipProfile(STM32ProfileManager::instance().getProfile("STM32F429IGTx"));
    ToolchainPaths paths;
    paths.makePath = temporary.filePath("missing-make.exe");
    widget.buildManager()->setToolchainPaths(paths);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(!widget.buildManager()->startBuild(temporary.path()));
    QCOMPARE(finished.size(), 1);
    QVERIFY(!finished.first().first().toBool());
    QVERIFY(outputText(widget).contains(QStringLiteral("无法启动 make")));
    QVERIFY(outputText(widget).contains(QStringLiteral("构建失败")));
    QVERIFY(widget.findChild<QTreeWidget *>("buildProblems")->topLevelItemCount() > 0);
}

void BuildExperienceWidgetTest::rebuildHasOneFinalResult()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    widget.setProjectPath(temporary.path());
    qputenv("BUILD_OUTPUT_FIXTURE", "1");
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(widget.buildManager()->startRebuild(temporary.path(), BuildConfiguration::Debug, 2));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QVERIFY(finished.first().first().toBool());
    QVERIFY(outputText(widget).contains(QStringLiteral("警告 1")));
    QVERIFY(!outputText(widget).contains(QStringLiteral("清理完成")));
    QCOMPARE(widget.findChild<QTreeWidget *>("buildProblems")->topLevelItemCount(), 1);
}

void BuildExperienceWidgetTest::newBuildClearsOldProblems()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(startFixture(widget, temporary.path(), 2));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QCOMPARE(widget.findChild<QTreeWidget *>("buildProblems")->topLevelItemCount(), 2);
    finished.clear();
    QVERIFY(startFixture(widget, temporary.path(), 0));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QCOMPARE(widget.findChild<QTreeWidget *>("buildProblems")->topLevelItemCount(), 0);
    QVERIFY(!outputText(widget).contains("missing_symbol"));
}

void BuildExperienceWidgetTest::downloadViewSurvivesLateBuildOutput()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(startFixture(widget, temporary.path(), 1));
    widget.flashProgrammer()->flashStarted("OpenOCD");
    widget.flashProgrammer()->flashOutput("** Programming Started **\n** Programming Finished **\n", false);
    widget.flashProgrammer()->flashFinished(true, QStringLiteral("下载完成"));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    QVERIFY(outputText(widget).contains(QStringLiteral("下载完成")));
    QVERIFY(!outputText(widget).contains("compiling main.c"));
    QCOMPARE(widget.findChild<QTreeWidget *>("buildProblems")->topLevelItemCount(), 1);
}

void BuildExperienceWidgetTest::repeatedBuildClicksPreserveOutput()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QVERIFY(startFixture(widget, temporary.path(), 4));
    QTRY_VERIFY_WITH_TIMEOUT(outputText(widget).contains("compiling main.c"), 5000);
    const QString before = outputText(widget);
    widget.onBuildClicked();
    QCOMPARE(outputText(widget), before);
    widget.onRebuildClicked();
    QCOMPARE(outputText(widget), before);
    widget.buildManager()->stopBuild();
}

void BuildExperienceWidgetTest::decodesSplitChinesePathBytes()
{
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(startFixture(widget, temporary.path(), 6));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
    auto *problems = widget.findChild<QTreeWidget *>("buildProblems");
    QCOMPARE(problems->topLevelItemCount(), 1);
    QVERIFY2(problems->topLevelItem(0)->toolTip(1).contains(QStringLiteral("User/源.c")),
             qPrintable(problems->topLevelItem(0)->toolTip(1)));
}

void BuildExperienceWidgetTest::realCompilerBuildAndIncremental()
{
    const QString project = qEnvironmentVariable("STM32_WORKBENCH_F429_PROJECT");
    if (project.isEmpty()) QSKIP("Requires a disposable F429 project copy.");
    QTemporaryDir temporary;
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    settings.setValue("STM32Toolchain/selectedChip", "STM32F429IGTx");
    settings.setValue("STM32Toolchain/gccPath", qEnvironmentVariable("STM32_WORKBENCH_REAL_GCC"));
    settings.setValue("STM32Toolchain/makePath", qEnvironmentVariable("STM32_WORKBENCH_REAL_MAKE"));
    STM32WorkbenchWidget widget(nullptr, &settings);
    widget.setProjectPath(project);
    QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
    QVERIFY(widget.buildManager()->startBuild(project, BuildConfiguration::Debug, 4));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 120000);
    QVERIFY2(finished.first().first().toBool(), qPrintable(outputText(widget)));
    QCOMPARE(widget.buildManager()->logModel().errorCount(), 0);
    QVERIFY(outputText(widget).contains("compiling"));
    QVERIFY(outputText(widget).contains("linking"));
    QVERIFY(QFileInfo::exists(widget.buildManager()->getBinPath(project)));
    QFile firstLog(QDir(project).filePath("verified-full-build.log"));
    QVERIFY(firstLog.open(QIODevice::WriteOnly));
    for (const auto &entry : widget.buildManager()->logModel().detailedEntries())
        firstLog.write((entry.text + '\n').toUtf8());
    firstLog.close();
    finished.clear();
    QVERIFY(widget.buildManager()->startBuild(project, BuildConfiguration::Debug, 4));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 120000);
    QVERIFY(finished.first().first().toBool());
    QVERIFY(!outputText(widget).contains("compiling"));
    QVERIFY(outputText(widget).contains(QStringLiteral("无需重新编译")));
    QFile incremental(QDir(project).filePath("verified-incremental-build.log"));
    QVERIFY(incremental.open(QIODevice::WriteOnly));
    for (const auto &entry : widget.buildManager()->logModel().detailedEntries())
        incremental.write((entry.text + '\n').toUtf8());
}

void BuildExperienceWidgetTest::capturesPreviews()
{
    const QString folder = qEnvironmentVariable("BUILD_PREVIEW_DIR");
    if (folder.isEmpty()) QSKIP("Set BUILD_PREVIEW_DIR for visual verification.");
    QVERIFY(QDir().mkpath(folder));
    QTemporaryDir temporary;
    QVERIFY(prepareProject(temporary.path()));
    QSettings settings(temporary.filePath("settings.ini"), QSettings::IniFormat);
    configure(settings);
    STM32WorkbenchWidget widget(nullptr, &settings);
    widget.resize(1100, 680);
    widget.findChild<GdbDebugPanel *>()->hide();
    widget.show();
    for (int mode : {0, 1, 2}) {
        QSignalSpy finished(widget.buildManager(), &BuildManager::buildFinished);
        QVERIFY(startFixture(widget, temporary.path(), mode));
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 8000);
        QCoreApplication::processEvents();
        QVERIFY(widget.grab().save(folder + QString("/build-mode-%1.png").arg(mode)));
        if (mode == 2) {
            auto *tabs = widget.findChild<QTabWidget *>("buildOutputTabs");
            QVERIFY(tabs);
            tabs->setCurrentIndex(1);
            QCoreApplication::processEvents();
            QVERIFY(widget.grab().save(folder + "/build-problems.png"));
        }
    }
}

int main(int argc, char **argv)
{
    if (argc > 1 && (QByteArray(argv[1]).startsWith("-j") || QByteArray(argv[1]) == "clean") &&
        qEnvironmentVariableIsSet("BUILD_OUTPUT_FIXTURE")) {
        if (QByteArray(argv[1]) == "clean") { std::puts("cleaning build..."); return 0; }
        const int mode = qEnvironmentVariableIntValue("BUILD_OUTPUT_FIXTURE");
        if (mode == 6) {
            const QByteArray bytes = QStringLiteral("User/源.c:12:5: warning: split multibyte path\n").toLocal8Bit();
            std::fwrite(bytes.constData(), 1, 6, stderr);
            std::fflush(stderr);
            QThread::msleep(150);
            std::fwrite(bytes.constData() + 6, 1, size_t(bytes.size() - 6), stderr);
            std::fflush(stderr);
            return 0;
        }
        if (mode == 5) { std::puts("mingw32-make: Nothing to be done for 'all'."); return 0; }
        std::puts("[BUILD:CC] \"User/main.c\"\narm-none-eabi-gcc -c -mcpu=cortex-m4 User/main.c -o build/main.o");
        std::fflush(stdout);
        QThread::msleep(80);
        if (mode == 4) { QThread::msleep(5000); return 0; }
        if (mode == 1 || mode == 2) {
            std::fputs("\033[33mUser/main.c:12:5: warning: unused variable 'test' [-Wunused-variable]\033[0m\n   12 | int test;\n      |     ^~~~\n", stderr);
            std::fflush(stderr);
            QThread::msleep(80);
        }
        if (mode == 2) {
            std::fputs("User/main.c:18:9: error: 'missing_symbol' undeclared", stderr);
            std::fflush(stderr);
            return 0; // Deliberately contradictory zero exit and no final newline.
        }
        std::puts("[BUILD:LD] \"build/stm32f429igtx.elf\"");
        if (mode == 3) {
            std::fputs("arm-none-eabi-ld: build/main.o: undefined reference to `missing_symbol'\ncollect2: error: ld returned 1 exit status\nmingw32-make: *** [Makefile:61: build/app.elf] Error 1\n", stderr);
            return 1;
        }
        std::puts("   text    data     bss     dec     hex filename\n   6200      20    1024    7244    1c4c build/stm32f429igtx.elf\n[BUILD:HEX] \"build/stm32f429igtx.hex\"\n[BUILD:BIN] \"build/stm32f429igtx.bin\"");
        return 0;
    }
    QApplication app(argc, argv);
    BuildExperienceWidgetTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "BuildExperienceWidgetTest.moc"
