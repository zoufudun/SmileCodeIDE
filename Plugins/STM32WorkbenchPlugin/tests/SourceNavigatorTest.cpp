#include <QtTest>
#include <QAbstractButton>
#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTextBlock>
#include "../UI/SourceNavigator.h"
#ifdef SOURCE_NAVIGATOR_HOST_TESTS
#include "mainwindow.h"
#include <QTimer>
#endif

class NavigationHost : public QWidget {
    Q_OBJECT
public:
    QString path;
    int line = 0;
    int column = 0;
    bool accepted = true;
    Q_INVOKABLE bool openSourceLocation(QString file, int row, int col) {
        path = file;
        line = row;
        column = col;
        return accepted;
    }
};

class SourceNavigatorTest : public QObject {
    Q_OBJECT
private slots:
#ifdef SOURCE_NAVIGATOR_HOST_TESTS
    void realHostReusesModifiedBufferAndHonorsSaveDiscardCancel() {
        QTemporaryDir dir;
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
        const QString firstPath = dir.filePath("first.c");
        const QString secondPath = dir.filePath("second.c");
        QFile first(firstPath);
        QVERIFY(first.open(QIODevice::WriteOnly));
        first.write("first\noriginal\n");
        first.close();
        QFile second(secondPath);
        QVERIFY(second.open(QIODevice::WriteOnly));
        second.write("second\nother\n");
        second.close();
        MainWindow host;
        auto *codeEditor = host.findChild<CodeEditor *>();
        QVERIFY(codeEditor);
        auto *editor = codeEditor->currentEditor();
        QVERIFY(editor);
        editor->setModified(false);
        QVERIFY(SourceNavigator::open(&host, &host, firstPath, 2, 2));
        editor->setText("first\nunsaved change\n");
        editor->setModified(true);
        QVERIFY(SourceNavigator::open(&host, &host, firstPath, 2, 5));
        QCOMPARE(editor->text(), QString("first\nunsaved change\n"));
        QVERIFY(editor->isModified());
        int row = -1;
        int column = -1;
        editor->getCursorPosition(&row, &column);
        QCOMPARE(row, 1);
        QCOMPARE(column, 4);
        QTimer::singleShot(0, &host, [] {
            if (auto *prompt = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                prompt->button(QMessageBox::Cancel)->click();
            }
        });
        QVERIFY(!SourceNavigator::open(&host, &host, secondPath, 1));
        QCOMPARE(editor->text(), QString("first\nunsaved change\n"));
        QVERIFY(editor->isModified());
        QTimer::singleShot(0, &host, [] {
            if (auto *prompt = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                prompt->button(QMessageBox::Discard)->click();
            }
        });
        QVERIFY(SourceNavigator::open(&host, &host, secondPath, 2, 3));
        QCOMPARE(editor->text(), QString("second\nother\n"));
        QVERIFY(!editor->isModified());
        editor->getCursorPosition(&row, &column);
        QCOMPARE(row, 1);
        QCOMPARE(column, 2);
        QVERIFY(first.open(QIODevice::ReadOnly));
        QCOMPARE(first.readAll(), QByteArray("first\noriginal\n"));
        first.close();
        editor->setText("second\nsaved change\n");
        editor->setModified(true);
        QTimer::singleShot(0, &host, [] {
            if (auto *prompt = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                prompt->button(QMessageBox::Save)->click();
            }
        });
        QVERIFY(SourceNavigator::open(&host, &host, firstPath, 1));
        QCOMPARE(editor->text(), QString("first\noriginal\n"));
        QVERIFY(second.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(second.readAll(), QByteArray("second\nsaved change\n"));
    }
#endif
    void invokesHostWithSourceLocation() {
        QWidget parent;
        NavigationHost host;
        QVERIFY(SourceNavigator::open(&parent, &host, "source.c", 12, 4));
        QCOMPARE(host.path, QString("source.c"));
        QCOMPARE(host.line, 12);
        QCOMPARE(host.column, 4);
        QVERIFY(parent.findChildren<QDialog *>().isEmpty());
    }

    void respectsHostCancellation() {
        QWidget parent;
        NavigationHost host;
        host.accepted = false;
        QVERIFY(!SourceNavigator::open(&parent, &host, "source.c", 12));
        QCOMPARE(host.column, 0);
        QVERIFY(parent.findChildren<QDialog *>().isEmpty());
    }

    void showsReadOnlyPreviewAtRequestedLocation() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QString::fromUtf8("测试 source.c"));
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly));
        const QByteArray contents("first\nsecond line\nthird\n");
        source.write(contents);
        source.close();
        QWidget parent;
        QWidget oldHost;
        QVERIFY(SourceNavigator::open(&parent, &oldHost, path, 2, 4));
        auto *preview = parent.findChild<QDialog *>("sourcePreviewDialog");
        QVERIFY(preview);
        QVERIFY(!preview->isModal());
        auto *editor = preview->findChild<QPlainTextEdit *>();
        QVERIFY(editor);
        QVERIFY(editor->isReadOnly());
        QCOMPARE(editor->toPlainText(), QString::fromUtf8(contents));
        QCOMPARE(editor->textCursor().blockNumber(), 1);
        QCOMPARE(editor->textCursor().positionInBlock(), 3);
        QCOMPARE(editor->extraSelections().size(), 1);
        QVERIFY(preview->findChild<QLabel *>()->text().contains(path));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), contents);
    }

    void clampsPreviewLocation() {
        QTemporaryDir dir;
        QFile source(dir.filePath("source.c"));
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("short\nlast");
        source.close();
        QWidget parent;
        QVERIFY(SourceNavigator::open(&parent, nullptr, source.fileName(), 1000, 1000));
        auto *editor = parent.findChild<QPlainTextEdit *>();
        QVERIFY(editor);
        QCOMPARE(editor->textCursor().blockNumber(), 1);
        QCOMPARE(editor->textCursor().positionInBlock(), 4);
    }

    void reusesParentPreviewAndUpdatesSourceLocation() {
        QTemporaryDir dir;
        const QString firstPath = dir.filePath("first.c");
        const QString secondPath = dir.filePath("second.c");
        QFile first(firstPath);
        QVERIFY(first.open(QIODevice::WriteOnly));
        first.write("first\nsecond\nthird");
        first.close();
        QFile second(secondPath);
        QVERIFY(second.open(QIODevice::WriteOnly));
        second.write("other\nfile content");
        second.close();
        QWidget parent;
        QVERIFY(SourceNavigator::open(&parent, nullptr, firstPath, 1));
        auto *preview = parent.findChild<QDialog *>("sourcePreviewDialog");
        QVERIFY(preview);
        auto *editor = preview->findChild<QPlainTextEdit *>();
        QVERIFY(editor);
        QVERIFY(SourceNavigator::open(&parent, nullptr, firstPath, 3, 2));
        QCOMPARE(parent.findChildren<QDialog *>("sourcePreviewDialog").size(), 1);
        QCOMPARE(parent.findChild<QDialog *>("sourcePreviewDialog"), preview);
        QCOMPARE(editor->textCursor().blockNumber(), 2);
        QCOMPARE(editor->textCursor().positionInBlock(), 1);
        QCOMPARE(editor->extraSelections().first().cursor.blockNumber(), 2);
        QVERIFY(SourceNavigator::open(&parent, nullptr, secondPath, 2, 4));
        QCOMPARE(parent.findChildren<QDialog *>("sourcePreviewDialog").size(), 1);
        QCOMPARE(editor->toPlainText(), QString("other\nfile content"));
        QCOMPARE(editor->textCursor().blockNumber(), 1);
        QCOMPARE(editor->textCursor().positionInBlock(), 3);
        QCOMPARE(editor->extraSelections().first().cursor.blockNumber(), 1);
        QVERIFY(editor->isReadOnly());
        QVERIFY(preview->findChild<QLabel *>()->text().contains(secondPath));
        QVERIFY(preview->windowTitle().contains("second.c"));
        NavigationHost host;
        host.accepted = false;
        QVERIFY(!SourceNavigator::open(&parent, &host, firstPath, 1));
        QCOMPARE(editor->toPlainText(), QString("other\nfile content"));
        QWidget otherParent;
        QVERIFY(SourceNavigator::open(&otherParent, nullptr, firstPath, 1));
        QVERIFY(otherParent.findChild<QDialog *>("sourcePreviewDialog") != preview);
    }

    void clampsUnspecifiedPreviewLocationToStart() {
        QTemporaryDir dir;
        QFile source(dir.filePath("source.c"));
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("first\nlast");
        source.close();
        QWidget parent;
        QVERIFY(SourceNavigator::open(&parent, nullptr, source.fileName(), 0));
        auto *editor = parent.findChild<QPlainTextEdit *>();
        QVERIFY(editor);
        QCOMPARE(editor->textCursor().blockNumber(), 0);
        QCOMPARE(editor->textCursor().positionInBlock(), 0);
    }

    void missingSourceHasNonblockingOwnedError() {
        QTemporaryDir dir;
        const QString missing = dir.filePath("missing.c");
        QWidget parent;
        QVERIFY(!SourceNavigator::open(&parent, nullptr, missing, 3));
        auto *error = parent.findChild<QMessageBox *>("sourceNavigationError");
        QVERIFY(error);
        QVERIFY(error->text().contains(missing));
        QVERIFY(!error->isModal());
        QVERIFY(!parent.findChild<QPlainTextEdit *>());
    }
};

QTEST_MAIN(SourceNavigatorTest)
#include "SourceNavigatorTest.moc"
