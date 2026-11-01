#include <QtTest>
#include <QAction>
#include <QToolBar>
#include <QTemporaryDir>
#include <QFile>
#include <QMessageBox>
#include <QAbstractButton>
#include <QTimer>
#include <QSettings>
#include <QMenuBar>
#include "codeeditor.h"
class EditorCommandsTest : public QObject {
 Q_OBJECT
private slots:
 void sharedMenus() {
  CodeEditor e; QMenuBar bar; e.createEditorMenus(&bar);
  QVERIFY(e.editorAction("findInProject")); QSignalSpy search(&e,&CodeEditor::projectSearchRequested);
  e.editorAction("findInProject")->trigger(); QCOMPARE(search.count(),1);
  QVERIFY(e.editorAction("goToDefinition")->shortcut() == QKeySequence("F12"));
  QVERIFY(e.editorAction("indentSelection")->shortcut() == QKeySequence("Ctrl+]"));
 }
 void mixedCommentsAndUnindent() {
  CodeEditor e; e.setText("  //old\n  next\nuntouched\n"); auto editor=e.currentEditor(); editor->setSelection(0,0,2,0);
  e.editorAction("toggleLineComment")->trigger(); QCOMPARE(e.text(),QString("  ////old\n  //next\nuntouched\n"));
  e.editorAction("toggleLineComment")->trigger(); QCOMPARE(e.text(),QString("  //old\n  next\nuntouched\n"));
  e.editorAction("unindentSelection")->trigger(); QCOMPARE(e.text(),QString("//old\nnext\nuntouched\n")); editor->undo(); QCOMPARE(e.text(),QString("  //old\n  next\nuntouched\n"));
 }
 void liveBufferSignals() {
  CodeEditor e; QSignalSpy buffer(&e,SIGNAL(currentBufferChanged(QString,QString))); QVERIFY(buffer.isValid());
  e.setText("int value;\n"); QVERIFY(buffer.count()>0); QCOMPARE(buffer.last().at(1).toString(),e.text());
  QVERIFY(e.metaObject()->indexOfSignal("projectReferencesRequested(QString)") >= 0);
 }
 void cancelledNavigationPreservesEdits() {
  QTemporaryDir dir; auto a=dir.filePath("a.c"),b=dir.filePath("b.c");
  for(const QString &path: {a,b}) { QFile f(path); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("saved\n"); }
  CodeEditor e; QVERIFY(e.openFile(a)); e.currentEditor()->insertAt("modified",0,0);
  const QString edited=e.text(); QVERIFY(e.currentEditor()->isModified());
  QTimer::singleShot(0, [] { auto box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if(box) box->button(QMessageBox::Cancel)->click(); });
  QVERIFY(!e.openFile(b)); QCOMPARE(e.text(),edited); QCOMPARE(e.currentFilePath(),QFileInfo(a).canonicalFilePath());
  QTimer::singleShot(0, [] { auto box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if(box) box->button(QMessageBox::Save)->click(); });
  QVERIFY(e.openFile(b)); QFile file(a); QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text)); QCOMPARE(QString::fromUtf8(file.readAll()),edited);
 }
 void doxygenVoidAndFileTemplate() {
  CodeEditor e; e.setText("void reset(void);\n"); e.currentEditor()->setCursorPosition(0,0); e.editorAction("insertDoxygenComment")->trigger();
  QVERIFY(!e.text().contains("@param")); QVERIFY(!e.text().contains("@return"));
  QSettings settings("PhudonTools","Settings"); auto before=settings.value("Doxygen/fileTemplate");
  settings.setValue("Doxygen/fileTemplate", "/** ${file} ${date} ${author} */\n");
  QTemporaryDir dir; QString path=dir.filePath("example.c"); QVERIFY(e.saveFile(path));
  e.editorAction("insertDoxygenFileComment")->trigger(); QVERIFY(e.text().startsWith("/** example.c "));
  if(before.isValid()) settings.setValue("Doxygen/fileTemplate",before); else settings.remove("Doxygen/fileTemplate");
 }

 void toolbarRemoved() { CodeEditor e; QVERIFY(e.findChildren<QToolBar *>().isEmpty()); }
 void selectionIndent() {
  CodeEditor e; e.setText("one\ntwo\nthree\n"); auto s=e.currentEditor(); s->setSelection(0,0,2,0);
  auto a=e.findChild<QAction *>("indentSelection"); QVERIFY(a); a->trigger();
  QCOMPARE(e.text(), QString("    one\n    two\nthree\n")); s->undo(); QCOMPARE(e.text(),QString("one\ntwo\nthree\n"));
 }
 void commentsRoundtrip() {
  CodeEditor e; e.setText("  a();\n    b();\n"); auto s=e.currentEditor(); s->setSelection(0,0,2,0);
  auto a=e.findChild<QAction *>("toggleLineComment"); QVERIFY(a); a->trigger(); QCOMPARE(e.text(),QString("  //a();\n    //b();\n")); a->trigger(); QCOMPARE(e.text(),QString("  a();\n    b();\n"));
  s->setSelection(0,2,0,6); a=e.findChild<QAction *>("toggleBlockComment"); QVERIFY(a); a->trigger(); QCOMPARE(e.text(),QString("  /*a();*/\n    b();\n")); a->trigger(); QCOMPARE(e.text(),QString("  a();\n    b();\n"));
 }
 void unnamedParameters() {
  CodeEditor e; e.setText("Widget resolve(const Widget, struct Thing, unsigned count);\n"); e.currentEditor()->setCursorPosition(0,0);
  e.editorAction("insertDoxygenComment")->trigger(); QVERIFY(!e.text().contains("@param Widget")); QVERIFY(!e.text().contains("@param Thing")); QVERIFY(e.text().contains("@param count"));
 }
 void doxygenParameters() {
  CodeEditor e; e.setText("int send(\n const char *data,\n unsigned count);\n"); e.currentEditor()->setCursorPosition(0,0);
  auto a=e.findChild<QAction *>("insertDoxygenComment"); QVERIFY(a); a->trigger(); QVERIFY(e.text().contains("@param data")); QVERIFY(e.text().contains("@param count")); QVERIFY(e.text().contains("@return"));
 }
 void navigationHistory() {
  QTemporaryDir dir; QString a=dir.filePath("a.c"),b=dir.filePath("b.c"),c=dir.filePath("c.c");
  for(const QString &p: {a,b,c}) { QFile f(p); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("one\ntwo\nthree\n"); }
  CodeEditor e; QVERIFY(e.openFile(a)); e.currentEditor()->setCursorPosition(1,2); e.openFileAndGoTo(b,3,2);
  auto back=e.findChild<QAction *>("navigateBack"); auto forward=e.findChild<QAction *>("navigateForward"); QVERIFY(back); QVERIFY(forward); back->trigger();
  int l,p; e.currentEditor()->getCursorPosition(&l,&p); QCOMPARE(l,1); QCOMPARE(p,2); QVERIFY(forward->isEnabled()); e.openFileAndGoTo(c,1,1); QVERIFY(!forward->isEnabled());
 }
};
QTEST_MAIN(EditorCommandsTest)
#include "EditorCommandsTest.moc"

