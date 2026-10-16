#include <QtTest>
#include "../Core/FlashLogModel.h"

namespace {
QStringList texts(const QVector<FlashLogEntry> &entries)
{
    QStringList result;
    for (const auto &entry : entries) result.append(entry.text);
    return result;
}

// Captured from serial-flash-verification.log. OpenOCD sends this ordinary
// successful session to stderr, including its adapter-speed fallback messages.
const QString successfulOpenOcdSession = QStringLiteral(
    "Open On-Chip Debugger 0.12.0 (2024-09-16) [https://github.com/sysprogs/openocd]\n"
    "Licensed under GNU GPL v2\n"
    "libusb1 d52e355daa09f17ce64819122cb067b8a2ee0d4b\n"
    "For bug reports, read\n"
    "\thttp://openocd.org/doc/doxygen/bugs.html\n"
    "Info : auto-selecting first available session transport \"hla_swd\". To override use 'transport select <transport>'.\n"
    "Info : The selected transport took over low-level target control. The results might differ compared to plain JTAG/SWD\n"
    "adapter speed: 4000 kHz\n"
    "Info : clock speed 4000 kHz\n"
    "Info : STLINK V2J46S7 (API v2) VID:PID 0483:3748\n"
    "Info : Target voltage: 3.244622\n"
    "Info : [stm32f4x.cpu] Cortex-M4 r0p1 processor detected\n"
    "Info : [stm32f4x.cpu] target has 6 breakpoints, 4 watchpoints\n"
    "Info : [stm32f4x.cpu] Examination succeed\n"
    "Info : [stm32f4x.cpu] starting gdb server on 3333\n"
    "Info : Listening on port 3333 for gdb connections\n"
    "Info : Unable to match requested speed 2000 kHz, using 1800 kHz\n"
    "Info : Unable to match requested speed 2000 kHz, using 1800 kHz\n"
    "[stm32f4x.cpu] halted due to debug-request, current mode: Thread \n"
    "xPSR: 0x01000000 pc: 0x080001c4 msp: 0x20000410\n"
    "Info : Unable to match requested speed 8000 kHz, using 4000 kHz\n"
    "Info : Unable to match requested speed 8000 kHz, using 4000 kHz\n"
    "** Programming Started **\n"
    "Info : device id = 0x20036419\n"
    "Info : flash size = 1024 KiB\n"
    "Info : Single Bank 1024 kiB STM32F42x/43x/469/479 found\n"
    "** Programming Finished **\n"
    "** Verify Started **\n"
    "** Verified OK **\n"
    "** Resetting Target **\n"
    "Info : Unable to match requested speed 2000 kHz, using 1800 kHz\n"
    "Info : Unable to match requested speed 2000 kHz, using 1800 kHz\n"
    "shutdown command invoked\n");
}

class FlashLogModelTest : public QObject {
    Q_OBJECT
private slots:
    void classifiesContent_data();
    void classifiesContent();
    void usesExistingPalette();
    void replaysSuccessfulStderrSession();
    void buffersPartialLinesPerStream();
    void preservesFinalPartialArrivalOrder();
    void retainsMixedDiagnosticsInConciseView();
    void deduplicatesConfirmedStages();
    void respectsDisabledVerifyAndReset();
    void completionRequiresProcessResult();
    void reportsFailureAndFlushesTail();
    void downgradesContradictorySuccess();
    void warningDoesNotFailSuccessfulResult();
    void resetsSessionState();
    void recognizesOtherProgrammerStages();
};

void FlashLogModelTest::classifiesContent_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<FlashLogSeverity>("expected");
    QTest::newRow("banner") << QString("Open On-Chip Debugger 0.12.0") << FlashLogSeverity::Info;
    QTest::newRow("gpl") << QString("Licensed under GNU GPL v2") << FlashLogSeverity::Info;
    QTest::newRow("info") << QString("Info : Target voltage: 3.244622") << FlashLogSeverity::Info;
    QTest::newRow("speed-fallback") << QString("Info : Unable to match requested speed 2000 kHz, using 1800 kHz") << FlashLogSeverity::Info;
    QTest::newRow("debug-trace") << QString("Debug: 364 72 command.c:155 script_debug(): command - program") << FlashLogSeverity::Info;
    QTest::newRow("filename-with-error") << QString("Downloading file [firmware_error.bin]...") << FlashLogSeverity::Info;
    QTest::newRow("no-errors") << QString("Download completed, 0 errors") << FlashLogSeverity::Info;
    QTest::newRow("error-count-zero") << QString("Error count: 0") << FlashLogSeverity::Info;
    QTest::newRow("warning") << QString("Warn : target voltage is low") << FlashLogSeverity::Warning;
    QTest::newRow("warning-word") << QString("WARNING: selected interface has no serial") << FlashLogSeverity::Warning;
    QTest::newRow("stflash-warning-level") << QString("2026-10-16T09:00:00 WARN common.c: unknown flash loader") << FlashLogSeverity::Warning;
    QTest::newRow("chinese-warning") << QStringLiteral("警告: 检测到多台下载器") << FlashLogSeverity::Warning;
    QTest::newRow("error") << QString("Error: init mode failed (unable to connect to the target)") << FlashLogSeverity::Error;
    QTest::newRow("fatal") << QString("FATAL: USB communication failed") << FlashLogSeverity::Error;
    QTest::newRow("stflash-error-level") << QString("2026-10-16T09:00:00 ERROR common.c: stlink_flash_loader_run(0x8000000) returned -1") << FlashLogSeverity::Error;
    QTest::newRow("cannot-connect") << QString("Cannot connect to target.") << FlashLogSeverity::Error;
    QTest::newRow("no-probe") << QString("No ST-LINK detected!") << FlashLogSeverity::Error;
    QTest::newRow("program-failed") << QString("Flash programming failed.") << FlashLogSeverity::Error;
    QTest::newRow("chinese-error") << QStringLiteral("错误: 固件文件不存在") << FlashLogSeverity::Error;
    QTest::newRow("chinese-cancel") << QStringLiteral("[已手动中止固件烧录]") << FlashLogSeverity::Error;
    QTest::newRow("programming-finished") << QString("** Programming Finished **") << FlashLogSeverity::Success;
    QTest::newRow("verified") << QString("** Verified OK **") << FlashLogSeverity::Success;
    QTest::newRow("reset-start") << QString("** Resetting Target **") << FlashLogSeverity::Info;
}

void FlashLogModelTest::classifiesContent()
{
    QFETCH(QString, line);
    QFETCH(FlashLogSeverity, expected);
    QCOMPARE(FlashLogModel::classify(line), expected);
}

void FlashLogModelTest::usesExistingPalette()
{
    QCOMPARE(FlashLogModel::color(FlashLogSeverity::Info), QColor("#cdd6f4"));
    QCOMPARE(FlashLogModel::color(FlashLogSeverity::Success), QColor("#a6e3a1"));
    QCOMPARE(FlashLogModel::color(FlashLogSeverity::Warning), QColor("#f9e2af"));
    QCOMPARE(FlashLogModel::color(FlashLogSeverity::Error), QColor("#f38ba8"));
}

void FlashLogModelTest::replaysSuccessfulStderrSession()
{
    FlashLogModel model;
    model.reset();
    const auto added = model.append(successfulOpenOcdSession, true);
    const QStringList stages = {QStringLiteral("连接成功"), QStringLiteral("开始下载…"),
        QStringLiteral("编程完成"), QStringLiteral("校验通过"), QStringLiteral("正在复位…")};
    QCOMPARE(texts(added), stages);
    QCOMPARE(texts(model.conciseEntries()), stages);
    QCOMPARE(model.detailedEntries().size(), successfulOpenOcdSession.count('\n'));
    QCOMPARE(model.detailedEntries().first().severity, FlashLogSeverity::Info);
    for (const auto &entry : model.detailedEntries())
        QVERIFY(entry.severity != FlashLogSeverity::Error);
    QCOMPARE(model.finish(true, QStringLiteral("下载完成，程序已复位运行。耗时 1.2 秒")).severity,
             FlashLogSeverity::Success);
    QCOMPARE(model.conciseEntries().size(), 6);
}

void FlashLogModelTest::buffersPartialLinesPerStream()
{
    FlashLogModel model;
    QVERIFY(model.append("** Progra", false).isEmpty());
    QVERIFY(model.append("Info : clock speed 4000 kHz\r", true).isEmpty());
    QCOMPARE(model.detailedEntries().size(), 1);
    QCOMPARE(model.detailedEntries().first().text, QString("Info : clock speed 4000 kHz"));
    auto added = model.append("mming Started **\r", false);
    QCOMPARE(texts(added), QStringList{QStringLiteral("开始下载…")});
    QVERIFY(model.append("\n** Programming Finished **\n", false).size() == 1);
    QVERIFY(model.append("\nWarn : low", true).isEmpty());
    QCOMPARE(model.detailedEntries().size(), 3);
    added = model.append(" target voltage\n\n", true);
    QCOMPARE(added.size(), 1);
    QCOMPARE(added.first().severity, FlashLogSeverity::Warning);
    QCOMPARE(model.detailedEntries().at(3).text, QString("Warn : low target voltage"));
    QCOMPARE(model.detailedEntries().at(4).text, QString());
    QVERIFY(model.flush().isEmpty());
}

void FlashLogModelTest::preservesFinalPartialArrivalOrder()
{
    FlashLogModel model;
    model.append("Warn : first partial", true);
    model.append("Error: second partial", false);
    QCOMPARE(texts(model.flush()), QStringList({"Warn : first partial", "Error: second partial"}));
    QCOMPARE(texts(model.detailedEntries()), QStringList({"Warn : first partial", "Error: second partial"}));
    QVERIFY(model.flush().isEmpty());
}

void FlashLogModelTest::retainsMixedDiagnosticsInConciseView()
{
    FlashLogModel model;
    const auto added = model.append("Info : clock speed 4000 kHz\nWarn : slow connection\n"
                                    "Error: target not examined yet\n", true);
    QCOMPARE(added.size(), 2);
    QCOMPARE(added.at(0).severity, FlashLogSeverity::Warning);
    QCOMPARE(added.at(1).severity, FlashLogSeverity::Error);
    QCOMPARE(model.detailedEntries().at(0).severity, FlashLogSeverity::Info);
    QCOMPARE(model.detailedEntries().at(1).text, QString("Warn : slow connection"));
}

void FlashLogModelTest::deduplicatesConfirmedStages()
{
    FlashLogModel model;
    model.append("Info : Cortex-M4 r0p1 processor detected\nInfo : Examination succeed\n"
                 "** Programming Started **\n** Programming Started **\n"
                 "** Programming Finished **\n** Programming Finished **\n"
                 "** Verified OK **\n** Verified OK **\n"
                 "** Resetting Target **\n** Resetting Target **\n", true);
    QCOMPARE(model.conciseEntries().size(), 5);
    QCOMPARE(model.detailedEntries().size(), 10);
}

void FlashLogModelTest::respectsDisabledVerifyAndReset()
{
    FlashLogModel model;
    model.reset(false, false);
    model.append("** Programming Finished **\n** Verified OK **\n** Resetting Target **\n", true);
    QCOMPARE(texts(model.conciseEntries()), QStringList{QStringLiteral("编程完成")});
    QCOMPARE(model.detailedEntries().size(), 3);
}

void FlashLogModelTest::completionRequiresProcessResult()
{
    FlashLogModel model;
    model.append("Erasing flash memory...\n** Programming Finished **\n** Verified OK **\n"
                 "** Resetting Target **\nshutdown command invoked\n", true);
    QCOMPARE(texts(model.conciseEntries()), QStringList({QStringLiteral("编程完成"),
        QStringLiteral("校验通过"), QStringLiteral("正在复位…")}));
    for (const auto &entry : model.conciseEntries()) {
        QVERIFY(!entry.text.contains(QStringLiteral("下载完成")));
        QVERIFY(!entry.text.contains(QStringLiteral("擦除完成")));
        QVERIFY(!entry.text.contains(QStringLiteral("复位完成")));
        QVERIFY(!entry.text.contains("100%"));
    }
    const auto summary = model.finish(true, QStringLiteral("下载完成。耗时 1.2 秒"));
    QCOMPARE(summary.severity, FlashLogSeverity::Success);
    QCOMPARE(summary.text, QStringLiteral("下载完成。耗时 1.2 秒"));
    QCOMPARE(model.detailedEntries().last().text, summary.text);
    QCOMPARE(model.conciseEntries().last().text, summary.text);
    QCOMPARE(model.finish(true, summary.text).text, summary.text);
    QCOMPARE(model.conciseEntries().size(), 4);
}

void FlashLogModelTest::reportsFailureAndFlushesTail()
{
    FlashLogModel model;
    model.append("Info : STLINK V2J46S7\nError: init mode failed (unable to connect to the target)", true);
    const auto summary = model.finish(false, QStringLiteral("下载失败，退出代码: 1"));
    QCOMPARE(summary.severity, FlashLogSeverity::Error);
    QCOMPARE(model.detailedEntries().size(), 3);
    QCOMPARE(model.detailedEntries().at(1).severity, FlashLogSeverity::Error);
    QCOMPARE(model.conciseEntries().size(), 2);
}

void FlashLogModelTest::downgradesContradictorySuccess()
{
    FlashLogModel model;
    model.append("Error: Failed to erase memory\n", true);
    const auto summary = model.finish(true, QStringLiteral("下载完成。耗时 1.2 秒"));
    QCOMPARE(summary.severity, FlashLogSeverity::Error);
    QVERIFY(summary.text.contains(QStringLiteral("日志")));
    QVERIFY(!summary.text.startsWith(QStringLiteral("下载完成")));
}

void FlashLogModelTest::warningDoesNotFailSuccessfulResult()
{
    FlashLogModel model;
    model.append("Warn : adapter speed adjusted\n", true);
    QCOMPARE(model.finish(true, QStringLiteral("下载完成")).severity, FlashLogSeverity::Success);
}

void FlashLogModelTest::resetsSessionState()
{
    FlashLogModel model;
    model.reset(false, false);
    model.append("Error: previous connection failed\nunfinished", true);
    model.finish(false, QStringLiteral("下载失败"));
    model.reset();
    QVERIFY(model.detailedEntries().isEmpty());
    QVERIFY(model.conciseEntries().isEmpty());
    QVERIFY(model.flush().isEmpty());
    model.append("** Verified OK **\n** Resetting Target **\n", true);
    QCOMPARE(model.conciseEntries().size(), 2);
    QCOMPARE(model.finish(true, QStringLiteral("下载完成")).severity, FlashLogSeverity::Success);
}

void FlashLogModelTest::recognizesOtherProgrammerStages()
{
    FlashLogModel cube;
    cube.append("ST-LINK SN : 066EFF313736504157095133\nDownload in Progress:\n"
                "File download complete\nDownload verified successfully\n", false);
    QCOMPARE(texts(cube.conciseEntries()), QStringList({QStringLiteral("开始下载…"),
        QStringLiteral("编程完成"), QStringLiteral("校验通过")}));
    FlashLogModel jlink;
    jlink.append("Cortex-M4 identified.\nDownloading file [firmware.bin]...\n"
                 "Flash download: Bank 0 @ 0x08000000: Skipped. Contents already match\n", false);
    QCOMPARE(texts(jlink.conciseEntries()), QStringList({QStringLiteral("连接成功"), QStringLiteral("开始下载…")}));
    FlashLogModel stflash;
    stflash.append("INFO common.c: Starting verification of write complete\n"
                   "INFO common.c: Flash written and verified! jolly good!\n", true);
    QCOMPARE(texts(stflash.conciseEntries()), QStringList({QStringLiteral("编程完成"), QStringLiteral("校验通过")}));
}

QTEST_GUILESS_MAIN(FlashLogModelTest)
#include "FlashLogModelTest.moc"
