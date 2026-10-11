#include <QtTest>
#include "../Core/ProbeDetector.h"
#include "../Core/FlashProgrammer.h"
#include "../UI/FlashConfigDialog.h"
#include <QTemporaryDir>

class ProbeDetectorTest : public QObject {
    Q_OBJECT
private slots:
    void recognizesCommonUsbProbes();
    void ignoresUnrelatedUsbDevices();
    void parsesCompositeDeviceSerial();
    void selectsProbeInFlashCommands();
    void rejectsIncompatibleDriverSelection();
    void scansPresentUsbDevicesWithoutDuplicates();
    void restoresSavedProbeSelection();
    void rejectsUnsafeFlashAddresses();
    void autoSelectsSingleConnectedProbe();
    void generatesValidJLinkDownloadScript();
};

void ProbeDetectorTest::recognizesCommonUsbProbes()
{
    QCOMPARE(ProbeDetector::classify("USB\\VID_0483&PID_374B", "ST-LINK/V2-1"), ProbeType::STLink);
    QCOMPARE(ProbeDetector::classify("USB\\VID_0483&PID_3757", "STLINK-V3"), ProbeType::STLink);
    QCOMPARE(ProbeDetector::classify("USB\\VID_1366&PID_0105", "J-Link"), ProbeType::JLink);
    QCOMPARE(ProbeDetector::classify("USB\\VID_0D28&PID_0204", "DAPLink CMSIS-DAP"), ProbeType::CmsisDap);
    QCOMPARE(ProbeDetector::classify("USB\\VID_C251&PID_F001", "CMSIS-DAP v2"), ProbeType::CmsisDap);
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

QTEST_MAIN(ProbeDetectorTest)
#include "ProbeDetectorTest.moc"
