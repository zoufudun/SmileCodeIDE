#include <QtTest>
#include <QSignalSpy>
#include <QTextEdit>
#include <QTreeWidget>
#include "codeeditor.h"

#ifdef QT_NO_DEBUG
#error This regression must run with Qt assertions enabled.
#endif

class EditorStartupDebugTest : public QObject {
    Q_OBJECT
private slots:
    void createsEditorAndReappliesTheme() {
        CodeEditor editor;
        QVERIFY(editor.currentEditor());
        for (const auto &theme : {"light", "dark", "light", "dark"}) editor.applyTheme(theme);
        editor.setText("void first(void) {}\n\nvoid second(int value) {}\n");
        editor.currentEditor()->setCursorPosition(2, 5);
        editor.updateVariableList();
        QTest::qWait(250);
        QVERIFY(editor.findChild<QTreeWidget *>("outlineTreeWidget"));
        editor.currentEditor()->setModified(false);
    }
};

QTEST_MAIN(EditorStartupDebugTest)
#include "EditorStartupDebugTest.moc"
