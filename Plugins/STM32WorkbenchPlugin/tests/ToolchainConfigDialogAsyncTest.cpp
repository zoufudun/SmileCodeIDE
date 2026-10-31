#include <QtTest>
#include <QApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QTimer>
#include "../UI/ToolchainConfigDialog.h"

class ToolchainConfigDialogAsyncTest : public QObject {
    Q_OBJECT
private slots:
    void constructionDoesNotWaitForVersions() {
        QTemporaryDir temporary;
        QSettings settings(temporary.filePath("tools.ini"), QSettings::IniFormat);
        const QString executable = QCoreApplication::applicationFilePath();
        for (const QString key : {"gccPath", "gdbPath", "makePath", "openocdPath",
                                   "jlinkPath", "stm32ProgPath"}) {
            settings.setValue("STM32Toolchain/" + key, executable);
        }
        QElapsedTimer elapsed;
        elapsed.start();
        ToolchainConfigDialog dialog(nullptr, &settings);
        QVERIFY2(elapsed.elapsed() < 300, qPrintable(QString("Construction blocked for %1 ms").arg(elapsed.elapsed())));
        bool heartbeat = false;
        QTimer::singleShot(20, &dialog, [&heartbeat] { heartbeat = true; });
        QTRY_VERIFY_WITH_TIMEOUT(heartbeat, 300);
        auto save = dialog.findChild<QPushButton *>("toolchainSave");
        QVERIFY(save);
        QVERIFY(!save->isEnabled());
        QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(), 6500);
    }

    void initializedPathsPreserveCompanionsAndHiddenChipSettings() {
        QTemporaryDir temporary;
        QSettings settings(temporary.filePath("tools.ini"), QSettings::IniFormat);
        settings.setValue("STM32Toolchain/selectedChip", "STM32F407VG");
        ToolchainConfigDialog dialog(nullptr, &settings);
        ToolchainPaths paths;
        paths.gccPath = "Z:/unplugged/toolchain/gcc.exe";
        paths.gppPath = "Z:/unplugged/toolchain/g++.exe";
        paths.gdbPath = "Z:/unplugged/toolchain/gdb.exe";
        paths.objcopyPath = "Z:/unplugged/toolchain/objcopy.exe";
        paths.sizePath = "Z:/unplugged/toolchain/size.exe";
        paths.openocdScriptsDir = "Z:/unplugged/openocd/scripts";
        dialog.setPaths(paths);
        dialog.setChipSelectionVisible(false);
        QVERIFY(dialog.findChild<QGroupBox *>("toolchainChipGroup")->isHidden());
        QCOMPARE(dialog.getPaths().gppPath, paths.gppPath);
        QCOMPARE(dialog.getPaths().objcopyPath, paths.objcopyPath);
        QCOMPARE(dialog.getPaths().sizePath, paths.sizePath);
        auto save = dialog.findChild<QPushButton *>("toolchainSave");
        QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(), 2000);
        QVERIFY(QMetaObject::invokeMethod(&dialog, "onSaveClicked"));
        QCOMPARE(settings.value("STM32Toolchain/gccPath").toString(), paths.gccPath);
        QCOMPARE(settings.value("STM32Toolchain/gppPath").toString(), paths.gppPath);
        QCOMPARE(settings.value("STM32Toolchain/selectedChip").toString(), QString("STM32F407VG"));
    }

    void pendingDiagnosticCanCloseWithoutBlockingOrUpdatingSettings() {
        QTemporaryDir temporary;
        QSettings settings(temporary.filePath("tools.ini"), QSettings::IniFormat);
        auto dialog = new ToolchainConfigDialog(nullptr, &settings);
        ToolchainPaths paths;
        paths.gccPath = QCoreApplication::applicationFilePath();
        dialog->setPaths(paths);
        QVERIFY(!dialog->findChild<QPushButton *>("toolchainSave")->isEnabled());
        QSignalSpy accepted(dialog, &QDialog::accepted);
        QVERIFY(QMetaObject::invokeMethod(dialog, "onSaveClicked"));
        QCOMPARE(accepted.count(), 0);
        QElapsedTimer elapsed;
        elapsed.start();
        delete dialog;
        QVERIFY2(elapsed.elapsed() < 300, "Destroying a pending dialog waited for its worker");
        QVERIFY(settings.allKeys().isEmpty());
        bool heartbeat = false;
        QTimer::singleShot(20, this, [&heartbeat] { heartbeat = true; });
        QTRY_VERIFY_WITH_TIMEOUT(heartbeat, 300);
        QTest::qWait(1100);
    }

    void newestInitializationWinsOverSlowPreviousReport() {
        ToolchainConfigDialog dialog;
        ToolchainPaths slow;
        slow.gccPath = QCoreApplication::applicationFilePath();
        dialog.setPaths(slow);
        ToolchainPaths latest;
        latest.gccPath = "Z:/new-selection/gcc.exe";
        dialog.setPaths(latest);
        auto save = dialog.findChild<QPushButton *>("toolchainSave");
        auto report = dialog.findChild<QTextEdit *>("toolchainDiagnosticReport");
        QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(), 2000);
        QVERIFY(report->toPlainText().contains(latest.gccPath));
        QTest::qWait(1100);
        QCOMPARE(dialog.getPaths().gccPath, latest.gccPath);
        QVERIFY(report->toPlainText().contains(latest.gccPath));
        QVERIFY(!report->toPlainText().contains(slow.gccPath));
    }

    void compilerOptimizationSettingsRoundtripAndDynamicDescription() {
        QTemporaryDir temporary;
        QSettings settings(temporary.filePath("tools.ini"), QSettings::IniFormat);
        ToolchainConfigDialog dialog(nullptr, &settings);

        auto optGroup = dialog.findChild<QGroupBox *>("toolchainOptGroup");
        QVERIFY(optGroup != nullptr);

        auto optCombo = dialog.findChild<QComboBox *>("optLevelCombo");
        auto descLabel = dialog.findChild<QLabel *>("toolchainOptDescription");
        auto debugCombo = dialog.findChild<QComboBox *>("debugLevelCombo");
        auto tolCheck = dialog.findChild<QCheckBox *>("tolerantFlagsCheck");
        auto customEdit = dialog.findChild<QLineEdit *>("customFlagsEdit");

        QVERIFY(optCombo != nullptr);
        QVERIFY(descLabel != nullptr);
        QVERIFY(debugCombo != nullptr);
        QVERIFY(tolCheck != nullptr);
        QVERIFY(customEdit != nullptr);

        // 默认应当为 -O1
        QCOMPARE(dialog.getBuildOptions().optLevel, QString("-O1"));
        QVERIFY(descLabel->text().contains(QStringLiteral("默认推荐")));

        // 切换为 -O0
        int idxO0 = optCombo->findData("-O0");
        QVERIFY(idxO0 >= 0);
        optCombo->setCurrentIndex(idxO0);
        QCOMPARE(dialog.getBuildOptions().optLevel, QString("-O0"));
        QVERIFY(descLabel->text().contains(QStringLiteral("无优化警告")));

        // 切换为 -O2
        int idxO2 = optCombo->findData("-O2");
        QVERIFY(idxO2 >= 0);
        optCombo->setCurrentIndex(idxO2);
        QCOMPARE(dialog.getBuildOptions().optLevel, QString("-O2"));
        QVERIFY(descLabel->text().contains(QStringLiteral("性能优先")));

        // 调试信息切换为 -g0
        int idxG0 = debugCombo->findData("-g0");
        QVERIFY(idxG0 >= 0);
        debugCombo->setCurrentIndex(idxG0);

        tolCheck->setChecked(true);
        customEdit->setText("-fstack-usage");

        // 保存
        auto save = dialog.findChild<QPushButton *>("toolchainSave");
        QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(), 2000);
        QVERIFY(QMetaObject::invokeMethod(&dialog, "onSaveClicked"));

        // 检查 settings 中的持久化数据
        QCOMPARE(settings.value("STM32Toolchain/optLevel").toString(), QString("-O2"));
        QCOMPARE(settings.value("STM32Toolchain/debugLevel").toString(), QString("-g0"));
        QCOMPARE(settings.value("STM32Toolchain/enableToleranceFlags").toBool(), true);
        QCOMPARE(settings.value("STM32Toolchain/customCFlags").toString(), QString("-fstack-usage"));

        // 验证持久化后再恢复
        ToolchainConfigDialog dialog2(nullptr, &settings);
        QCOMPARE(dialog2.getBuildOptions().optLevel, QString("-O2"));
        QCOMPARE(dialog2.getBuildOptions().debugLevel, QString("-g0"));
        QCOMPARE(dialog2.getBuildOptions().customCFlags, QString("-fstack-usage"));
    }

    void scanReturnsImmediatelyAndPreservesUnavailableManualTools() {
        ToolchainConfigDialog dialog;
        ToolchainPaths manual;
        manual.gccPath = "Z:/manual/gcc.exe";
        manual.gppPath = "Z:/manual/g++.exe";
        manual.gdbPath = "Z:/manual/gdb.exe";
        manual.objcopyPath = "Z:/manual/objcopy.exe";
        manual.sizePath = "Z:/manual/size.exe";
        manual.makePath = "Z:/manual/make.exe";
        manual.openocdPath = "Z:/manual/openocd.exe";
        manual.openocdScriptsDir = "Z:/manual/openocd/scripts";
        manual.jlinkPath = "Z:/manual/JLink.exe";
        manual.stm32ProgPath = "Z:/manual/STM32_Programmer_CLI.exe";
        dialog.setPaths(manual);
        auto save = dialog.findChild<QPushButton *>("toolchainSave");
        auto scan = dialog.findChild<QPushButton *>("toolchainAutoDetect");
        QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(), 2000);
        QElapsedTimer elapsed;
        elapsed.start();
        QVERIFY(QMetaObject::invokeMethod(&dialog, "onAutoDetectClicked"));
        QVERIFY2(elapsed.elapsed() < 300, "Scanning blocked the GUI thread");
        QVERIFY(!scan->isEnabled());
        QVERIFY(!save->isEnabled());
        bool heartbeat = false;
        QTimer::singleShot(20, &dialog, [&heartbeat] { heartbeat = true; });
        QTRY_VERIFY_WITH_TIMEOUT(heartbeat, 300);
        QTRY_VERIFY_WITH_TIMEOUT(save->isEnabled(), 20000);
        QCOMPARE(dialog.getPaths().gccPath, manual.gccPath);
        QCOMPARE(dialog.getPaths().gppPath, manual.gppPath);
        QCOMPARE(dialog.getPaths().gdbPath, manual.gdbPath);
        QCOMPARE(dialog.getPaths().objcopyPath, manual.objcopyPath);
        QCOMPARE(dialog.getPaths().sizePath, manual.sizePath);
        QCOMPARE(dialog.getPaths().makePath, manual.makePath);
        QCOMPARE(dialog.getPaths().openocdPath, manual.openocdPath);
        QCOMPARE(dialog.getPaths().openocdScriptsDir, manual.openocdScriptsDir);
        QCOMPARE(dialog.getPaths().jlinkPath, manual.jlinkPath);
        QCOMPARE(dialog.getPaths().stm32ProgPath, manual.stm32ProgPath);
        QVERIFY(dialog.findChild<QTextEdit *>("toolchainDiagnosticReport")->toPlainText()
                    .contains(QStringLiteral("扫描完成")));
        QVERIFY(scan->isEnabled());
    }
};

int main(int argc, char **argv) {
    if (argc > 1 && QByteArray(argv[1]) == "--version") {
        QThread::msleep(900);
        return 0;
    }
    QApplication application(argc, argv);
    ToolchainConfigDialogAsyncTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "ToolchainConfigDialogAsyncTest.moc"
