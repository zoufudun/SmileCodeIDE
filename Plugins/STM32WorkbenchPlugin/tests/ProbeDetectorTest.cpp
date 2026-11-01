#include <QtTest>
#include "../Core/ProbeDetector.h"
#include "../Core/FlashProgrammer.h"
#include "../UI/FlashConfigDialog.h"
#include <QTemporaryDir>
#include <QStandardPaths>

class ProbeDetectorTest : public QObject {
    Q_OBJECT
private slots:
    void recognizesCommonUsbProbes();
    void ignoresUnrelatedUsbDevices();
    void parsesCompositeDeviceSerial();
    void ignoresTruncatedLegacyStLinkSerial();
    void readsActualSerialOfAttachedLegacyStLink();
    void decodesStLinkUsbSerialDescriptors();
    void preservesManualSerialForLegacyStLink_data();
    void preservesManualSerialForLegacyStLink();
    void selectsProbeInFlashCommands();
    void preservesSelectedOpenOcdSpeedAfterTargetConfig();
    void preservesOpenOcdFirmwarePath_data();
    void preservesOpenOcdFirmwarePath();
    void rejectsIncompatibleDriverSelection();
    void scansPresentUsbDevicesWithoutDuplicates();
    void restoresSavedProbeSelection();
    void rejectsUnsafeFlashAddresses();
    void autoSelectsSingleConnectedProbe();
    void generatesValidJLinkDownloadScript();
    void validatesOpenOcdProbeScripts();
    void allowsOnlyOneMatchingSeriallessProbe();
    void resolvesOpenOcdScriptsFromBothLayouts();
    void defaultsFlashAddressPerCore();
    void refusesFlashWithoutSelectedChip();
};

void ProbeDetectorTest::recognizesCommonUsbProbes()
{
    QCOMPARE(ProbeDetector::classify("USB\\VID_0483&PID_374B", "ST-LINK/V2-1"), ProbeType::STLink);
    QCOMPARE(ProbeDetector::classify("USB\\VID_0483&PID_3757", "STLINK-V3"), ProbeType::STLink);
    QCOMPARE(ProbeDetector::classify("USB\\VID_1366&PID_0105", "J-Link"), ProbeType::JLink);
    QCOMPARE(ProbeDetector::classify("USB\\VID_0D28&PID_0204", "DAPLink CMSIS-DAP"), ProbeType::CmsisDap);
    QCOMPARE(ProbeDetector::classify("USB\\VID_C251&PID_F001", "CMSIS-DAP v2"), ProbeType::CmsisDap);
}

void ProbeDetectorTest::defaultsFlashAddressPerCore()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QSettings settings(temporary.filePath("flash.ini"), QSettings::IniFormat);
    settings.setValue("STM32Flash/targetAddr", "0x08000000");
    STM32ChipProfile profile;
    profile.chipName = "STM32H745ZITx";
    profile.linkerContext = "CM4";
    profile.flashOrigin = "0x08100000";
    FlashConfigDialog cm4(nullptr, &settings, profile);
    QCOMPARE(cm4.getOptions().targetAddress, QString("0x08100000"));
    settings.setValue("STM32Flash/targetAddrByChip/STM32H745ZITx@CM4", "0x08120000");
    FlashConfigDialog custom(nullptr, &settings, profile);
    QCOMPARE(custom.getOptions().targetAddress, QString("0x08120000"));
    profile.linkerContext = "CM7";
    profile.flashOrigin = "0x08000000";
    FlashConfigDialog cm7(nullptr, &settings, profile);
    QCOMPARE(cm7.getOptions().targetAddress, QString("0x08000000"));
}

void ProbeDetectorTest::refusesFlashWithoutSelectedChip()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString firmware = temporary.filePath("firmware.bin");
    QFile file(firmware);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("firmware");
    file.close();
    FlashProgrammer programmer;
    QSignalSpy output(&programmer, &FlashProgrammer::flashOutput);
    FlashOptions options;
    options.driver = FlashDriverType::STM32CubeProg;
    QVERIFY(!programmer.flashFirmware(firmware, options));
    QVERIFY(!output.isEmpty());
    QVERIFY(output.first().at(0).toString().contains(QStringLiteral("目标芯片")));
}

void ProbeDetectorTest::ignoresUnrelatedUsbDevices()
{
    QCOMPARE(ProbeDetector::classify("USB\\VID_0483&PID_DF11", "STM32 BOOTLOADER"), ProbeType::Unknown);
    QCOMPARE(ProbeDetector::classify("USB\\VID_1234&PID_5678", "USB keyboard"), ProbeType::Unknown);
}

void ProbeDetectorTest::parsesCompositeDeviceSerial()
{
    QCOMPARE(ProbeDetector::serialFromInstanceId("USB\\VID_0483&PID_374B\\066EFF313736504157095133"), QString("066EFF313736504157095133"));
    QCOMPARE(ProbeDetector::serialFromInstanceId("USB\\VID_0483&PID_374B&MI_00\\7&12345&0&0000"), QString());
    QCOMPARE(ProbeDetector::serialFromInstanceId("USB\\VID_0483&PID_374B&MI_00\\CHILD123"), QString());
}

void ProbeDetectorTest::ignoresTruncatedLegacyStLinkSerial()
{
    QCOMPARE(ProbeDetector::serialFromInstanceId("USB\\VID_0483&PID_3748\\C"), QString());
    QCOMPARE(ProbeDetector::serialFromInstanceId("USB\\VID_0483&PID_3748\\63003700090000383036544E"),
             QString("63003700090000383036544E"));
    QCOMPARE(ProbeDetector::serialFromInstanceId("USB\\VID_1366&PID_0105\\123456789"),
             QString("123456789"));
}

void ProbeDetectorTest::readsActualSerialOfAttachedLegacyStLink()
{
    for (const ProbeInfo &probe : ProbeDetector::scan()) {
        if (probe.type != ProbeType::STLink || probe.productId != "3748" ||
            !probe.instanceId.endsWith("\\C", Qt::CaseInsensitive)) continue;
        QVERIFY2(QRegularExpression("^[0-9A-Fa-f]{24}$").match(probe.serial).hasMatch(),
                 qPrintable("Expected actual USB serial, got: " + probe.serial));
        const QString expected = qEnvironmentVariable("STLINK_EXPECTED_SERIAL");
        if (!expected.isEmpty()) QCOMPARE(probe.serial, expected);
        return;
    }
    QSKIP("Requires the legacy ST-Link with truncated Windows instance C.");
}

void ProbeDetectorTest::preservesManualSerialForLegacyStLink_data()
{
    QTest::addColumn<QString>("savedSerial");
    QTest::addColumn<QString>("expectedSerial");
    QTest::newRow("discard-truncated-instance-serial") << QString("C") << QString();
    QTest::newRow("preserve-verified-manual-serial")
        << QString("63003700090000383036544E") << QString("63003700090000383036544E");
}

void ProbeDetectorTest::decodesStLinkUsbSerialDescriptors()
{
    const QByteArray binary = QByteArray::fromHex("63003700090000383036544e");
    QByteArray legacy;
    legacy.append(char(26));
    legacy.append(char(3));
    for (char value : binary) { legacy.append(value); legacy.append('\0'); }
    QCOMPARE(ProbeDetector::stLinkSerialFromUsbDescriptor(legacy),
             QString("63003700090000383036544E"));
    QByteArray standard;
    standard.append(char(50));
    standard.append(char(3));
    for (char value : QByteArray("066EFF313736504157095133")) {
        standard.append(value); standard.append('\0');
    }
    QCOMPARE(ProbeDetector::stLinkSerialFromUsbDescriptor(standard),
             QString("066EFF313736504157095133"));
    QVERIFY(ProbeDetector::stLinkSerialFromUsbDescriptor({}).isEmpty());
    QVERIFY(ProbeDetector::stLinkSerialFromUsbDescriptor(legacy.left(12)).isEmpty());
    legacy[1] = 2;
    QVERIFY(ProbeDetector::stLinkSerialFromUsbDescriptor(legacy).isEmpty());
    standard[3] = 1;
    QVERIFY(ProbeDetector::stLinkSerialFromUsbDescriptor(standard).isEmpty());
}

void ProbeDetectorTest::preservesManualSerialForLegacyStLink()
{
    QFETCH(QString, savedSerial);
    QFETCH(QString, expectedSerial);
    QString legacyId;
    QString detectedSerial;
    for (const ProbeInfo &probe : ProbeDetector::scan()) {
        if (probe.type == ProbeType::STLink && probe.productId == "3748" &&
            probe.instanceId.endsWith("\\C", Qt::CaseInsensitive)) {
            legacyId = probe.instanceId;
            detectedSerial = probe.serial;
        }
    }
    if (legacyId.isEmpty()) QSKIP("Requires the legacy ST-Link with truncated Windows instance C.");
    QTemporaryDir temporary(QDir::current().absoluteFilePath("legacy-serial-XXXXXX"));
    QVERIFY(temporary.isValid());
    QSettings settings(temporary.filePath("flash.ini"), QSettings::IniFormat);
    settings.setValue("STM32Flash/probeInstanceId", legacyId);
    settings.setValue("STM32Flash/probeSerial", savedSerial);
    FlashConfigDialog dialog(nullptr, &settings);
    QCOMPARE(dialog.getOptions().probeSerial, detectedSerial.isEmpty() ? expectedSerial : detectedSerial);
    QCOMPARE(dialog.getOptions().probeInstanceId, legacyId);
}

void ProbeDetectorTest::selectsProbeInFlashCommands()
{
    FlashOptions st;
    st.driver = FlashDriverType::OpenOCD_STLink;
    st.probeSerial = "066EFF313736504157095133";
    st.swdSpeedKhz = 1800;
    const QStringList openocd = FlashProgrammer::openOcdArguments("C:/firmware.bin", st, "target/stm32h7x.cfg");
    QVERIFY(openocd.contains("adapter serial 066EFF313736504157095133"));
    QVERIFY(openocd.contains("adapter speed 1800"));
    const QStringList cube = FlashProgrammer::cubeProgrammerArguments("C:/firmware.bin", st);
    QVERIFY(cube.contains("port=SWD"));
    QVERIFY(cube.contains("sn=066EFF313736504157095133"));
    QVERIFY(cube.contains("freq=1800"));
    const QStringList jlink = FlashProgrammer::jLinkArguments(st, "STM32H743VI", "script.jlink");
    QVERIFY(jlink.contains("-USB"));
    QVERIFY(jlink.contains(st.probeSerial));
}

void ProbeDetectorTest::preservesSelectedOpenOcdSpeedAfterTargetConfig()
{
    QString executable = qEnvironmentVariable("OPENOCD_TEST_EXECUTABLE");
    if (executable.isEmpty()) executable = QStandardPaths::findExecutable("openocd");
    if (executable.isEmpty()) QSKIP("OpenOCD is required to verify configuration ordering.");
    QTemporaryDir temporary(QDir::current().absoluteFilePath("speed-test-XXXXXX"));
    QVERIFY(temporary.isValid());
    QFile interfaceFile(temporary.filePath("interface.cfg"));
    QVERIFY(interfaceFile.open(QIODevice::WriteOnly));
    interfaceFile.close();
    QFile targetFile(temporary.filePath("target.cfg"));
    QVERIFY(targetFile.open(QIODevice::WriteOnly));
    targetFile.write("adapter speed 2000\n");
    targetFile.close();
    FlashOptions options;
    options.customOpenocdInterfaceCfg = interfaceFile.fileName();
    options.swdSpeedKhz = 1800;
    QStringList args = {"-c", "proc program {args} {echo EFFECTIVE_SPEED:[adapter speed]}"};
    args << FlashProgrammer::openOcdArguments("firmware.bin", options, targetFile.fileName());
    args << "-c" << "shutdown";
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(executable, args);
    QVERIFY(process.waitForFinished(10000));
    const QByteArray output = process.readAll();
    QVERIFY2(process.exitCode() == 0, output.constData());
    QVERIFY2(output.contains("EFFECTIVE_SPEED:adapter speed: 1800 kHz"), output.constData());
}

void ProbeDetectorTest::preservesOpenOcdFirmwarePath_data()
{
    QTest::addColumn<QString>("firmwarePath");
    QTest::addColumn<bool>("verify");
    QTest::addColumn<bool>("reset");
    QTest::newRow("plain") << QString("C:/build/firmware.bin") << true << true;
    QTest::newRow("wildfire-example") << QStringLiteral(
        "E:/野火【STM32H750_H743开发板-H7Pro】资料/1-程序源码_教程文档/"
        "1-[野火]《STM32 HAL库开发实战指南》(HAL库源码)【优先学习】/"
        "1-书籍配套例程-H743XI-V_20240311/12-GPIO输出—使用固件库点亮LED灯/"
        "build/stm32h743vi.bin") << true << true;
    QTest::newRow("windows-special-characters")
        << QString("C:\\firmware [board]\\$env(HOME); {draft}\\firmware.bin") << false << true;
    QTest::newRow("unbalanced-brace")
        << QString("C:/firmware {draft/firmware.bin") << true << false;
    QTest::newRow("quoted-posix-path")
        << QString("/tmp/board \"A\" [rev]/firmware.bin") << false << false;
}

void ProbeDetectorTest::preservesOpenOcdFirmwarePath()
{
    QFETCH(QString, firmwarePath);
    QFETCH(bool, verify);
    QFETCH(bool, reset);
    QString executable = qEnvironmentVariable("OPENOCD_TEST_EXECUTABLE");
    if (executable.isEmpty()) executable = QStandardPaths::findExecutable("openocd");
    if (executable.isEmpty()) QSKIP("Set OPENOCD_TEST_EXECUTABLE to test with the real Tcl parser.");

    FlashOptions options;
    options.verify = verify;
    options.resetAndRun = reset;
    const QString command = FlashProgrammer::openOcdArguments(
        firmwarePath, options, "target/stm32h7x.cfg").last();

    // No adapter/target config: replace flashing with argument capture before evaluating.
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(executable, {"-c",
        "proc program {args} {echo PATH:[lindex $args 0]; echo OPTIONS:[lrange $args 1 end]}",
        "-c", command, "-c", "shutdown"});
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
    QVERIFY(process.waitForFinished(10000));
    const QString output = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0,
             qPrintable(output));
    QString normalizedPath = firmwarePath;
    normalizedPath.replace('\\', '/');
    QVERIFY2(output.contains("PATH:" + normalizedPath), qPrintable(output));
    QString expectedOptions = "OPTIONS:0x08000000";
    if (verify) expectedOptions += " verify";
    if (reset) expectedOptions += " reset";
    expectedOptions += " exit";
    QVERIFY2(output.contains(expectedOptions), qPrintable(output));
}

void ProbeDetectorTest::rejectsIncompatibleDriverSelection()
{
    QVERIFY(FlashProgrammer::driverSupportsProbe(FlashDriverType::STM32CubeProg, ProbeType::STLink));
    QVERIFY(FlashProgrammer::driverSupportsProbe(FlashDriverType::OpenOCD_CMSISDAP, ProbeType::CmsisDap));
    QVERIFY(FlashProgrammer::driverSupportsProbe(FlashDriverType::JLink_Native, ProbeType::JLink));
    QVERIFY(!FlashProgrammer::driverSupportsProbe(FlashDriverType::STM32CubeProg, ProbeType::CmsisDap));
    QVERIFY(!FlashProgrammer::driverSupportsProbe(FlashDriverType::OpenOCD_STLink, ProbeType::JLink));
    QVERIFY(FlashProgrammer::driverSupportsMassErase(FlashDriverType::STM32CubeProg));
    QVERIFY(!FlashProgrammer::driverSupportsMassErase(FlashDriverType::OpenOCD_STLink));
}

void ProbeDetectorTest::scansPresentUsbDevicesWithoutDuplicates()
{
    QSet<QString> ids;
    for (const ProbeInfo &probe : ProbeDetector::scan()) {
        QVERIFY(probe.type != ProbeType::Unknown);
        QVERIFY(!probe.instanceId.isEmpty());
        QVERIFY(!ids.contains(probe.instanceId));
        ids.insert(probe.instanceId);
        qInfo().noquote() << ProbeDetector::typeName(probe.type) << probe.name
                          << probe.vendorId + ":" + probe.productId << probe.serial;
    }
    qInfo() << "Present probes:" << ids.size();
}

void ProbeDetectorTest::restoresSavedProbeSelection()
{
    QTemporaryDir temporary(QDir::current().absoluteFilePath("probe-settings-test-XXXXXX"));
    QVERIFY(temporary.isValid());
    QSettings settings(temporary.filePath("flash.ini"), QSettings::IniFormat);
    settings.setValue("STM32Flash/driver", static_cast<int>(FlashDriverType::JLink_Native));
    settings.setValue("STM32Flash/probeInstanceId", "USB\\VID_1366&PID_0105\\123456789");
    settings.setValue("STM32Flash/probeSerial", "123456789");
    FlashConfigDialog dialog(nullptr, &settings);
    const FlashOptions options = dialog.getOptions();
    QCOMPARE(options.driver, FlashDriverType::JLink_Native);
    QCOMPARE(options.probeSerial, QString("123456789"));
    QCOMPARE(options.probeInstanceId, QString("USB\\VID_1366&PID_0105\\123456789"));
    const auto *combo = dialog.findChild<QComboBox *>("probeCombo");
    QVERIFY(combo);
    QVERIFY(combo->currentText().contains(QStringLiteral("未连接")));
}

void ProbeDetectorTest::rejectsUnsafeFlashAddresses()
{
    QVERIFY(FlashProgrammer::validTargetAddress("0x08000000"));
    QVERIFY(FlashProgrammer::validTargetAddress("134217728"));
    QVERIFY(!FlashProgrammer::validTargetAddress(""));
    QVERIFY(!FlashProgrammer::validTargetAddress("0x08000000; reset halt"));
    QVERIFY(!FlashProgrammer::validTargetAddress("0x100000000"));
}

void ProbeDetectorTest::autoSelectsSingleConnectedProbe()
{
    const QList<ProbeInfo> probes = ProbeDetector::scan();
    if (probes.size() != 1 || probes.first().serial.isEmpty())
        QSKIP("Requires exactly one connected probe with a readable serial");
    QTemporaryDir temporary(QDir::current().absoluteFilePath("probe-auto-test-XXXXXX"));
    QVERIFY(temporary.isValid());
    QSettings settings(temporary.filePath("flash.ini"), QSettings::IniFormat);
    FlashConfigDialog dialog(nullptr, &settings);
    const FlashOptions options = dialog.getOptions();
    QCOMPARE(options.probeInstanceId, probes.first().instanceId);
    QCOMPARE(options.probeSerial, probes.first().serial);
    QVERIFY(FlashProgrammer::driverSupportsProbe(options.driver, probes.first().type));
    settings.setValue("STM32Flash/probeInstanceId", probes.first().instanceId);
    settings.setValue("STM32Flash/probeSerial", "stale-serial");
    FlashConfigDialog restored(nullptr, &settings);
    QCOMPARE(restored.getOptions().probeSerial, probes.first().serial);
}

void ProbeDetectorTest::generatesValidJLinkDownloadScript()
{
    FlashOptions options;
    options.targetAddress = "0x08000000";
    options.verify = true;
    options.resetAndRun = true;
    const QString script = FlashProgrammer::jLinkCommandScript("C:/staged/firmware.bin", options, "STM32H743VI");
    QVERIFY(script.contains("loadfile \"C:/staged/firmware.bin\" 0x08000000 noreset\n"));
    QVERIFY(!script.contains("firmware.bin\", 0x08000000"));
    QVERIFY(script.contains("r\ng\nq\n"));
    QVERIFY(!script.contains("verifybin")); // loadfile verifies internally
}

void ProbeDetectorTest::validatesOpenOcdProbeScripts()
{
    QTemporaryDir temporary(QDir::current().absoluteFilePath("openocd-preflight-XXXXXX"));
    QVERIFY(temporary.isValid());
    QVERIFY(QDir().mkpath(temporary.filePath("interface")));
    QVERIFY(QDir().mkpath(temporary.filePath("target")));
    const auto write = [](const QString &path) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write("# test\n") > 0;
    };
    QVERIFY(write(temporary.filePath("interface/stlink.cfg")));
    QVERIFY(write(temporary.filePath("interface/cmsis-dap.cfg")));
    QVERIFY(write(temporary.filePath("target/stm32h7x.cfg")));
    FlashOptions options;
    QString error;
    options.driver = FlashDriverType::OpenOCD_STLink;
    QVERIFY(FlashProgrammer::validateOpenOcdScripts(options, "target/stm32h7x.cfg",
                                                   temporary.path(), &error));
    options.driver = FlashDriverType::OpenOCD_CMSISDAP;
    QVERIFY(FlashProgrammer::validateOpenOcdScripts(options, "target/stm32h7x.cfg",
                                                   temporary.path(), &error));
    QVERIFY(!FlashProgrammer::validateOpenOcdScripts(options, "target/stm32u3x.cfg",
                                                    temporary.path(), &error));
    QVERIFY(error.contains("stm32u3x.cfg"));
    QVERIFY(STM32ProfileManager::getOpenocdTarget("STM32X9").isEmpty());
}

void ProbeDetectorTest::allowsOnlyOneMatchingSeriallessProbe()
{
    ProbeInfo first;
    first.type = ProbeType::STLink;
    first.instanceId = "USB\\VID_0483&PID_3748\\A";
    ProbeInfo second = first;
    second.instanceId = "USB\\VID_0483&PID_3748\\B";
    FlashOptions options;
    options.driver = FlashDriverType::OpenOCD_STLink;
    options.probeInstanceId = first.instanceId;
    QString error;
    QVERIFY(FlashProgrammer::validateProbeSelection(options, {first}, &error));
    QVERIFY(!FlashProgrammer::validateProbeSelection(options, {first, second}, &error));
    QVERIFY(error.contains(QStringLiteral("序列号")));
}

void ProbeDetectorTest::resolvesOpenOcdScriptsFromBothLayouts()
{
    QTemporaryDir temporary(QDir::current().absoluteFilePath("openocd-layout-test-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QString bin = temporary.filePath("bin");
    QVERIFY(QDir().mkpath(bin));
    const QString executable = QDir(bin).filePath("openocd.exe");
    const QString standard = temporary.filePath("share/openocd/scripts");
    const QString compact = temporary.filePath("scripts");
    QVERIFY(QDir().mkpath(standard));
    QCOMPARE(QDir::cleanPath(FlashProgrammer::resolveOpenOcdScriptsDir(executable)),
             QDir::cleanPath(standard));
    QVERIFY(QDir().rmdir(standard));
    QVERIFY(QDir().rmdir(temporary.filePath("share/openocd")));
    QVERIFY(QDir().rmdir(temporary.filePath("share")));
    QVERIFY(QDir().mkpath(compact));
    QCOMPARE(QDir::cleanPath(FlashProgrammer::resolveOpenOcdScriptsDir(executable)),
             QDir::cleanPath(compact));
}

QTEST_MAIN(ProbeDetectorTest)
#include <ProbeDetectorTest.moc>
