#include <QtTest/QtTest>
#include <QApplication>
#include <QMainWindow>
#include <QTreeView>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTabWidget>
#include <QPushButton>
#include <cmath>
#include "idetheme.h"
#include "codeeditor.h"

class IdeThemeQualityTest : public QObject {
    Q_OBJECT

private:
    static double getRelativeLuminance(const QColor &c) {
        auto channel = [](double val) -> double {
            val /= 255.0;
            return (val <= 0.03928) ? (val / 12.92) : std::pow((val + 0.055) / 1.055, 2.4);
        };
        double r = channel(c.red());
        double g = channel(c.green());
        double b = channel(c.blue());
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }

    static double calculateContrastRatio(const QColor &c1, const QColor &c2) {
        double l1 = getRelativeLuminance(c1);
        double l2 = getRelativeLuminance(c2);
        if (l1 < l2) std::swap(l1, l2);
        return (l1 + 0.05) / (l2 + 0.05);
    }

private slots:
    void testThemeListAndPaletteIntegrity() {
        const QStringList themes = IdeTheme::themeList();
        QCOMPARE(themes.size(), 8);
        QVERIFY(themes.contains("dark"));
        QVERIFY(themes.contains("cyberneon"));
        QVERIFY(themes.contains("obsidiangold"));
        QVERIFY(themes.contains("dracula"));
        QVERIFY(themes.contains("nord"));
        QVERIFY(themes.contains("vue"));
        QVERIFY(themes.contains("light"));
        QVERIFY(themes.contains("solarizedlight"));

        for (const QString &id : themes) {
            const IdeTheme::ThemePalette p = IdeTheme::paletteFor(id);
            QCOMPARE(p.id, id);
            QVERIFY(!p.displayName.isEmpty());
            QVERIFY(!p.icon.isEmpty());

            // 验证深浅属性匹配
            if (id == "light" || id == "solarizedlight") {
                QVERIFY(!p.isDark);
            } else {
                QVERIFY(p.isDark);
            }

            // 验证核心背景与文字色值均为有效颜色
            QVERIFY(QColor(p.windowBg).isValid());
            QVERIFY(QColor(p.panelBg).isValid());
            QVERIFY(QColor(p.editorBg).isValid());
            QVERIFY(QColor(p.cardBg).isValid());
            QVERIFY(QColor(p.inputBg).isValid());
            QVERIFY(QColor(p.textMain).isValid());
            QVERIFY(QColor(p.textSub).isValid());
            QVERIFY(QColor(p.border).isValid());
            QVERIFY(QColor(p.accent).isValid());
            QVERIFY(QColor(p.selectionBg).isValid());
            QVERIFY(QColor(p.selectionText).isValid());
            QVERIFY(QColor(p.tooltipBg).isValid());
            QVERIFY(QColor(p.tooltipText).isValid());

            // 验证语法高亮色系有效
            QVERIFY(p.synKeyword.isValid());
            QVERIFY(p.synString.isValid());
            QVERIFY(p.synNumber.isValid());
            QVERIFY(p.synComment.isValid());
            QVERIFY(p.synFunction.isValid());
            QVERIFY(p.synGlobalClass.isValid());
        }
    }

    void testWcagContrastRatios() {
        const QStringList themes = IdeTheme::themeList();
        for (const QString &id : themes) {
            const IdeTheme::ThemePalette p = IdeTheme::paletteFor(id);
            QColor fgMain(p.textMain);
            QColor fgSub(p.textSub);
            QColor bgEditor(p.editorBg);
            QColor bgWindow(p.windowBg);
            QColor bgCard(p.cardBg);
            QColor bgTooltip(p.tooltipBg);
            QColor fgTooltip(p.tooltipText);

            // 1. 主文字 vs 编辑器底色 (要求 > 7.0:1 AAA 级高对比，保证代码阅读最舒适)
            double crEditor = calculateContrastRatio(fgMain, bgEditor);
            QVERIFY2(crEditor >= 7.0, qPrintable(QString("主题 %1 主文字在编辑器中的对比度 %2:1 低于 7.0:1").arg(id).arg(crEditor)));

            // 2. 主文字 vs 窗口底色 (要求 > 4.5:1 AA 级)
            double crWindow = calculateContrastRatio(fgMain, bgWindow);
            QVERIFY2(crWindow >= 4.5, qPrintable(QString("主题 %1 主文字在窗口中的对比度 %2:1 低于 4.5:1").arg(id).arg(crWindow)));

            // 3. 次级文字 vs 窗口与卡片底色 (要求 > 3.0:1，杜绝浅底浅灰或深底深灰看不清)
            double crSubWin = calculateContrastRatio(fgSub, bgWindow);
            QVERIFY2(crSubWin >= 3.0, qPrintable(QString("主题 %1 次级文字对比度 %2:1 低于 3.0:1").arg(id).arg(crSubWin)));

            // 4. ToolTip 悬浮气泡文字 vs 气泡底色 (要求 > 4.5:1)
            double crTooltip = calculateContrastRatio(fgTooltip, bgTooltip);
            QVERIFY2(crTooltip >= 4.5, qPrintable(QString("主题 %1 ToolTip 对比度 %2:1 低于 4.5:1").arg(id).arg(crTooltip)));
        }
    }

    void testGeneratedStyleSheetCompleteness() {
        const QStringList themes = IdeTheme::themeList();
        for (const QString &id : themes) {
            QString qss = IdeTheme::generateStyleSheet(id);
            QVERIFY(!qss.isEmpty());

            // 验证不存在未替换的占位符
            QVERIFY2(!qss.contains("{{"), qPrintable(QString("主题 %1 生成的 QSS 存在未替换占位符 {{").arg(id)));
            QVERIFY2(!qss.contains("}}"), qPrintable(QString("主题 %1 生成的 QSS 存在未替换占位符 }}").arg(id)));

            // 验证核心组件选择器均已包含
            QVERIFY(qss.contains("QToolTip"));
            QVERIFY(qss.contains("QMenuBar"));
            QVERIFY(qss.contains("QMenu"));
            QVERIFY(qss.contains("QToolBar"));
            QVERIFY(qss.contains("QToolButton"));
            QVERIFY(qss.contains("QPushButton"));
            QVERIFY(qss.contains("QSplitter"));
            QVERIFY(qss.contains("QDockWidget"));
            QVERIFY(qss.contains("QTabWidget"));
            QVERIFY(qss.contains("QTabBar"));
            QVERIFY(qss.contains("QTreeView"));
            QVERIFY(qss.contains("QTreeView::branch"));
            QVERIFY(qss.contains("QHeaderView"));
            QVERIFY(qss.contains("QScrollBar"));
            QVERIFY(qss.contains("QLineEdit"));
            QVERIFY(qss.contains("QTextEdit"));
            QVERIFY(qss.contains("QComboBox"));
            QVERIFY(qss.contains("QGroupBox"));
            QVERIFY(qss.contains("QProgressBar"));
            QVERIFY(qss.contains("QStatusBar"));
            QVERIFY(qss.contains("QWidget#FindReplaceWidget"));
            QVERIFY(qss.contains("QWidget#projectSearchPanel"));
        }
    }

    void testApplyThemeToWidgetsWithoutCrash() {
        QWidget container;
        auto *layout = new QVBoxLayout(&container);
        auto *edit = new QLineEdit(&container);
        auto *combo = new QComboBox(&container);
        combo->addItems({"Alpha", "Beta", "Gamma"});
        auto *btn = new QPushButton("Action", &container);
        auto *tab = new QTabWidget(&container);
        tab->addTab(new QWidget(), "Tab 1");
        tab->addTab(new QWidget(), "Tab 2");

        layout->addWidget(edit);
        layout->addWidget(combo);
        layout->addWidget(btn);
        layout->addWidget(tab);
        container.resize(400, 300);

        const QStringList themes = IdeTheme::themeList();
        for (const QString &id : themes) {
            QString qss = IdeTheme::generateStyleSheet(id);
            container.setStyleSheet(qss);
            container.show();
            QCoreApplication::processEvents();
        }
        QVERIFY(true);
    }

    void testCodeEditorThemeSwitching() {
        CodeEditor editor;
        editor.resize(600, 400);
        editor.show();
        QCoreApplication::processEvents();

        const QStringList themes = IdeTheme::themeList();
        // 往返切换测试：深色 -> 浅色 -> 深色
        for (const QString &id : themes) {
            editor.applyTheme(id);
            QCoreApplication::processEvents();
            QVERIFY(editor.currentEditor() != nullptr);
        }

        // 再次切回纯白曜石并校验
        editor.applyTheme("light");
        QCoreApplication::processEvents();
        QVERIFY(!editor.isDarkTheme());

        // 切回极客钛金
        editor.applyTheme("dark");
        QCoreApplication::processEvents();
        QVERIFY(editor.isDarkTheme());
    }
};

QTEST_MAIN(IdeThemeQualityTest)
#include "IdeThemeQualityTest.moc"

