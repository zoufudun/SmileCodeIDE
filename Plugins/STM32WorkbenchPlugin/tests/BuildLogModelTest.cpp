#include <QtTest>
#include "../Core/BuildLogModel.h"
class BuildLogModelTest : public QObject {
    Q_OBJECT
private slots:
    void commandAndStageTextIsNotDiagnostic() {
        BuildLogModel m; m.reset("C:/p", "build");
        const QStringList tools={"gcc","g++","as","ld","objcopy"};
        for (const auto &tool:tools) {
            m.append(QString("arm-none-eabi-%1 -DMSG=\\\"error:\\\" -DOTHER=\\\"warning:\\\" -c main.c\n").arg(tool),false);
            m.append(QString("\"D:/Tool chain/arm-none-eabi-%1.exe\" -DMSG=\\\"error:\\\" -c main.c\n").arg(tool),false);
        }
        for (const auto &stage:QStringList{"CC","CXX","AS","LD","HEX","BIN"})
            m.append(QString("[BUILD:%1] \"Src/error:warning:main.c\"\n").arg(stage),false);
        m.append("printf(\"error: bad\");\n    fprintf(stderr, \"warning: bad\");\nprintf(\"context: error: bad\");\n",true);
        QCOMPARE(m.errorCount(),0); QCOMPARE(m.warningCount(),0);
        QCOMPARE(m.conciseEntries().size(),6);
        for (const auto &e:m.detailedEntries()) QCOMPARE(e.severity,DiagnosticSeverity::Info);
        QVERIFY(m.finish(true,0,10).success);
        m.reset("C:/p","build");
        m.append("arm-none-eabi-gcc: error: missing input\narm-none-eabi-g++: warning: ignored option\n",true);
        QCOMPARE(m.errorCount(),1); QCOMPARE(m.warningCount(),1);
    }
    void streamsAndLocations() {
        BuildLogModel m; m.reset("C:/工程", "build");
        QVERIFY(m.append("\x1b[31mC:/工程/Src/main.c:12:", true).isEmpty());
        m.append("[BUILD:CC] \"Src/main.c\"\r\n", false);
        const auto d = m.append("7: error: bad value\x1b[0m\r\n", true);
        QCOMPARE(d.size(), 1); QCOMPARE(d[0].filePath, QString("C:/工程/Src/main.c"));
        QCOMPARE(d[0].line, 12); QCOMPARE(d[0].column, 7);
        QCOMPARE(m.errorCount(), 1); QCOMPARE(m.conciseEntries()[0].text, QString("compiling main.c..."));
        QVERIFY(!m.detailedEntries().last().text.contains(QChar(27)));
        m.append("Src/other.c:8: warning: unused", true);
        QCOMPARE(m.flush().size(), 1); QCOMPARE(m.warningCount(), 1);
        QCOMPARE(m.diagnostics().last().filePath, QString("C:/工程/Src/other.c"));
        QVERIFY(m.flush().isEmpty());
    }
    void diagnosticContexts() {
        BuildLogModel m; m.reset("C:/p", "build");
        m.append("a.c:1:2: warning: unused\na.c:2:2: warning: unused\na.c:3:4: note: declared here\n  3 | x\n    | ^\n", true);
        QCOMPARE(m.warningCount(), 2); QCOMPARE(m.conciseEntries().size(), 2);
        QCOMPARE(m.detailedEntries().size(), 5); QCOMPARE(m.diagnostics().size(), 3);
    }
    void splitAnsiAndWindowsPaths() {
        BuildLogModel m; m.reset("C:/工程项目", "build");
        m.append("\x1b[3", true);
        m.append("1mD:\\项目 甲\\Src\\main.c:42: fatal error: absent header\x1b[0m\r", true);
        QCOMPARE(m.errorCount(),0);
        const auto diagnostics = m.append("\n",true);
        QCOMPARE(diagnostics.size(),1);
        QCOMPARE(diagnostics[0].filePath,QString("D:/项目 甲/Src/main.c"));
        QCOMPARE(diagnostics[0].severity,DiagnosticSeverity::Fatal);
        QCOMPARE(diagnostics[0].column,0);
        m.append("make: *** [main.o] Error 1\ncollect2: error: ld returned 1 exit status\n",true);
        QCOMPARE(m.errorCount(),1);
        QVERIFY(!m.finish(true,0,10).success);
        QVERIFY(m.append("a.c:1: error: too late\n",true).isEmpty());
        QCOMPARE(m.errorCount(),1);
    }
    void linkerLocationWithoutSeverity() {
        BuildLogModel m; m.reset("C:/工程", "build");
        m.append("C:/工程/Src/main.c:71: undefined reference to `missing'\n", true);
        QCOMPARE(m.errorCount(),1);
        QCOMPARE(m.diagnostics()[0].filePath,QString("C:/工程/Src/main.c"));
        QCOMPARE(m.diagnostics()[0].line,71);
    }
    void sourceExcerptsDoNotCreateDiagnostics() {
        BuildLogModel m; m.reset("C:/p","build");
        m.append("main.c:12: warning: format mismatch\n   12 | printf(\"error: bad\");\n      | ^~~~~~~~~~~~~~~~~~~~\n",true);
        QCOMPARE(m.warningCount(),1); QCOMPARE(m.errorCount(),0);
        QCOMPARE(m.conciseEntries().size(),1); QCOMPARE(m.detailedEntries().size(),3);
    }
    void localizedPreflightFailure_data() {
        QTest::addColumn<QString>("message");
        QTest::newRow("start") << QStringLiteral("无法启动 make：找不到文件");
        QTest::newRow("generate") << QStringLiteral("Makefile 生成失败：目录不可写");
        QTest::newRow("cancel") << QStringLiteral("已手动终止构建");
        QTest::newRow("error") << QStringLiteral("错误: 工具链未配置");
    }
    void localizedPreflightFailure() {
        QFETCH(QString,message); BuildLogModel m; m.reset("C:/p","build");
        m.append(message+"\n",true); QCOMPARE(m.errorCount(),1);
        QCOMPARE(m.conciseEntries()[0].text,message);
        auto r=m.finish(false,-1,100); QCOMPARE(r.errorCount,1);
        QVERIFY(r.summary.contains(QStringLiteral("错误 1")));
    }
    void toolFailures_data() {
        QTest::addColumn<QString>("output");
        QTest::newRow("undefined") << "arm-none-eabi-ld: build/main.o: undefined reference to `foo'\ncollect2: error: ld returned 1 exit status\nmake: *** [app.elf] Error 1\n";
        QTest::newRow("overflow") << "arm-none-eabi-ld: region `FLASH' overflowed by 24 bytes\ncollect2: error: ld returned 1 exit status\n";
        QTest::newRow("make") << "make: *** No rule to make target 'missing.c'. Stop.\n";
        QTest::newRow("missing") << "arm-none-eabi-gcc: fatal error: missing.c: No such file or directory\ncompilation terminated.\nmake: *** [main.o] Error 1\n";
    }
    void toolFailures() {
        QFETCH(QString, output); BuildLogModel m; m.reset("C:/p", "build");
        m.append(output, true); QCOMPARE(m.errorCount(), 1);
        auto r = m.finish(false, 2, 250); QVERIFY(!r.success); QCOMPARE(r.errorCount, 1);
        QVERIFY(m.diagnostics()[0].filePath.isEmpty());
    }
    void resultsAndReset() {
        BuildLogModel m; m.reset("C:/p", "build");
        m.append("a.c:1: warning: unused", true);
        auto r=m.finish(true,0,1250); QVERIFY(r.success); QCOMPARE(r.warningCount,1);
        QVERIFY(r.summary.contains("构建完成")); QVERIFY(r.summary.contains("1"));
        QVERIFY(m.conciseEntries().last().success);
        const auto n=m.conciseEntries().size(); QCOMPARE(m.finish(false,1,0).summary,r.summary); QCOMPARE(m.conciseEntries().size(),n);
        m.reset("C:/p","clean"); QVERIFY(m.diagnostics().isEmpty());
        QVERIFY(m.finish(true,0,50).summary.contains("清理完成"));
        m.reset("C:/p","build"); QVERIFY(!m.finish(false,-1,50).success); QCOMPARE(m.errorCount(),1);
        m.reset("C:/p","build"); m.append("a.c:1: error: bad\n",true); QVERIFY(!m.finish(true,0,50).success);
        m.reset("C:/p","build"); QVERIFY(!m.finish(true,3,50).success); QCOMPARE(m.errorCount(),1);
    }
    void stagesSizeAndNoWork() {
        BuildLogModel m; m.reset("C:/p","build");
        m.append("[BUILD:CXX] \"Src/a.cpp\"\n[BUILD:AS] \"startup.s\"\n[BUILD:LD] \"build/app.elf\"\n[BUILD:HEX] \"build/app.hex\"\n[BUILD:BIN] \"build/app.bin\"\narm-none-eabi-gcc -IInc main.c\n   text data bss dec hex filename\n  100 20 30 150 96 build/app.elf\nmake: Nothing to be done for 'all'.\n",false);
        QCOMPARE(m.conciseEntries().size(),7);
        QCOMPARE(m.conciseEntries()[0].text,QString("compiling a.cpp..."));
        QVERIFY(m.conciseEntries()[1].text.contains("assembling"));
        QVERIFY(m.conciseEntries()[5].text.contains("Flash: 120"));
        QVERIFY(m.conciseEntries()[5].text.contains("RAM: 50"));
        QVERIFY(m.conciseEntries()[5].text.contains("text: 100"));
        QVERIFY(m.conciseEntries().last().text.contains("已是最新"));
        QCOMPARE(BuildLogModel::color(DiagnosticSeverity::Warning),QColor("#f9e2af"));
        QVERIFY(BuildLogModel::color(DiagnosticSeverity::Error)!=BuildLogModel::color(DiagnosticSeverity::Info));
    }
};
QTEST_GUILESS_MAIN(BuildLogModelTest)
#include "BuildLogModelTest.moc"
