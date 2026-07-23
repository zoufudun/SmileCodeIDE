#include <QCoreApplication>
#include <QDebug>
#include <QRegularExpression>


int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);

  QString code = R"(
// 添加一个辅助方法来查找分割器中的第一个编辑器
QsciScintilla *CodeEditor::findFirstEditor(QSplitter *splitter) {
)";

  // 1. 普通函数正则
  // 匹配: [返回类型] [类名::]函数名(参数) [修饰符] {
  // 注意：支持多行匹配
  QRegularExpression functionRegex(
      R"((?:^|\n)\s*(?:template\s*<[^>]*>\s*)?(?:(?:static|virtual|inline|explicit|friend|constexpr)\s+)*(?:[\w<>:.*&]+\s*)+?(\w+(?:::\w+)*)\s*\([^)]*\)\s*(?:const|override|final|noexcept|try|\s)*\{)");

  if (!functionRegex.isValid()) {
    qDebug() << "Regex invalid:" << functionRegex.errorString();
    return 1;
  }

  QRegularExpressionMatchIterator i = functionRegex.globalMatch(code);
  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QString funcName = match.captured(1);
    qDebug() << "MATCH:" << funcName;
  }

  return 0;
}
