#include <QtTest>
#include <QTemporaryDir>
#include <QTcpServer>
#include "stm32debugcontroller.h"

class STM32DebugControllerTest : public QObject {
    Q_OBJECT
private slots:
    void configurationChangeReleasesIdleServer_data() {
        QTest::addColumn<int>("setting");
        QTest::newRow("chip") << 0;
        QTest::newRow("probe") << 1;
        QTest::newRow("flash-options") << 2;
        QTest::newRow("toolchain") << 3;
    }

    void configurationChangeReleasesIdleServer() {
        QFETCH(int, setting);
        STM32DebugController controller;
        QTemporaryDir project;
        controller.openProject(project.path());
        auto *server = controller.findChild<GdbServerManager *>();
        auto *process = server->findChild<QProcess *>();
        process->start("powershell.exe", {"-NoProfile", "-Command", "Start-Sleep -Seconds 60"});
        QVERIFY(process->waitForStarted());
        switch (setting) {
        case 0: controller.setTargetChip("STM32F407VG"); break;
        case 1: controller.setProbeName("J-Link"); break;
        case 2: controller.setFlashOptions(FlashOptions()); break;
        case 3: controller.setCustomOpenocdPath("Z:/offline/openocd.exe"); break;
        }
        QVERIFY2(!server->isRunning(), "Configuration change did not release the old probe session");
    }

    void projectSwitchReleasesServerEvenWhenSessionIsIdle() {
        STM32DebugController controller;
        QTemporaryDir first, second;
        controller.openProject(first.path());
        auto *server = controller.findChild<GdbServerManager *>();
        QVERIFY(server);
        auto *process = server->findChild<QProcess *>();
        QVERIFY(process);
        process->start("powershell.exe", {"-NoProfile", "-Command", "Start-Sleep -Seconds 60"});
        QVERIFY(process->waitForStarted());
        QCOMPARE(controller.state(), STM32SessionState::Idle);
        controller.openProject(second.path());
        QVERIFY2(!server->isRunning(), "Idle session left a debug server holding the probe");
    }

    void initTestCase() {
        // Invalid paths prevent these tests from accessing a hardware probe.
        m_controller = new STM32DebugController;
        ToolchainPaths paths;
        paths.gccPath = "Z:/smilecode-test-missing/gcc.exe";
        paths.makePath = "Z:/smilecode-test-missing/make.exe";
        paths.openocdPath = "Z:/smilecode-test-missing/openocd.exe";
        paths.jlinkPath = "Z:/smilecode-test-missing/JLink.exe";
        m_controller->setToolchainPaths(paths);
    }

    void repeatedProjectSelectionKeepsSession() {
        QTemporaryDir project;
        QVERIFY(project.isValid());
        m_controller->openProject(project.path());
        QVERIFY(QMetaObject::invokeMethod(m_controller, "onGdbTargetRunning"));
        m_controller->openProject(project.path());
        QCOMPARE(m_controller->state(), STM32SessionState::Running);
    }

    void cleaningIsRejectedWhileDebugging() {
        QTemporaryDir project;
        m_controller->openProject(project.path());
        QVERIFY(QMetaObject::invokeMethod(m_controller, "onGdbTargetRunning"));
        QVERIFY(!m_controller->cleanProject());
        QCOMPARE(m_controller->state(), STM32SessionState::Running);
    }

    void flashSignalsAreConnectedBeforeFirstAttempt() {
        auto programmer = m_controller->findChild<FlashProgrammer *>();
        QVERIFY(programmer);
        QSignalSpy output(m_controller, &STM32DebugController::flashOutput);
        QSignalSpy finished(m_controller, &STM32DebugController::flashFinished);
        emit programmer->flashOutput("preflight error", true);
        QCOMPARE(output.count(), 1);
        QCOMPARE(output.first().first().toString(), QString("preflight error"));
        emit programmer->flashFinished(false, "failed");
        emit programmer->flashFinished(true, "done");
        QCOMPARE(finished.count(), 2);
    }

    void projectSwitchClearsBreakpoints() {
        QTemporaryDir first;
        QTemporaryDir second;
        m_controller->openProject(first.path());
        m_controller->addBreakpoint(first.filePath("main.c"), 12);
        QVERIFY(!m_controller->allBreakpoints().isEmpty());
        m_controller->openProject(second.path());
        QVERIFY(m_controller->allBreakpoints().isEmpty());
    }

    void jlinkUsesNativeDriver() {
        QTemporaryDir project;
        m_controller->openProject(project.path());
        m_controller->setProbeType(ProbeType::JLink);
        auto builder = m_controller->findChild<BuildManager *>();
        auto programmer = m_controller->findChild<FlashProgrammer *>();
        QVERIFY(builder && programmer);
        const QString firmware = builder->getBinPath(project.path());
        QVERIFY(QDir().mkpath(QFileInfo(firmware).absolutePath()));
        QFile file(firmware);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("test firmware");
        file.close();
        FlashDriverType driver = FlashDriverType::OpenOCD_STLink;
        const auto connection = connect(programmer, &FlashProgrammer::flashSessionStarted,
            this, [&driver](const QString &, const FlashOptions &options) { driver = options.driver; });
        QVERIFY(!m_controller->flashProject());
        disconnect(connection);
        QCOMPARE(driver, FlashDriverType::JLink_Native);
        QCOMPARE(m_controller->state(), STM32SessionState::Idle);
    }

    void stopIsIdempotentAndDoesNotRecurse() {
        QSignalSpy ended(m_controller, &STM32DebugController::debugSessionEnded);
        m_controller->stopDebug();
        QCOMPARE(ended.count(), 0);
        QVERIFY(QMetaObject::invokeMethod(m_controller, "onGdbTargetRunning"));
        m_controller->stopDebug();
        QCOMPARE(ended.count(), 1);
        m_controller->stopDebug();
        QCOMPARE(ended.count(), 1);
        QCOMPARE(m_controller->state(), STM32SessionState::Idle);
    }

    void cancelledDebugAndCloseClearProject() {
        QTemporaryDir project;
        m_controller->openProject(project.path());
        QVERIFY(QMetaObject::invokeMethod(m_controller, "onGdbTargetRunning"));
        m_controller->cancelCurrentTask();
        QCOMPARE(m_controller->state(), STM32SessionState::Idle);
        QVERIFY(QMetaObject::invokeMethod(m_controller, "onGdbTargetRunning"));
        m_controller->closeProject();
        QVERIFY(!m_controller->hasProject());
        QCOMPARE(m_controller->state(), STM32SessionState::Idle);
    }

    void buildFailureReturnsToIdle() {
        QTemporaryDir project;
        m_controller->openProject(project.path());
        QVERIFY(!m_controller->buildProject());
        QCOMPARE(m_controller->state(), STM32SessionState::Idle);
        QVERIFY(!m_controller->rebuildProject());
        QCOMPARE(m_controller->state(), STM32SessionState::Idle);
    }

    void targetCannotChangeDuringDebug() {
        QTemporaryDir project;
        m_controller->openProject(project.path());
        m_controller->setProbeName("ST-Link");
        const auto chip = m_controller->targetChip();
        QVERIFY(QMetaObject::invokeMethod(m_controller, "onGdbTargetRunning"));
        m_controller->setTargetChip("STM32F407VG");
        m_controller->setProbeName("J-Link");
        QCOMPARE(m_controller->targetChip(), chip);
        QCOMPARE(m_controller->probeName(), QString("ST-Link"));
    }

    void projectProbeRestoresWithoutBeingOverwritten() {
        QTemporaryDir first;
        QTemporaryDir second;
        m_controller->openProject(first.path());
        m_controller->setProbeName("CMSIS-DAP");
        m_controller->openProject(second.path());
        m_controller->setProbeName("J-Link");
        QSignalSpy changed(m_controller, &STM32DebugController::probeChanged);
        m_controller->openProject(first.path());
        QCOMPARE(m_controller->probeName(), QString("CMSIS-DAP"));
        QCOMPARE(changed.last().first().toString(), QString("CMSIS-DAP"));
        QFile config(first.filePath(".smilecode/project.json"));
        QVERIFY(config.open(QIODevice::ReadOnly));
        QCOMPARE(QJsonDocument::fromJson(config.readAll()).object().value("probe").toString(),
                 QString("CMSIS-DAP"));
    }

    void cancellingBuildStopsProcessAndRejectsOtherTasks() {
        QTemporaryDir project;
        QFile source(project.filePath("main.c"));
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("int main(void) { return 0; }\n");
        source.close();
        QFile linker(project.filePath("test.ld"));
        QVERIFY(linker.open(QIODevice::WriteOnly));
        linker.write("SECTIONS {}\n");
        linker.close();
        m_controller->openProject(project.path());
        auto paths = m_controller->toolchainPaths();
        paths.makePath = QCoreApplication::applicationFilePath();
        m_controller->setToolchainPaths(paths);
        QVERIFY(m_controller->buildProject());
        QCOMPARE(m_controller->state(), STM32SessionState::Building);
        QVERIFY(!m_controller->cleanProject());
        QVERIFY(!m_controller->flashProject());
        QVERIFY(!m_controller->startDebug());
        m_controller->cancelCurrentTask();
        QCOMPARE(m_controller->state(), STM32SessionState::Idle);
        QVERIFY(!m_controller->findChild<BuildManager *>()->isBusy());
    }

    void cancellingFlashAndSwitchingProjectStopsProcess() {
        QTemporaryDir project;
        QTemporaryDir next;
        m_controller->openProject(project.path());
        m_controller->setProbeName("J-Link");
        auto paths = m_controller->toolchainPaths();
        paths.jlinkPath = QCoreApplication::applicationFilePath();
        m_controller->setToolchainPaths(paths);
        const auto builder = m_controller->findChild<BuildManager *>();
        const auto programmer = m_controller->findChild<FlashProgrammer *>();
        const QString firmware = builder->getBinPath(project.path());
        QVERIFY(QDir().mkpath(QFileInfo(firmware).absolutePath()));
        QFile file(firmware);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("test firmware");
        file.close();
        QSignalSpy finished(m_controller, &STM32DebugController::flashFinished);
        for (int attempt = 0; attempt < 2; ++attempt) {
            QVERIFY(m_controller->flashProject());
            QCOMPARE(m_controller->state(), STM32SessionState::Flashing);
            QVERIFY(!m_controller->buildProject());
            m_controller->cancelCurrentTask();
            QCOMPARE(m_controller->state(), STM32SessionState::Idle);
            QVERIFY(!programmer->isBusy());
            QCOMPARE(finished.count(), attempt + 1);
        }
        QVERIFY(m_controller->flashProject());
        m_controller->openProject(next.path());
        QVERIFY(!programmer->isBusy());
        QCOMPARE(m_controller->state(), STM32SessionState::Idle);
        QCOMPARE(m_controller->currentProjectDir(), QFileInfo(next.path()).canonicalFilePath());
    }

    void cleanupTestCase() {
        delete m_controller;
        QVERIFY(!STM32DebugController::instance());
    }

    void projectFlashConfigurationIsIsolatedAndPreservesUnknownFields() {
        QTemporaryDir first;
        QTemporaryDir second;
        m_controller->openProject(first.path());
        FlashOptions firstOptions;
        firstOptions.driver = FlashDriverType::OpenOCD_CMSISDAP;
        firstOptions.targetAddress = "0x08004000";
        firstOptions.swdSpeedKhz = 1200;
        firstOptions.probeSerial = "probe-one";
        firstOptions.verify = false;
        firstOptions.resetAndRun = false;
        firstOptions.customOpenocdInterfaceCfg = "custom/interface.cfg";
        firstOptions.customOpenocdTargetCfg = "custom/target.cfg";
        m_controller->setFlashOptions(firstOptions);
        QFile config(first.filePath(".smilecode/project.json"));
        QVERIFY(config.open(QIODevice::ReadOnly));
        auto json = QJsonDocument::fromJson(config.readAll()).object();
        config.close();
        json["customProjectField"] = "preserve-me";
        QVERIFY(config.open(QIODevice::WriteOnly | QIODevice::Truncate));
        config.write(QJsonDocument(json).toJson());
        config.close();
        m_controller->openProject(second.path());
        FlashOptions secondOptions;
        secondOptions.driver = FlashDriverType::JLink_Native;
        secondOptions.targetAddress = "0x08008000";
        m_controller->setFlashOptions(secondOptions);
        m_controller->openProject(first.path());
        const auto loaded = m_controller->flashOptions();
        QCOMPARE(loaded.driver, firstOptions.driver);
        QCOMPARE(loaded.targetAddress, firstOptions.targetAddress);
        QCOMPARE(loaded.swdSpeedKhz, firstOptions.swdSpeedKhz);
        QCOMPARE(loaded.probeSerial, firstOptions.probeSerial);
        QCOMPARE(loaded.verify, false);
        QCOMPARE(loaded.resetAndRun, false);
        QCOMPARE(loaded.customOpenocdTargetCfg, firstOptions.customOpenocdTargetCfg);
        QVERIFY(config.open(QIODevice::ReadOnly));
        QCOMPARE(QJsonDocument::fromJson(config.readAll()).object().value("customProjectField").toString(),
                 QString("preserve-me"));
    }

    void debugServerReceivesSelectedProbeAndScripts() {
        GdbServerManager server;
        ToolchainPaths paths;
        paths.openocdPath = QCoreApplication::applicationFilePath();
        server.setToolchainPaths(paths);
        QTcpServer portReservation;
        QVERIFY(portReservation.listen(QHostAddress::LocalHost, 0));
        const int port = portReservation.serverPort();
        portReservation.close();
        QVERIFY(server.startServer(GdbServerType::OpenOCD, "custom interface.cfg", "custom target.cfg",
                                   port, "probe-test-123", 1800));
        const auto process = server.findChild<QProcess *>();
        QVERIFY(process);
        const auto args = process->arguments();
        QVERIFY(args.contains("custom interface.cfg"));
        QVERIFY(args.contains("custom target.cfg"));
        QVERIFY(args.contains("adapter serial {probe-test-123}"));
        QVERIFY(args.contains("adapter speed 1800"));
        server.stopServer();
    }

private:
    STM32DebugController *m_controller = nullptr;
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    // The test binary also serves as a harmless, bounded fake make/J-Link process.
    if (app.arguments().contains("-j4") || app.arguments().contains("-CommandFile") ||
        app.arguments().contains("custom interface.cfg")) {
        QTimer::singleShot(15000, &app, &QCoreApplication::quit);
        return app.exec();
    }
    STM32DebugControllerTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "STM32DebugControllerTest.moc"
