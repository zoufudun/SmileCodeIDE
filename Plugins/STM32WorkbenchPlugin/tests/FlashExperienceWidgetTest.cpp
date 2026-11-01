#include <QtTest>
#include <QCheckBox>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QTextCursor>
#include <QThread>
#include <cstdio>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "../UI/STM32WorkbenchWidget.h"
#include "../UI/FlashConfigDialog.h"

class FlashExperienceWidgetTest : public QObject {
    Q_OBJECT
private slots:
    void informationalStderrIsNeutral();
    void conciseSuccessCanExpandDetails();
    void realErrorsRemainVisible();
    void buildDiagnosticsStayClickable();
    void lateFlashOutputPreservesBuildDiagnostics();
    void backendClassifiesBothStreams_data();
    void backendClassifiesBothStreams();
    void cancelledDownloadCannotSucceed();
    void switchingProjectReleasesDebugServer();
    void downloadReleasesDebugServerBeforeStarting();
    void abnormalHostExitReleasesDebugServer();
    void backendDownloadReleasesOtherWorkbenchServer();
    void concurrentDownloadsAreRejected();
    void serverConfigurationChangeReleasesProbe();
    void capturesPreviews();
};

static bool startIdleServer(GdbServerManager *server)
{
    auto *process = server->findChild<QProcess *>();
    if (!process) return false;
    process->start(QStringLiteral("powershell.exe"),
                   {QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
                    QStringLiteral("Start-Sleep -Seconds 60")});
    return process->waitForStarted();
}

static void configureOfflineProgrammer(FlashProgrammer &programmer)
{
    ToolchainPaths paths;
    paths.jlinkPath = QCoreApplication::applicationFilePath();
    programmer.setToolchainPaths(paths);
    programmer.setChipProfile(STM32ProfileManager::instance().getProfile("STM32F407VG"));
}

void FlashExperienceWidgetTest::backendDownloadReleasesOtherWorkbenchServer()
{
    GdbServerManager otherWorkbench;
    QVERIFY(startIdleServer(&otherWorkbench));
    FlashProgrammer programmer;
    configureOfflineProgrammer(programmer);
    QTemporaryFile firmware;
    QVERIFY(firmware.open());
    FlashOptions options;
    options.driver = FlashDriverType::JLink_Native;
    QVERIFY(programmer.flashFirmware(firmware.fileName(), options));
    QVERIFY2(!otherWorkbench.isRunning(), "Common download backend did not release another workbench's server");
    programmer.cancelFlash();
}

void FlashExperienceWidgetTest::concurrentDownloadsAreRejected()
{
    FlashProgrammer first, second;
    configureOfflineProgrammer(first);
    configureOfflineProgrammer(second);
    QTemporaryFile firmware;
    QVERIFY(firmware.open());
    FlashOptions options;
    options.driver = FlashDriverType::JLink_Native;
    QVERIFY(first.flashFirmware(firmware.fileName(), options));
    const bool competing = second.flashFirmware(firmware.fileName(), options);
    second.cancelFlash();
    first.cancelFlash();
    QVERIFY2(!competing, "Two download processes were allowed to access the hardware simultaneously");
    QVERIFY(second.flashFirmware(firmware.fileName(), options));
    second.cancelFlash();
}

void FlashExperienceWidgetTest::serverConfigurationChangeReleasesProbe()
{
    GdbServerManager server;
    QVERIFY(startIdleServer(&server));
    server.setChipProfile(STM32ProfileManager::instance().getProfile("STM32H743IITx"));
    QVERIFY2(!server.isRunning(), "Chip change kept the old server running");
    QVERIFY(startIdleServer(&server));
    server.setToolchainPaths(ToolchainPaths());
    QVERIFY2(!server.isRunning(), "Toolchain change kept the old server running");
}

void FlashExperienceWidgetTest::abnormalHostExitReleasesDebugServer()
{
#ifdef Q_OS_WIN
    QProcess host;
    host.start(QCoreApplication::applicationFilePath(), {"--idle-server-host", "-platform", "offscreen"});
    QVERIFY(host.waitForStarted());
    QByteArray output;
    QElapsedTimer timeout;
    timeout.start();
    while (!output.contains('\n') && timeout.elapsed() < 10000) {
        host.waitForReadyRead(200);
        output += host.readAllStandardOutput();
    }
    bool valid = false;
    const DWORD pid = output.trimmed().toULong(&valid);
    QVERIFY2(valid, output.constData());
    HANDLE server = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);
    QVERIFY(server);
    host.kill();
    QVERIFY(host.waitForFinished());
    const DWORD result = WaitForSingleObject(server, 5000);
    if (result != WAIT_OBJECT_0) TerminateProcess(server, 1);
    CloseHandle(server);
    QCOMPARE(result, DWORD(WAIT_OBJECT_0));
#else
    QSKIP("Windows process ownership regression");
#endif
}

void FlashExperienceWidgetTest::switchingProjectReleasesDebugServer()
{
    STM32WorkbenchWidget widget;
    QTemporaryDir first, second;
    widget.setProjectPath(first.path());
    auto *server = widget.findChild<GdbServerManager *>();
    QVERIFY(server);
    QVERIFY(startIdleServer(server));
    widget.setProjectPath(first.path());
    QVERIFY(server->isRunning());
    widget.setProjectPath(second.path());
    QVERIFY2(!server->isRunning(), "Previous project's server still owns the probe");
}

void FlashExperienceWidgetTest::downloadReleasesDebugServerBeforeStarting()
{
    STM32WorkbenchWidget widget;
    QTemporaryDir project;
    widget.setProjectPath(project.path());
    // Keep this lifecycle test offline: no configured tool can access hardware.
    widget.flashProgrammer()->setToolchainPaths(ToolchainPaths());
    const QString bin = widget.buildManager()->getBinPath(project.path());
    QVERIFY(QDir().mkpath(QFileInfo(bin).absolutePath()));
    QFile firmware(bin);
    QVERIFY(firmware.open(QIODevice::WriteOnly));
    firmware.write("test");
    firmware.close();
    auto *server = widget.findChild<GdbServerManager *>();
    QVERIFY(server);
    QVERIFY(startIdleServer(server));
    bool started = false;
    bool released = false;
    connect(widget.flashProgrammer(), &FlashProgrammer::flashSessionStarted,
            &widget, [&] { started = true; released = !server->isRunning(); });
    widget.onFlashClicked();
    QVERIFY(started);
    QVERIFY2(released, "Download started before debug server released the probe");
}

static QString successLog()
{
    return QStringLiteral("Open On-Chip Debugger 0.12.0\nLicensed under GNU GPL v2\n"
        "Info : STLINK V2J46S7 (API v2) VID:PID 0483:3748\n"
        "Info : Target voltage: 3.244622\nInfo : [stm32f4x.cpu] Examination succeed\n"
        "Info : Listening on port 3333 for gdb connections\n"
        "** Programming Started **\n** Programming Finished **\n"
        "** Verify Started **\n** Verified OK **\n** Resetting Target **\nshutdown command invoked\n");
}

static QTextBrowser *console(STM32WorkbenchWidget &widget)
{
    return widget.findChild<QTextBrowser *>();
}

static void replay(STM32WorkbenchWidget &widget, const QString &log, bool success)
{
    widget.flashProgrammer()->flashStarted("OpenOCD");
    widget.flashProgrammer()->flashOutput(log, true);
    widget.flashProgrammer()->flashFinished(success, success ? QStringLiteral("烧录成功完成")
                                                            : QStringLiteral("烧录失败，退出代码: 1"));
}

void FlashExperienceWidgetTest::informationalStderrIsNeutral()
{
    STM32WorkbenchWidget widget;
    replay(widget, "Info : clock speed 4000 kHz\n", true);
    if (auto *details = widget.findChild<QCheckBox *>("flashDetailedLog")) details->setChecked(true);
    auto *output = console(widget);
    QVERIFY(output);
    const QTextCursor info = output->document()->find("Info : clock speed 4000 kHz");
    QVERIFY(!info.isNull());
    QVERIFY(info.charFormat().foreground().color() != QColor("#f38ba8"));
}

void FlashExperienceWidgetTest::conciseSuccessCanExpandDetails()
{
    STM32WorkbenchWidget widget;
    replay(widget, successLog(), true);
    auto *output = console(widget);
    QVERIFY(output);
    QVERIFY(output->toPlainText().contains(QStringLiteral("编程完成")));
    QVERIFY(output->toPlainText().contains(QStringLiteral("校验通过")));
    QVERIFY(output->toPlainText().contains(QStringLiteral("下载完成")));
    QVERIFY(!output->toPlainText().contains("GNU GPL"));
    auto *details = widget.findChild<QCheckBox *>("flashDetailedLog");
    QVERIFY(details);
    details->setChecked(true);
    QVERIFY(output->toPlainText().contains("GNU GPL"));
    QVERIFY(output->toPlainText().contains("Listening on port"));
    const QTextCursor verified = output->document()->find("** Verified OK **");
    QVERIFY(!verified.isNull());
    QCOMPARE(verified.charFormat().foreground().color(), QColor("#a6e3a1"));
    details->setChecked(false);
    QVERIFY(output->toPlainText().contains(QStringLiteral("校验通过")));
}

void FlashExperienceWidgetTest::realErrorsRemainVisible()
{
    STM32WorkbenchWidget widget;
    replay(widget, "Info : clock speed 4000 kHz\nError: open failed\n", false);
    auto *output = console(widget);
    QVERIFY(output);
    const QTextCursor error = output->document()->find("Error: open failed");
    QVERIFY(!error.isNull());
    QCOMPARE(error.charFormat().foreground().color(), QColor("#f38ba8"));
    QVERIFY(!output->toPlainText().contains(QStringLiteral("下载完成")));
}

void FlashExperienceWidgetTest::buildDiagnosticsStayClickable()
{
    STM32WorkbenchWidget widget;
    widget.buildManager()->buildStarted(QStringLiteral("编译构建"));
    widget.buildManager()->buildOutput("User/main.c:12:4: error: compiler diagnostic\n", true);
    auto *output = console(widget);
    QVERIFY(output);
    const QTextCursor error = output->document()->find("compiler diagnostic");
    QVERIFY(!error.isNull());
    QVERIFY(error.charFormat().isAnchor());
    QVERIFY(error.charFormat().anchorHref().contains("line=12"));
}

void FlashExperienceWidgetTest::lateFlashOutputPreservesBuildDiagnostics()
{
    STM32WorkbenchWidget widget;
    widget.flashProgrammer()->flashStarted("OpenOCD");
    widget.flashProgrammer()->flashOutput("** Programming Started **\n", true);
    widget.buildManager()->buildStarted(QStringLiteral("编译构建"));
    widget.buildManager()->buildOutput("User/main.c:12:4: error: compiler diagnostic\n", true);
    widget.flashProgrammer()->flashOutput("** Programming Finished **\n", true);
    widget.flashProgrammer()->flashFinished(true, QStringLiteral("下载完成"));
    const QTextCursor diagnostic = console(widget)->document()->find("compiler diagnostic");
    QVERIFY(!diagnostic.isNull());
    QVERIFY(diagnostic.charFormat().isAnchor());
    QVERIFY(!widget.findChild<QCheckBox *>("flashDetailedLog")->isEnabled());
}

void FlashExperienceWidgetTest::backendClassifiesBothStreams_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("successful-stderr") << 0;
    QTest::newRow("error-with-zero-exit") << 1;
}

void FlashExperienceWidgetTest::backendClassifiesBothStreams()
{
    QFETCH(int, mode);
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString path = temporary.filePath("firmware.bin");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("fixture");
    file.close();
    FlashProgrammer programmer;
    ToolchainPaths paths;
    // Run this test executable as a harmless fake downloader, never a hardware tool.
    paths.stm32ProgPath = QCoreApplication::applicationFilePath();
    programmer.setToolchainPaths(paths);
    programmer.setChipProfile(STM32ProfileManager::instance().getProfile("STM32F429IGTx"));
    QSignalSpy output(&programmer, &FlashProgrammer::flashOutput);
    QSignalSpy finished(&programmer, &FlashProgrammer::flashFinished);
    bool prematureCompletion = false;
    connect(&programmer, &FlashProgrammer::flashProgress, this, [&](int value, const QString &) {
        if (value == 100 && programmer.isBusy()) prematureCompletion = true;
    });
    qputenv("FLASH_OUTPUT_FIXTURE", QByteArray::number(mode));
    FlashOptions options;
    options.driver = FlashDriverType::STM32CubeProg;
    QVERIFY(programmer.flashFirmware(path, options));
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
    qunsetenv("FLASH_OUTPUT_FIXTURE");
    QCOMPARE(finished.first().first().toBool(), mode == 0);
    QVERIFY(!prematureCompletion);
    bool sawInfo = false, sawError = false;
    for (const auto &arguments : output) {
        const QString line = arguments.at(0).toString();
        if (line.contains("Info : clock speed")) {
            sawInfo = true;
            QVERIFY(!arguments.at(1).toBool());
            QVERIFY(line.contains("4000 kHz"));
        }
        if (line.contains("Error: simulated failure")) {
            sawError = true;
            QVERIFY(arguments.at(1).toBool());
        }
    }
    QVERIFY(sawInfo);
    QCOMPARE(sawError, mode == 1);
}

void FlashExperienceWidgetTest::cancelledDownloadCannotSucceed()
{
    FlashProgrammer programmer;
    QSignalSpy finished(&programmer, &FlashProgrammer::flashFinished);
    // Start only the local fixture process; cancelFlash must report failure even
    // if a tool handles termination by returning exit code zero.
    QProcess *process = programmer.findChild<QProcess *>();
    QVERIFY(process);
    process->start(QCoreApplication::applicationFilePath(), {"-c", "port=SWD"});
    QVERIFY(process->waitForStarted());
    programmer.cancelFlash();
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 5000);
    QVERIFY(!finished.first().first().toBool());
}

void FlashExperienceWidgetTest::capturesPreviews()
{
    const QString folder = qEnvironmentVariable("FLASH_PREVIEW_DIR");
    if (folder.isEmpty()) QSKIP("Set FLASH_PREVIEW_DIR to capture UI previews.");
    QVERIFY(QDir().mkpath(folder));
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QSettings settings(temporary.filePath("ui.ini"), QSettings::IniFormat);
    settings.setValue("STM32Toolchain/selectedChip", "STM32F429IGTx");
    FlashOptions options;
    for (const auto &probe : ProbeDetector::scan()) {
        if (probe.type != ProbeType::STLink) continue;
        settings.setValue("STM32Flash/probeInstanceId", probe.instanceId);
        options.probeSerial = probe.serial;
        break;
    }
    const QString firmware = temporary.filePath("stm32f429igtx.bin");
    QFile file(firmware);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(QByteArray(6220, '\0')), qint64(6220));
    file.close();
    STM32WorkbenchWidget widget(nullptr, &settings);
    widget.resize(1100, 680);
    widget.findChild<GdbDebugPanel *>()->hide();
    widget.show();
    widget.flashProgrammer()->flashSessionStarted(firmware, options);
    replay(widget, successLog(), true);
    QCoreApplication::processEvents();
    QVERIFY(widget.grab().save(folder + "/download-concise.png"));
    auto *details = widget.findChild<QCheckBox *>("flashDetailedLog");
    QVERIFY(details);
    details->setChecked(true);
    QCoreApplication::processEvents();
    QVERIFY(widget.grab().save(folder + "/download-detailed.png"));
    details->setChecked(false);
    replay(widget, "Info : clock speed 4000 kHz\nError: No device matches the serial string\n", false);
    QCoreApplication::processEvents();
    QVERIFY(widget.grab().save(folder + "/download-error.png"));
    FlashConfigDialog dialog(nullptr, &settings,
        STM32ProfileManager::instance().getProfile("STM32F429IGTx"));
    dialog.show();
    QCoreApplication::processEvents();
    QVERIFY(dialog.grab().save(folder + "/probe-serial.png"));
}

int main(int argc, char **argv)
{
    if (argc > 2 && QByteArray(argv[1]) == "-c" && QByteArray(argv[2]) == "port=SWD") {
        std::fputs("Info : clock ", stderr);
        std::fflush(stderr);
        QThread::msleep(100);
        std::fputs("speed 4000 kHz\n** Programming Started **\n", stderr);
        std::fflush(stderr);
        QThread::msleep(100);
        std::fputs("** Programming Finished **\n", stdout);
        std::fflush(stdout);
        QThread::msleep(100);
        std::fputs("** Verified OK **\n", stderr);
        if (qEnvironmentVariableIntValue("FLASH_OUTPUT_FIXTURE") == 1)
            std::fputs("Error: simulated failure", stderr); // Deliberately no final newline.
        std::fflush(stderr);
        QThread::msleep(100);
        return 0;
    }
    QApplication app(argc, argv);
    if (app.arguments().contains("-CommandFile")) {
        return app.exec(); // Fake J-Link tool: lifecycle tests never access hardware.
    }
    if (app.arguments().contains("--idle-server-host")) {
        GdbServerManager server;
        if (!startIdleServer(&server)) return 2;
        std::printf("%lld\n", static_cast<long long>(server.findChild<QProcess *>()->processId()));
        std::fflush(stdout);
        return app.exec();
    }
    FlashExperienceWidgetTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "FlashExperienceWidgetTest.moc"
