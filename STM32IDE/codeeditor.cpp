/*
 * @Description:
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-05 21:44:22
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-08-11 14:23:04
 */

#include "codeeditor.h"
#include <QAction>      // 添加动作头文件
#include <QApplication> // 添加此行以使用QStyle
#include <QFile>
#include <QFileInfo>
#include <QIcon>  // 添加图标头文件
#include <QLabel> // 添加标签头文件
#include <QMessageBox>
#include <QPair> // 添加QPair头文件
#include <QRegExp>
#include <QRegularExpression> // Add this line to include
#include <QStack>
#include <QStyle> // Add this for QStyle class
#include <QTextStream>
#include <QTimer>   // 添加定时器头文件
#include <QToolBar> // 添加工具栏头文件
#include <QVBoxLayout>
#include <vector> // Add this for std::vector

CodeEditor::CodeEditor(QWidget *parent)
    : QWidget(parent), m_currentEditor(nullptr), m_apiCPP(nullptr),
      m_functionList(nullptr), m_isDarkTheme(false) // Add this initialization
{
  // 创建主布局
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  // 创建工具栏
  createToolBar();
  layout->addWidget(m_toolBar);

  // 创建主分割器
  m_mainSplitter = new QSplitter(Qt::Horizontal, this);
  layout->addWidget(m_mainSplitter);

  // 创建函数列表控件 - 移到主分割器创建之后
  createFunctionList();

  // 创建C++词法分析器
  m_lexerCPP = new QsciLexerCPP();

  // 创建第一个编辑器
  QsciScintilla *editor = new QsciScintilla(this);
  m_editors.append(editor);
  m_currentEditor = editor;
  m_mainSplitter->addWidget(editor);

  // 设置编辑器属性
  setupEditor(editor);

  // 设置自动补全
  setupAutoCompletion(editor);

  // 设置示例代码
  editor->setText(
      "// STM32 代码编辑器\n#include <stdint.h>\n\nint main(void) {\n    // "
      "初始化代码\n    while(1) {\n        // 主循环\n    }\n    return 0;\n}");

  // 连接信号和槽
  connect(editor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateVariableList);
  connect(editor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateFunctionList);

  // 初始化函数列表
  updateFunctionList();
}

CodeEditor::~CodeEditor() {
  if (m_apiCPP) {
    delete m_apiCPP;
    m_apiCPP = nullptr;
  }

  if (m_lexerCPP) {
    delete m_lexerCPP;
    m_lexerCPP = nullptr;
  }
}

QsciScintilla *CodeEditor::currentEditor() const { return m_currentEditor; }

QList<QsciScintilla *> CodeEditor::allEditors() const { return m_editors; }

void CodeEditor::setText(const QString &text) {
  if (m_currentEditor) {
    m_currentEditor->setText(text);
  }
}

QString CodeEditor::text() const {
  if (m_currentEditor) {
    return m_currentEditor->text();
  }
  return QString();
}

bool CodeEditor::openFile(const QString &filePath) {
  QFile file(filePath);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // 在当前编辑器中显示文件内容
    if (m_currentEditor) {
      m_currentEditor->setText(content);
      // Trigger highlighting manually after loading file
      highlightRainbowBrackets(m_currentEditor);
      highlightFunctionNames(m_currentEditor, 20); // 20 is FUNCTION_INDICATOR
    }

    // 更新当前文件路径
    m_currentFilePath = filePath;

    // 解析文件中的变量并更新自动补全
    updateVariableList();

    // 更新函数列表
    updateFunctionList();

    return true;
  }
  return false;
}

bool CodeEditor::saveFile(const QString &filePath) {
  if (!m_currentEditor) {
    return false;
  }

  QFile file(filePath);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << m_currentEditor->text();
    file.close();

    // 更新当前文件路径
    m_currentFilePath = filePath;

    return true;
  }
  return false;
}

// void CodeEditor::applyTheme(const QString &themeName)
// {
//     if (themeName == "dark") {
//         // 深色主题
//         for (QsciScintilla* editor : m_editors) {
//             // 设置编辑器背景色和默认文本颜色
//             editor->setColor(QColor("#DCDCDC"));
//             editor->setPaper(QColor("#1E1E1E"));

//             // 设置行号边距颜色
//             editor->setMarginsBackgroundColor(QColor("#1E1E1E"));
//             editor->setMarginsForegroundColor(QColor("#858585"));

//             // 设置折叠边距颜色
//             editor->setFoldMarginColors(QColor("#1E1E1E"),
//             QColor("#1E1E1E"));

//             // 设置选中文本的颜色
//             editor->setSelectionBackgroundColor(QColor("#264F78"));
//             editor->setSelectionForegroundColor(QColor("#FFFFFF"));
//         }

//         // 设置语法高亮颜色
//         m_lexerCPP->setColor(QColor("#569CD6"), QsciLexerCPP::Keyword); //
//         关键字 m_lexerCPP->setColor(QColor("#CE9178"),
//         QsciLexerCPP::DoubleQuotedString); // 字符串
//         m_lexerCPP->setColor(QColor("#CE9178"),
//         QsciLexerCPP::SingleQuotedString); // 字符
//         m_lexerCPP->setColor(QColor("#B5CEA8"), QsciLexerCPP::Number); //
//         数字 m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::Comment);
//         // 注释 m_lexerCPP->setColor(QColor("#608B4E"),
//         QsciLexerCPP::CommentLine); // 行注释
//         m_lexerCPP->setColor(QColor("#C586C0"), QsciLexerCPP::PreProcessor);
//         // 预处理器 m_lexerCPP->setColor(QColor("#4EC9B0"),
//         QsciLexerCPP::GlobalClass); // 类名

//         // 设置背景色
//         m_lexerCPP->setPaper(QColor("#1E1E1E"));

//         // 设置默认字体
//         QFont font("Consolas", 10);
//         m_lexerCPP->setFont(font);
//     } else if (themeName == "light") {
//         // 浅色主题
//         for (QsciScintilla* editor : m_editors) {
//             // 设置编辑器背景色和默认文本颜色
//             editor->setColor(QColor("#000000"));
//             editor->setPaper(QColor("#FFFFFF"));

//             // 设置行号边距颜色
//             editor->setMarginsBackgroundColor(QColor("#F0F0F0"));
//             editor->setMarginsForegroundColor(QColor("#2B91AF"));

//             // 设置折叠边距颜色
//             editor->setFoldMarginColors(QColor("#F0F0F0"),
//             QColor("#F0F0F0"));

//             // 设置选中文本的颜色
//             editor->setSelectionBackgroundColor(QColor("#ADD6FF"));
//             editor->setSelectionForegroundColor(QColor("#000000"));
//         }

//         // 设置语法高亮颜色
//         m_lexerCPP->setColor(QColor("#0000FF"), QsciLexerCPP::Keyword); //
//         关键字 m_lexerCPP->setColor(QColor("#A31515"),
//         QsciLexerCPP::DoubleQuotedString); // 字符串
//         m_lexerCPP->setColor(QColor("#A31515"),
//         QsciLexerCPP::SingleQuotedString); // 字符
//         m_lexerCPP->setColor(QColor("#098658"), QsciLexerCPP::Number); //
//         数字 m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::Comment);
//         // 注释 m_lexerCPP->setColor(QColor("#008000"),
//         QsciLexerCPP::CommentLine); // 行注释
//         m_lexerCPP->setColor(QColor("#800000"), QsciLexerCPP::PreProcessor);
//         // 预处理器 m_lexerCPP->setColor(QColor("#267F99"),
//         QsciLexerCPP::GlobalClass); // 类名

//         // 设置背景色
//         m_lexerCPP->setPaper(QColor("#FFFFFF"));

//         // 设置默认字体
//         QFont font("Consolas", 10);
//         m_lexerCPP->setFont(font);
//     }
//     // 可以添加更多主题...
// }

void CodeEditor::applyTheme(const QString &themeName) {
  if (themeName == "dark") {
    // 深色主题
    for (QsciScintilla *editor : m_editors) {
      // 设置编辑器背景色和默认文本颜色
      editor->setColor(QColor("#DCDCDC"));
      editor->setPaper(QColor("#1E1E1E"));

      // 设置行号边距颜色
      editor->setMarginsBackgroundColor(QColor("#1E1E1E"));
      editor->setMarginsForegroundColor(QColor("#858585"));

      // 设置折叠边距颜色
      editor->setFoldMarginColors(QColor("#1E1E1E"), QColor("#1E1E1E"));

      // 设置选中文本的颜色
      editor->setSelectionBackgroundColor(QColor("#264F78"));
      editor->setSelectionForegroundColor(QColor("#FFFFFF"));

      // Set up Rainbow Brackets for Dark Theme
      setupRainbowBrackets(editor);
    }

    // 设置函数列表深色主题样式
    if (m_functionList) {
      m_functionList->setStyleSheet("QListWidget {"
                                    "    background-color: #252526;"
                                    "    border: 1px solid #3E3E42;"
                                    "    color: #DCDCDC;"
                                    "}"
                                    "QListWidget::item {"
                                    "    padding: 4px;"
                                    "    border-bottom: 1px solid #3E3E42;"
                                    "}"
                                    "QListWidget::item:selected {"
                                    "    background-color: #0078D7;"
                                    "    color: white;"
                                    "}"
                                    "QListWidget::item:alternate {"
                                    "    background-color: #2D2D30;"
                                    "}");

      // 设置函数列表标题标签样式
      if (m_functionList->parentWidget() &&
          m_functionList->parentWidget()->layout()) {
        QLayoutItem *item = m_functionList->parentWidget()->layout()->itemAt(0);
        if (item && item->widget()) {
          QLabel *titleLabel = qobject_cast<QLabel *>(item->widget());
          if (titleLabel) {
            titleLabel->setStyleSheet("font-weight: bold; "
                                      "padding: 5px; "
                                      "color: #DCDCDC; "
                                      "background-color: #2D2D30; "
                                      "border-bottom: 1px solid #3E3E42;");
          }
        }
      }
    }

    // 设置语法高亮颜色
    m_lexerCPP->setColor(QColor("#569CD6"), QsciLexerCPP::Keyword); // 关键字
    m_lexerCPP->setColor(QColor("#CE9178"),
                         QsciLexerCPP::DoubleQuotedString); // 字符串
    m_lexerCPP->setColor(QColor("#CE9178"),
                         QsciLexerCPP::SingleQuotedString);         // 字符
    m_lexerCPP->setColor(QColor("#B5CEA8"), QsciLexerCPP::Number);  // 数字
    m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::Comment); // 注释
    m_lexerCPP->setColor(QColor("#608B4E"),
                         QsciLexerCPP::CommentLine); // 行注释
    m_lexerCPP->setColor(QColor("#C586C0"),
                         QsciLexerCPP::PreProcessor); // 预处理器
    m_lexerCPP->setColor(QColor("#4EC9B0"), QsciLexerCPP::GlobalClass); // 类名

    // 添加函数名颜色设置 - 橙色
    // m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::Identifier); //
    // 标识符(包括函数名)
    // 或者使用更专门的函数名样式
    // m_lexerCPP->setColor(QColor("#DCDCAA"),
    // QsciLexerCPP::FunctionMethodName); // 函数方法名 恢复标识符的默认颜色
    m_lexerCPP->setColor(QColor("#DCDCDC"),
                         QsciLexerCPP::Identifier); // 标识符恢复默认颜色

    // 设置函数名为橙色 - 使用GlobalFunction样式

    // 设置背景色
    m_lexerCPP->setPaper(QColor("#1E1E1E"));

    // 设置默认字体
    QFont font("Consolas", 10);
    m_lexerCPP->setFont(font);

    // 设置自定义函数名高亮
    setupFunctionNameHighlighting(true);
  } else if (themeName == "light") {
    // 浅色主题
    for (QsciScintilla *editor : m_editors) {
      // 设置编辑器背景色和默认文本颜色
      editor->setColor(QColor("#000000"));
      editor->setPaper(QColor("#FFFFFF"));

      // 设置行号边距颜色
      editor->setMarginsBackgroundColor(QColor("#F0F0F0"));
      editor->setMarginsForegroundColor(QColor("#2B91AF"));

      // 设置折叠边距颜色
      editor->setFoldMarginColors(QColor("#F0F0F0"), QColor("#F0F0F0"));

      // 设置选中文本的颜色
      editor->setSelectionBackgroundColor(QColor("#ADD6FF"));
      editor->setSelectionForegroundColor(QColor("#000000"));

      // Set up Rainbow Brackets for Light Theme
      setupRainbowBrackets(editor);
    }

    // 设置函数列表浅色主题样式
    if (m_functionList) {
      m_functionList->setStyleSheet("QListWidget {"
                                    "    background-color: #F5F5F5;"
                                    "    border: 1px solid #DDD;"
                                    "    color: #000000;"
                                    "}"
                                    "QListWidget::item {"
                                    "    padding: 4px;"
                                    "    border-bottom: 1px solid #EEE;"
                                    "}"
                                    "QListWidget::item:selected {"
                                    "    background-color: #0078D7;"
                                    "    color: white;"
                                    "}"
                                    "QListWidget::item:alternate {"
                                    "    background-color: #F9F9F9;"
                                    "}");

      // 设置函数列表标题标签样式
      if (m_functionList->parentWidget() &&
          m_functionList->parentWidget()->layout()) {
        QLayoutItem *item = m_functionList->parentWidget()->layout()->itemAt(0);
        if (item && item->widget()) {
          QLabel *titleLabel = qobject_cast<QLabel *>(item->widget());
          if (titleLabel) {
            titleLabel->setStyleSheet("font-weight: bold; "
                                      "padding: 5px; "
                                      "color: #000000; "
                                      "background-color: #E0E0E0; "
                                      "border-bottom: 1px solid #CCC;");
          }
        }
      }
    }

    // 设置语法高亮颜色
    m_lexerCPP->setColor(QColor("#0000FF"), QsciLexerCPP::Keyword); // 关键字
    m_lexerCPP->setColor(QColor("#A31515"),
                         QsciLexerCPP::DoubleQuotedString); // 字符串
    m_lexerCPP->setColor(QColor("#A31515"),
                         QsciLexerCPP::SingleQuotedString);         // 字符
    m_lexerCPP->setColor(QColor("#098658"), QsciLexerCPP::Number);  // 数字
    m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::Comment); // 注释
    m_lexerCPP->setColor(QColor("#008000"),
                         QsciLexerCPP::CommentLine); // 行注释
    m_lexerCPP->setColor(QColor("#800000"),
                         QsciLexerCPP::PreProcessor); // 预处理器
    m_lexerCPP->setColor(QColor("#267F99"), QsciLexerCPP::GlobalClass); // 类名

    // 添加函数名颜色设置 - 橙色
    m_lexerCPP->setColor(QColor("#000000"),
                         QsciLexerCPP::Identifier); // 标识符(包括函数名)

    // 设置背景色
    m_lexerCPP->setPaper(QColor("#FFFFFF"));

    // 设置默认字体
    QFont font("Consolas", 10);
    m_lexerCPP->setFont(font);

    // 设置自定义函数名高亮
    setupFunctionNameHighlighting(false);
  }
  // 可以添加更多主题...
}

// 添加新方法：设置函数名高亮
void CodeEditor::setupFunctionNameHighlighting(bool isDarkTheme) {
  // 为每个编辑器设置函数名高亮
  for (QsciScintilla *editor : m_editors) {
    // 使用自定义指示器来高亮函数名
    const int FUNCTION_INDICATOR = 20;

    // 设置指示器样式为文本前景色
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, FUNCTION_INDICATOR,
                          QsciScintilla::INDIC_TEXTFORE);

    // 根据主题设置颜色 - 修改为更明显的橙色
    if (isDarkTheme) {
      // 深色主题 - 明亮的橙色 (注意：颜色格式为BGR)
      editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, FUNCTION_INDICATOR,
                            0xAA78DC); // 橙色 #DCDCAA
    } else {
      // 浅色主题 - 暗金色
      editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, FUNCTION_INDICATOR,
                            0x0B6DB8); // 暗金色 #B86D0B
    }

    // 设置指示器透明度和优先级
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, FUNCTION_INDICATOR,
                          255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, FUNCTION_INDICATOR,
                          false); // 不在文本下方显示
    editor->SendScintilla(QsciScintilla::SCI_INDICSETOUTLINEALPHA,
                          FUNCTION_INDICATOR, 255); // 设置轮廓透明度

    // 断开之前的连接并重新连接
    // Do not disconnect all signals, just add ours
    // disconnect(editor, &QsciScintilla::textChanged, nullptr, nullptr);
    connect(editor, &QsciScintilla::textChanged, [this, editor]() {
      // 确保在文本变化后立即高亮函数名
      QTimer::singleShot(
          0, [this, editor]() { highlightFunctionNames(editor, 20); });
    });

    // Initial highlight
    highlightFunctionNames(editor, FUNCTION_INDICATOR);
  }
}

// 修改高亮函数名的方法
void CodeEditor::highlightFunctionNames(QsciScintilla *editor,
                                        int indicatorId) {
  // 清除现有的函数名高亮
  editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(),
                              indicatorId);

  // 获取编辑器文本
  QString text = editor->text();
  qDebug() << "Text length:" << text.length();
  QStringList lines = text.split('\n');

  // 简化的函数名识别正则表达式
  QRegularExpression functionNameRegex(R"(\b([A-Za-z_]\w*)\s*\()");

  // 关键字过滤列表
  QSet<QString> keywords = {
      "if",       "for",       "while",    "switch",   "catch",   "return",
      "else",     "do",        "break",    "continue", "goto",    "sizeof",
      "typedef",  "volatile",  "register", "extern",   "static",  "auto",
      "const",    "struct",    "union",    "enum",     "class",   "template",
      "typename", "namespace", "using",    "try",      "throw",   "new",
      "delete",   "case",      "default",  "public",   "private", "protected",
      "virtual",  "inline",    "explicit", "friend",   "printf",  "scanf",
      "malloc",   "free",      "strlen",   "strcpy",   "strcmp"};

  // 逐行处理
  for (int lineNum = 0; lineNum < lines.size(); lineNum++) {
    QString line = lines[lineNum];
    QString trimmedLine = line.trimmed();

    // 跳过注释行
    if (trimmedLine.startsWith("//") || trimmedLine.startsWith("/*") ||
        trimmedLine.startsWith("*")) {
      continue;
    }

    // 跳过预处理指令
    if (trimmedLine.startsWith("#")) {
      continue;
    }

    // 查找函数调用和定义
    QRegularExpressionMatchIterator matches =
        functionNameRegex.globalMatch(line);
    while (matches.hasNext()) {
      QRegularExpressionMatch match = matches.next();
      QString functionName = match.captured(1);

      // 过滤关键字和常见的C库函数
      if (keywords.contains(functionName)) {
        continue;
      }

      // 检查是否为函数定义或调用
      int startPos = match.capturedStart(1);
      int endPos = match.capturedEnd(1);

      // 计算在整个文档中的位置
      int globalStartPos = 0;
      for (int i = 0; i < lineNum; i++) {
        globalStartPos += lines[i].length() + 1; // +1 for newline
      }
      globalStartPos += startPos;
      int globalEndPos = globalStartPos + functionName.length();

      // 将位置转换为行和列
      int startLine = 0, startCol = 0, endLine = 0, endCol = 0;
      editor->lineIndexFromPosition(globalStartPos, &startLine, &startCol);
      editor->lineIndexFromPosition(globalEndPos, &endLine, &endCol);

      // 高亮函数名
      editor->fillIndicatorRange(startLine, startCol, endLine, endCol,
                                 indicatorId);
    }
  }
}

// 在setupFunctionHighlight函数中添加防抖机制
void CodeEditor::setupFunctionHighlight(QsciScintilla *editor) {
  if (!editor)
    return;

  const int FUNCTION_INDICATOR = 20;

  // // 设置指示器样式
  // editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, FUNCTION_INDICATOR,
  // QsciScintilla::INDIC_TEXTFORE);

  // if (m_isDarkTheme) {
  //     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
  //     FUNCTION_INDICATOR, 0xAA78DC); // 橙色 #DCDCAA
  // } else {
  //     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
  //     FUNCTION_INDICATOR, 0x0B6DB8); // 暗金色 #B86D0B
  // }

  // editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, FUNCTION_INDICATOR,
  // 255); editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER,
  // FUNCTION_INDICATOR, false);
  // editor->SendScintilla(QsciScintilla::SCI_INDICSETOUTLINEALPHA,
  // FUNCTION_INDICATOR, 255);

  // 设置指示器样式和颜色
  editor->indicatorDefine(QsciScintilla::FullBoxIndicator, FUNCTION_INDICATOR);

  // 使用更明显的颜色进行测试
  if (m_isDarkTheme) {
    editor->setIndicatorForegroundColor(QColor("#FFD700"),
                                        FUNCTION_INDICATOR); // 金色，更明显
  } else {
    editor->setIndicatorForegroundColor(QColor("#FF0000"),
                                        FUNCTION_INDICATOR); // 红色，更明显
  }

  // 设置透明度和下划线
  editor->setIndicatorOutlineColor(QColor("#FFD700"), FUNCTION_INDICATOR);
  editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, FUNCTION_INDICATOR,
                        100);
  editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, FUNCTION_INDICATOR,
                        true);

  // 添加调试输出
  qDebug() << "Setting up function highlight for editor with theme:"
           << (m_isDarkTheme ? "dark" : "light");

  // 创建防抖定时器
  static QTimer *highlightTimer = new QTimer();
  highlightTimer->setSingleShot(true);
  highlightTimer->setInterval(500); // 500ms延迟

  // 断开之前的连接
  disconnect(editor, &QsciScintilla::textChanged, nullptr, nullptr);
  disconnect(highlightTimer, &QTimer::timeout, nullptr, nullptr);

  // 连接文本变化信号到防抖定时器
  connect(editor, &QsciScintilla::textChanged,
          [highlightTimer]() { highlightTimer->start(); });

  // 在定时器连接之前添加调试
  connect(highlightTimer, &QTimer::timeout, [this, editor]() {
    qDebug() << "Highlighting functions...";
    highlightFunctionNames(editor, FUNCTION_INDICATOR);
    updateFunctionList();
  });

  // 立即调用一次以测试
  qDebug() << "Initial function highlighting...";
  highlightFunctionNames(editor, FUNCTION_INDICATOR);

  // // 连接定时器超时信号到高亮函数
  // connect(highlightTimer, &QTimer::timeout, [this, editor]() {
  //     highlightFunctionNames(editor, FUNCTION_INDICATOR);
  //     updateFunctionList(); // 同时更新函数列表
  // });

  // // 初始化时高亮函数名
  // highlightFunctionNames(editor, FUNCTION_INDICATOR);
}

void CodeEditor::createSplitView(Qt::Orientation orientation) {
  // if (!m_currentEditor) {
  //     return;
  // }

  // // 创建新的分割器，替换当前编辑器
  // QSplitter* splitter = new QSplitter(orientation);

  // // 获取当前编辑器在主分割器中的索引
  // int index = m_mainSplitter->indexOf(m_currentEditor);

  // // 从主分割器中移除当前编辑器
  // m_currentEditor->setParent(nullptr);

  // // 将当前编辑器添加到新分割器
  // splitter->addWidget(m_currentEditor);

  // // 创建新的编辑器
  QsciScintilla *newEditor = new QsciScintilla();
  m_editors.append(newEditor);

  // 初始化函数列表控件
  if (!m_functionList) {
    m_functionList = new QListWidget();
    m_functionList->setMinimumWidth(200);
    m_mainSplitter->addWidget(m_functionList);
  }

  // // 设置编辑器属性
  // setupEditor(newEditor);

  // // 设置自动补全
  // setupAutoCompletion(newEditor);

  // // 复制当前编辑器的内容到新编辑器
  // newEditor->setText(m_currentEditor->text());

  // // 将新编辑器添加到分割器
  // splitter->addWidget(newEditor);

  // // 将分割器添加到主分割器
  // m_mainSplitter->insertWidget(index, splitter);

  // // 设置分割器比例
  // splitter->setSizes(QList<int>() << 1 << 1);

  // // 将焦点设置到新编辑器
  // newEditor->setFocus();
  // m_currentEditor = newEditor;

  // // 连接信号和槽
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateFunctionList);
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateVariableList);
  if (!m_currentEditor) {
    return;
  }

  // 获取当前编辑器的父分割器
  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(m_currentEditor->parent());
  if (!parentSplitter) {
    parentSplitter = m_mainSplitter;
  }

  // 创建新的分割器，替换当前编辑器
  QSplitter *splitter = new QSplitter(orientation);

  // 获取当前编辑器在父分割器中的索引
  int index = parentSplitter->indexOf(m_currentEditor);

  // 从父分割器中移除当前编辑器
  m_currentEditor->setParent(nullptr);

  // 将当前编辑器添加到新分割器
  splitter->addWidget(m_currentEditor);

  // 连接文本更新信号
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateFunctionList);

  // 设置编辑器属性
  setupEditor(newEditor);

  // 设置自动补全
  setupAutoCompletion(newEditor);

  // 复制当前编辑器的内容到新编辑器
  newEditor->setText(m_currentEditor->text());

  // 将新编辑器添加到分割器
  splitter->addWidget(newEditor);

  // 将分割器添加到父分割器
  parentSplitter->insertWidget(index, splitter);

  // 设置分割器比例
  splitter->setSizes(QList<int>() << 1 << 1);

  // 将焦点设置到新编辑器
  newEditor->setFocus();
  m_currentEditor = newEditor;

  // 连接信号和槽
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateVariableList);
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateFunctionList);
}

// void CodeEditor::closeSplitView()
// {
//     if (!m_currentEditor || m_editors.size() <= 1) {
//         return;
//     }

//     // 获取当前编辑器的父分割器
//     QSplitter* parentSplitter =
//     qobject_cast<QSplitter*>(m_currentEditor->parent()); if (!parentSplitter
//     || parentSplitter == m_mainSplitter) {
//         return;
//     }

//     // 获取分割器中的另一个部件
//     QWidget* otherWidget = nullptr;
//     for (int i = 0; i < parentSplitter->count(); ++i) {
//         QWidget* widget = parentSplitter->widget(i);
//         if (widget != m_currentEditor) {
//             otherWidget = widget;
//             break;
//         }
//     }

//     if (!otherWidget) {
//         return;
//     }

//     // 获取父分割器的父部件
//     QWidget* grandParent =  qobject_cast<QWidget*>(parentSplitter->parent());
//     QSplitter* grandParentSplitter = qobject_cast<QSplitter*>(grandParent);

//     // 获取父分割器在其父分割器中的索引
//     int index = -1;
//     if (grandParentSplitter) {
//         index = grandParentSplitter->indexOf(parentSplitter);
//     } else if (grandParent == m_mainSplitter) {
//         index = m_mainSplitter->indexOf(parentSplitter);
//         grandParentSplitter = m_mainSplitter;
//     }

//     // 从父分割器中移除另一个部件
//     otherWidget->setParent(nullptr);

//     // 将另一个部件添加到父分割器的父部件中
//     if (index != -1 && grandParentSplitter) {
//         grandParentSplitter->insertWidget(index, otherWidget);
//     } else {
//         // 如果找不到父分割器的父部件，则添加到主分割器
//         m_mainSplitter->addWidget(otherWidget);
//     }

//     // 从编辑器列表中移除当前编辑器
//     m_editors.removeOne(m_currentEditor);

//     // 删除当前编辑器
//     delete m_currentEditor;

//     // 删除父分割器
//     delete parentSplitter;

//     // 更新当前编辑器
//     if (qobject_cast<QsciScintilla*>(otherWidget)) {
//         m_currentEditor = qobject_cast<QsciScintilla*>(otherWidget);
//     } else if (QSplitter* otherSplitter =
//     qobject_cast<QSplitter*>(otherWidget)) {
//         // 如果另一个部件是分割器，则找到其中的第一个编辑器
//         m_currentEditor = findFirstEditor(otherSplitter);
//     } else {
//         m_currentEditor = m_editors.first();
//     }

//     // 将焦点设置到当前编辑器
//     if (m_currentEditor) {
//         m_currentEditor->setFocus();
//     }
// }

// void CodeEditor::closeSplitView()
// {
//     if (!m_currentEditor || m_editors.size() <= 1) {
//         return;
//     }

//     // 获取当前编辑器的父分割器
//     QSplitter* parentSplitter =
//     qobject_cast<QSplitter*>(m_currentEditor->parent()); if (!parentSplitter
//     || parentSplitter == m_mainSplitter) {
//         return;
//     }

//     // 获取分割器中的另一个部件
//     QWidget* otherWidget = nullptr;
//     for (int i = 0; i < parentSplitter->count(); ++i) {
//         QWidget* widget = parentSplitter->widget(i);
//         if (widget != m_currentEditor) {
//             otherWidget = widget;
//             break;
//         }
//     }

//     if (!otherWidget) {
//         return;
//     }

//     // 获取父分割器的父部件 - 修复类型转换问题
//     QWidget* grandParent = qobject_cast<QWidget*>(parentSplitter->parent());
//     QSplitter* grandParentSplitter = nullptr;

//     // 检查父分割器的父部件是否是分割器或主分割器
//     if (grandParent == m_mainSplitter) {
//         grandParentSplitter = m_mainSplitter;
//     } else {
//         grandParentSplitter = qobject_cast<QSplitter*>(grandParent);
//     }

//     // 获取父分割器在其父分割器中的索引
//     int index = -1;
//     if (grandParentSplitter) {
//         index = grandParentSplitter->indexOf(parentSplitter);
//     }

//     // 从父分割器中移除另一个部件
//     otherWidget->setParent(nullptr);

//     // 将另一个部件添加到父分割器的父部件中
//     if (index != -1 && grandParentSplitter) {
//         grandParentSplitter->insertWidget(index, otherWidget);
//     } else {
//         // 如果找不到父分割器的父部件，则添加到主分割器
//         m_mainSplitter->addWidget(otherWidget);
//     }

//     // 从编辑器列表中移除当前编辑器
//     m_editors.removeOne(m_currentEditor);

//     // 删除当前编辑器
//     delete m_currentEditor;

//     // 删除父分割器
//     delete parentSplitter;

//     // 更新当前编辑器
//     if (qobject_cast<QsciScintilla*>(otherWidget)) {
//         m_currentEditor = qobject_cast<QsciScintilla*>(otherWidget);
//     } else if (QSplitter* otherSplitter =
//     qobject_cast<QSplitter*>(otherWidget)) {
//         // 如果另一个部件是分割器，则找到其中的第一个编辑器
//         m_currentEditor = findFirstEditor(otherSplitter);
//     } else {
//         m_currentEditor = m_editors.first();
//     }

//     // 将焦点设置到当前编辑器
//     if (m_currentEditor) {
//         m_currentEditor->setFocus();
//     }
// }

void CodeEditor::closeSplitView() {
  if (!m_currentEditor || m_editors.size() <= 1) {
    return;
  }

  // 获取当前编辑器的父分割器
  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(m_currentEditor->parent());
  if (!parentSplitter || parentSplitter == m_mainSplitter) {
    return;
  }

  // 获取分割器中的另一个部件
  QWidget *otherWidget = nullptr;
  for (int i = 0; i < parentSplitter->count(); ++i) {
    QWidget *widget = parentSplitter->widget(i);
    if (widget != m_currentEditor) {
      otherWidget = widget;
      break;
    }
  }

  if (!otherWidget) {
    return;
  }

  // 从编辑器列表中移除当前编辑器
  m_editors.removeOne(m_currentEditor);

  // 保存当前编辑器的引用，以便后面删除
  QsciScintilla *editorToDelete = m_currentEditor;

  // 获取父分割器的父部件
  QWidget *grandParent = qobject_cast<QWidget *>(parentSplitter->parent());
  QSplitter *grandParentSplitter = nullptr;

  // 先更新当前编辑器引用，避免使用已删除的指针
  if (qobject_cast<QsciScintilla *>(otherWidget)) {
    m_currentEditor = qobject_cast<QsciScintilla *>(otherWidget);
  } else if (QSplitter *otherSplitter =
                 qobject_cast<QSplitter *>(otherWidget)) {
    // 如果另一个部件是分割器，则找到其中的第一个编辑器
    m_currentEditor = findFirstEditor(otherSplitter);
  } else {
    // 如果没有找到合适的编辑器，使用列表中的第一个
    m_currentEditor = m_editors.isEmpty() ? nullptr : m_editors.first();
  }

  // 处理特殊情况：如果父部件是主分割器
  if (grandParent == m_mainSplitter) {
    // 从父分割器中移除另一个部件（先保存引用）
    otherWidget->setParent(nullptr);

    // 获取父分割器在主分割器中的索引
    int index = m_mainSplitter->indexOf(parentSplitter);

    // 将另一个部件添加到主分割器
    m_mainSplitter->insertWidget(index, otherWidget);
  } else {
    // 处理嵌套分割器的情况
    grandParentSplitter = qobject_cast<QSplitter *>(grandParent);
    if (!grandParentSplitter) {
      // 安全检查失败，恢复编辑器列表并返回
      m_editors.append(editorToDelete);
      return;
    }

    // 获取父分割器在祖父分割器中的索引
    int index = grandParentSplitter->indexOf(parentSplitter);

    // 从父分割器中移除另一个部件
    otherWidget->setParent(nullptr);

    // 将另一个部件添加到祖父分割器
    grandParentSplitter->insertWidget(index, otherWidget);
  }

  // 删除当前编辑器和父分割器
  delete editorToDelete;
  delete parentSplitter;

  // 将焦点设置到当前编辑器
  if (m_currentEditor) {
    m_currentEditor->setFocus();
  }
}

// 添加一个辅助方法来查找分割器中的第一个编辑器
QsciScintilla *CodeEditor::findFirstEditor(QSplitter *splitter) {
  if (!splitter) {
    return nullptr;
  }

  for (int i = 0; i < splitter->count(); ++i) {
    QWidget *widget = splitter->widget(i);

    // 如果是编辑器，直接返回
    QsciScintilla *editor = qobject_cast<QsciScintilla *>(widget);
    if (editor) {
      return editor;
    }

    // 如果是分割器，递归查找
    QSplitter *childSplitter = qobject_cast<QSplitter *>(widget);
    if (childSplitter) {
      QsciScintilla *foundEditor = findFirstEditor(childSplitter);
      if (foundEditor) {
        return foundEditor;
      }
    }
  }

  return nullptr;
}

void CodeEditor::setupEditor(QsciScintilla *editor) {
  // 设置编辑器字体
  QFont font("Consolas", 10);
  font.setFixedPitch(true);
  editor->setFont(font);
  editor->setMarginsFont(font);

  // 设置行号边距
  editor->setMarginWidth(0, "0000");
  editor->setMarginLineNumbers(0, true);

  // 设置折叠边距
  editor->setMarginWidth(2, 14);
  editor->setMarginType(2, QsciScintilla::SymbolMargin);
  editor->setMarginSensitivity(2, true);
  editor->setFolding(QsciScintilla::BoxedTreeFoldStyle);

  // 设置C++语法高亮
  editor->setLexer(m_lexerCPP);

  // 设置自动缩进
  editor->setAutoIndent(true);
  editor->setIndentationWidth(4);
  editor->setTabWidth(4);
  editor->setTabIndents(true);
  editor->setBackspaceUnindents(true);

  // 设置UTF-8编码
  editor->setUtf8(true);

  // 设置括号匹配
  editor->setBraceMatching(QsciScintilla::SloppyBraceMatch);

  // ... existing code ...
  // 设置括号匹配
  editor->setBraceMatching(QsciScintilla::SloppyBraceMatch);

  // 设置不同括号对的颜色
  // // 圆括号 ()
  // editor->setMatchedBraceForegroundColor(QColor("#FF0000"));  // 红色
  // editor->setMatchedBraceBackgroundColor(QColor("#FFE4E1"));  // 浅红色背景

  // // 方括号 []
  // editor->setUnmatchedBraceForegroundColor(QColor("#0000FF"));  // 蓝色
  // editor->setUnmatchedBraceBackgroundColor(QColor("#E6E6FA"));  // 浅蓝色背景

  // 大括号 {}
  editor->setCaretForegroundColor(QColor("#008000"));   // 绿色
  editor->setMarginsForegroundColor(QColor("#90EE90")); // 浅绿色背景

  // 设置括号匹配的样式和颜色
  // 注意：QScintilla默认只支持一种括号匹配颜色，但我们可以通过设置不同的样式来区分
  editor->setMatchedBraceBackgroundColor(QColor("#3B514D")); // 匹配的括号背景色
  editor->setMatchedBraceForegroundColor(
      QColor("#FFD700")); // 匹配的括号前景色 - 金色
  editor->setUnmatchedBraceBackgroundColor(
      QColor("#4B1515")); // 不匹配的括号背景色
  editor->setUnmatchedBraceForegroundColor(
      QColor("#FF0000")); // 不匹配的括号前景色 - 红色

  // 设置自动换行
  editor->setWrapMode(QsciScintilla::WrapNone);

  // 设置光标宽度
  editor->setCaretWidth(2);

  // 设置行尾可见
  editor->setEolVisibility(false);

  // 设置缩进指南
  editor->setIndentationGuides(true);

  // 设置不同类型括号的颜色 (通过自定义指示器实现)
  setupBraceColors(editor);

  // 在setupEditor中确保highlightBraces在适当的时间点调用
  connect(editor, &QsciScintilla::SCN_STYLENEEDED,
          [this, editor](int position) {
            // 在样式需要更新后调用
            highlightBraces(editor);
          });

  // 添加函数名高亮设置
  setupFunctionHighlight(editor);
  setupRainbowBrackets(editor);
}

// 添加新方法：设置不同类型括号的颜色
// void CodeEditor::setupBraceColors(QsciScintilla* editor)
// {
//     // 定义不同类型的括号指示器
//     const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
//     const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
//     const int CURLY_BRACE_INDICATOR = 10; // 花括号 {}
//     const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

//     // 设置圆括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator,
//     ROUND_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#4EC9B0"),
//     ROUND_BRACE_INDICATOR); // 青绿色
//     editor->setIndicatorOutlineColor(QColor("#4EC9B0"),
//     ROUND_BRACE_INDICATOR);

//     // 设置方括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator,
//     SQUARE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#CE9178"),
//     SQUARE_BRACE_INDICATOR); // 橙色
//     editor->setIndicatorOutlineColor(QColor("#CE9178"),
//     SQUARE_BRACE_INDICATOR);

//     // 设置花括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator,
//     CURLY_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#569CD6"),
//     CURLY_BRACE_INDICATOR); // 蓝色
//     editor->setIndicatorOutlineColor(QColor("#569CD6"),
//     CURLY_BRACE_INDICATOR);

//     // 设置尖括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator,
//     ANGLE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#C586C0"),
//     ANGLE_BRACE_INDICATOR); // 紫色
//     editor->setIndicatorOutlineColor(QColor("#C586C0"),
//     ANGLE_BRACE_INDICATOR);

//     // 连接文本变化信号，以便在文本变化时更新括号颜色
//     connect(editor, &QsciScintilla::textChanged, [this, editor]() {
//         highlightBraces(editor);
//     });

//     // 初始化时高亮括号
//     highlightBraces(editor);
// }

// void CodeEditor::setupBraceColors(QsciScintilla* editor)
// {
//     // 定义不同类型的括号指示器
//     const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
//     const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
//     const int CURLY_BRACE_INDICATOR = 10; // 花括号 {}
//     const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

//     // 提高指示器优先级
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
//     ROUND_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE,
//     ROUND_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
//     ROUND_BRACE_INDICATOR, 0x4EC9B0);

//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
//     SQUARE_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE,
//     SQUARE_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
//     SQUARE_BRACE_INDICATOR, 0xCE9178);

//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
//     CURLY_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE,
//     CURLY_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
//     CURLY_BRACE_INDICATOR, 0x569CD6);

//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
//     ANGLE_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE,
//     ANGLE_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
//     ANGLE_BRACE_INDICATOR, 0xC586C0);

//     // 设置圆括号指示器样式 - 使用更明显的样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator,
//     ROUND_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#4EC9B0"),
//     ROUND_BRACE_INDICATOR); // 青绿色

//     // 设置方括号指示器样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator,
//     SQUARE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#CE9178"),
//     SQUARE_BRACE_INDICATOR); // 橙色

//     // 设置花括号指示器样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator,
//     CURLY_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#569CD6"),
//     CURLY_BRACE_INDICATOR); // 蓝色

//     // 设置尖括号指示器样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator,
//     ANGLE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#C586C0"),
//     ANGLE_BRACE_INDICATOR); // 紫色

//     // 连接文本变化信号，以便在文本变化时更新括号颜色
//     connect(editor, &QsciScintilla::textChanged, [this, editor]() {
//         highlightBraces(editor);
//     });

//     // 初始化时高亮括号
//     highlightBraces(editor);
// }

// 1. 修复编译问题 - 更新setupBraceColors方法
void CodeEditor::setupBraceColors(QsciScintilla *editor) {
  // 1. 禁用词法分析器对运算符(包括括号)的默认处理
  m_lexerCPP->setColor(QColor(0, 0, 0, 0), QsciLexerCPP::Operator);

  // 2. 使用非常高的指示器ID值(避免与其他指示器冲突)
  const int ROUND_BRACE_INDICATOR = 40;
  const int SQUARE_BRACE_INDICATOR = 41;
  const int CURLY_BRACE_INDICATOR = 42;

  // 3. 使用更明显的样式并修复类型转换问题
  editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, ROUND_BRACE_INDICATOR,
                        QsciScintilla::INDIC_COMPOSITIONTHICK);
  editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, ROUND_BRACE_INDICATOR,
                        0x0000FF); // 红色
  editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, ROUND_BRACE_INDICATOR,
                        255);
  editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, ROUND_BRACE_INDICATOR,
                        static_cast<long>(false));

  // 为方括号和花括号做同样处理...

  // 4. 统一信号连接方式，使用SCN_UPDATEUI
  disconnect(editor, &QsciScintilla::SCN_STYLENEEDED, nullptr, nullptr);
  disconnect(editor, &QsciScintilla::SCN_UPDATEUI, nullptr, nullptr);
  connect(editor, &QsciScintilla::SCN_UPDATEUI,
          [this, editor](int) { highlightBraces(editor); });

  // 初始显示
  QTimer::singleShot(100, [this, editor]() { highlightBraces(editor); });
}

void CodeEditor::highlightBraces(QsciScintilla *editor) {
  // 获取编辑器文本
  QString text = editor->text();

  // 定义不同类型的括号指示器基础值
  const int ROUND_BRACE_INDICATOR = 30; // 更高的值
  const int SQUARE_BRACE_INDICATOR = 31;
  const int CURLY_BRACE_INDICATOR = 32;
  const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

  // 清除所有括号指示器
  editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(),
                              ROUND_BRACE_INDICATOR);
  editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(),
                              SQUARE_BRACE_INDICATOR);
  editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(),
                              CURLY_BRACE_INDICATOR);
  editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(),
                              ANGLE_BRACE_INDICATOR);

  // 清除所有花括号指示器 (12-20) - 确保完全清除
  for (int i = 0; i < 8; i++) {
    int indicatorId = CURLY_BRACE_INDICATOR + i;
    editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(),
                                indicatorId);
  }

  // 使用栈来匹配括号对
  QStack<QPair<int, int>> roundBraceStack;  // 圆括号栈 (行, 列)
  QStack<QPair<int, int>> squareBraceStack; // 方括号栈
  QStack<QPair<int, int>> curlyBraceStack;  // 花括号栈
  QStack<QPair<int, int>> angleBraceStack;  // 尖括号栈

  // 为嵌套的花括号准备不同的颜色 - 使用更鲜明的颜色
  QList<QColor> curlyBraceColors;
  curlyBraceColors << QColor("#569CD6")  // 蓝色
                   << QColor("#4EC9B0")  // 青绿色
                   << QColor("#CE9178")  // 橙色
                   << QColor("#C586C0")  // 紫色
                   << QColor("#DCDCAA")  // 黄色
                   << QColor("#9CDCFE")  // 浅蓝色
                   << QColor("#D16969")  // 红色
                   << QColor("#6A9955"); // 绿色

  // 为每个嵌套级别预先设置指示器样式和颜色 - 确保每个指示器都有独特的样式
  for (int i = 0; i < curlyBraceColors.size(); i++) {
    int indicatorId = CURLY_BRACE_INDICATOR + i;
    QColor color = curlyBraceColors[i];

    // 设置指示器样式为文本前景色 - 使用更明显的样式
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, indicatorId,
                          QsciScintilla::INDIC_FULLBOX);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, indicatorId,
                          (color.red() << 16) | (color.green() << 8) |
                              color.blue());
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, indicatorId, 255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, indicatorId, true);
  }

  // 记录每对花括号的嵌套级别和对应的指示器ID
  QMap<QPair<int, int>, int> bracePairToIndicator;
  int currentCurlyLevel = 0;

  // 遍历文本中的每个字符
  for (int line = 0; line < editor->lines(); line++) {
    QString lineText = editor->text(line);

    for (int col = 0; col < lineText.length(); col++) {
      QChar ch = lineText.at(col);

      // 处理圆括号
      if (ch == '(') {
        roundBraceStack.push(qMakePair(line, col));
      } else if (ch == ')') {
        if (!roundBraceStack.isEmpty()) {
          QPair<int, int> openBrace = roundBraceStack.pop();
          // 高亮开括号
          editor->fillIndicatorRange(openBrace.first, openBrace.second,
                                     openBrace.first, openBrace.second + 1,
                                     ROUND_BRACE_INDICATOR);
          // 高亮闭括号
          editor->fillIndicatorRange(line, col, line, col + 1,
                                     ROUND_BRACE_INDICATOR);
        }
      }

      // 处理方括号
      if (ch == '[') {
        squareBraceStack.push(qMakePair(line, col));
      } else if (ch == ']') {
        if (!squareBraceStack.isEmpty()) {
          QPair<int, int> openBrace = squareBraceStack.pop();
          // 高亮开括号
          editor->fillIndicatorRange(openBrace.first, openBrace.second,
                                     openBrace.first, openBrace.second + 1,
                                     SQUARE_BRACE_INDICATOR);
          // 高亮闭括号
          editor->fillIndicatorRange(line, col, line, col + 1,
                                     SQUARE_BRACE_INDICATOR);
        }
      }

      // 处理花括号 - 特别处理嵌套级别
      if (ch == '{') {
        // 保存当前位置
        QPair<int, int> bracePair = qMakePair(line, col);

        // 计算当前嵌套级别对应的指示器ID
        int level = currentCurlyLevel % curlyBraceColors.size();
        int indicatorId = CURLY_BRACE_INDICATOR + level;

        // 记录这对花括号使用的指示器ID
        bracePairToIndicator[bracePair] = indicatorId;

        // 将当前位置压入栈
        curlyBraceStack.push(bracePair);

        // 高亮开括号
        editor->fillIndicatorRange(line, col, line, col + 1, indicatorId);

        // 增加嵌套级别
        currentCurlyLevel++;
      } else if (ch == '}') {
        if (!curlyBraceStack.isEmpty()) {
          // 获取对应的开括号位置
          QPair<int, int> openBrace = curlyBraceStack.pop();

          // 获取这对花括号使用的指示器ID
          int indicatorId = bracePairToIndicator[openBrace];

          // 高亮闭括号 - 使用与开括号相同的指示器
          editor->fillIndicatorRange(line, col, line, col + 1, indicatorId);

          // 减少嵌套级别
          currentCurlyLevel--;
        }
      }

      // 处理尖括号
      if (ch == '<') {
        angleBraceStack.push(qMakePair(line, col));
      } else if (ch == '>') {
        if (!angleBraceStack.isEmpty()) {
          QPair<int, int> openBrace = angleBraceStack.pop();
          // 高亮开括号
          editor->fillIndicatorRange(openBrace.first, openBrace.second,
                                     openBrace.first, openBrace.second + 1,
                                     ANGLE_BRACE_INDICATOR);
          // 高亮闭括号
          editor->fillIndicatorRange(line, col, line, col + 1,
                                     ANGLE_BRACE_INDICATOR);
        }
      }
    }
  }
}

void CodeEditor::setupAutoCompletion(QsciScintilla *editor) {
  // 如果还没有创建API对象，则创建
  if (!m_apiCPP) {
    m_apiCPP = new QsciAPIs(m_lexerCPP);

    // 添加C/C++关键字和STM32相关API
    QStringList keywords;
    // C/C++关键字
    keywords << "auto"
             << "break"
             << "case"
             << "char"
             << "const"
             << "continue"
             << "default"
             << "do"
             << "double"
             << "else"
             << "enum"
             << "extern"
             << "float"
             << "for"
             << "goto"
             << "if"
             << "int"
             << "long"
             << "register"
             << "return"
             << "short"
             << "signed"
             << "sizeof"
             << "static"
             << "struct"
             << "switch"
             << "typedef"
             << "union"
             << "unsigned"
             << "void"
             << "volatile"
             << "while";

    // STM32数据类型
    keywords << "uint8_t"
             << "uint16_t"
             << "uint32_t"
             << "int8_t"
             << "int16_t"
             << "int32_t"
             << "GPIO_InitTypeDef"
             << "USART_InitTypeDef"
             << "TIM_TimeBaseInitTypeDef";

    // STM32特定函数和宏
    keywords << "GPIO_Init"
             << "GPIO_SetBits"
             << "GPIO_ResetBits"
             << "GPIO_ReadInputDataBit"
             << "TIM_TimeBaseInit"
             << "TIM_Cmd"
             << "TIM_ITConfig"
             << "TIM_GetCounter"
             << "USART_Init"
             << "USART_Cmd"
             << "USART_SendData"
             << "USART_ReceiveData"
             << "ADC_Init"
             << "ADC_Cmd"
             << "ADC_StartConversion"
             << "ADC_GetConversionValue"
             << "RCC_APB1PeriphClockCmd"
             << "RCC_APB2PeriphClockCmd"
             << "RCC_AHB1PeriphClockCmd"
             << "NVIC_Init"
             << "NVIC_EnableIRQ"
             << "NVIC_DisableIRQ"
             << "SysTick_Config"
             << "HAL_Delay"
             << "HAL_GPIO_WritePin"
             << "HAL_GPIO_ReadPin";

    // STM32 HAL库函数
    keywords << "HAL_Init()"
             << "HAL_GPIO_Init()"
             << "HAL_GPIO_WritePin()"
             << "HAL_GPIO_ReadPin()"
             << "HAL_Delay()"
             << "HAL_UART_Init()"
             << "HAL_UART_Transmit()"
             << "HAL_UART_Receive()"
             << "HAL_TIM_Base_Init()"
             << "HAL_TIM_Base_Start()"
             << "HAL_TIM_Base_Stop()";

    // 添加到API
    for (const QString &keyword : keywords) {
      m_apiCPP->add(keyword);
    }

    // 准备API
    m_apiCPP->prepare();
  }

  // 设置自动补全
  editor->setAutoCompletionThreshold(1); // 输入1个字符后显示补全
  editor->setAutoCompletionSource(
      QsciScintilla::AcsAll); // 使用所有可用的补全源
  editor->setAutoCompletionCaseSensitivity(false); // 不区分大小写
  editor->setAutoCompletionReplaceWord(true);      // 替换当前单词
  editor->setAutoCompletionUseSingle(
      QsciScintilla::AcusNever); // 不自动选择唯一匹配项

  // 启用自动补全弹出
  editor->setAutoCompletionFillupsEnabled(true);
}

void CodeEditor::createFunctionList() {
  // 创建函数列表容器
  QWidget *functionListContainer = new QWidget(this);
  QVBoxLayout *containerLayout = new QVBoxLayout(functionListContainer);
  containerLayout->setContentsMargins(0, 0, 0, 0);

  // 添加标题标签
  QLabel *titleLabel = new QLabel("函数列表", functionListContainer);
  titleLabel->setAlignment(Qt::AlignCenter);
  titleLabel->setStyleSheet(
      "font-weight: bold; padding: 5px; background-color: #e0e0e0; "
      "border-bottom: 1px solid #ccc;");

  containerLayout->addWidget(titleLabel);

  // 创建函数列表控件
  m_functionList = new QListWidget(functionListContainer);
  containerLayout->addWidget(m_functionList);

  // 设置函数列表属性
  m_functionList->setFont(QFont("Consolas", 10));
  m_functionList->setAlternatingRowColors(true);
  m_functionList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_functionList->setSortingEnabled(false);
  // m_functionList->setMinimumWidth(200);
  // m_functionList->setMaximumWidth(300);

  // 设置样式表
  m_functionList->setStyleSheet("QListWidget {"
                                "    background-color: #f5f5f5;"
                                "    border: 1px solid #ddd;"
                                "}"
                                "QListWidget::item {"
                                "    padding: 4px;"
                                "    border-bottom: 1px solid #eee;"
                                "}"
                                "QListWidget::item:selected {"
                                "    background-color: #0078d7;"
                                "    color: white;"
                                "}"
                                "QListWidget::item:alternate {"
                                "    background-color: #f9f9f9;"
                                "}");

  // 将函数列表容器添加到主分割器
  m_mainSplitter->addWidget(functionListContainer);

  // 设置分割器比例
  m_mainSplitter->setStretchFactor(0, 3); // 编辑器占3份
  m_mainSplitter->setStretchFactor(1, 1); // 函数列表占1份
  // 连接信号和槽 - 使用Qt::UniqueConnection避免重复连接
  // connect(m_functionList, &QListWidget::currentRowChanged, this,
  // &CodeEditor::jumpToFunction, Qt::UniqueConnection);

  // 连接函数列表项点击信号 - 简化跳转逻辑
  connect(m_functionList, &QListWidget::itemClicked, this,
          [this](QListWidgetItem *item) {
            if (!m_currentEditor || !item) {
              return;
            }

            // 获取存储的行号
            int line = item->data(Qt::UserRole).toInt();

            // 确保行号有效
            if (line >= 0 && line < m_currentEditor->lines()) {
              // 直接跳转到指定行的开始位置
              m_currentEditor->setCursorPosition(line, 0);

              // 确保目标行可见
              m_currentEditor->ensureLineVisible(line);

              // 将目标行居中显示
              int visibleLines = m_currentEditor->SendScintilla(
                  QsciScintilla::SCI_LINESONSCREEN);
              int targetLine = qMax(0, line - visibleLines / 2);
              m_currentEditor->SendScintilla(
                  QsciScintilla::SCI_SETFIRSTVISIBLELINE, targetLine);

              // 高亮显示当前行
              m_currentEditor->setCaretLineVisible(true);
              m_currentEditor->setCaretLineBackgroundColor(
                  QColor(255, 255, 0, 100)); // 淡黄色背景

              // 设置焦点到编辑器
              m_currentEditor->setFocus();

              // 可选：选择整行以便用户看到跳转位置
              QString lineText = m_currentEditor->text(line);
              int lineLength = lineText.length();
              if (lineLength > 0) {
                // 选择从行首到行尾（不包括换行符）
                m_currentEditor->setSelection(line, 0, line, lineLength);
              }
            }
          });
}

// 解析代码中的函数
void CodeEditor::parseFunctions(const QString &code) {
  try {
    // 清空函数列表
    m_functions.clear();

    if (code.isEmpty()) {
      qDebug() << "解析函数: 代码为空";
      return; // 避免处理空代码
    }

    // 预处理：移除注释
    QString filteredText = code;
    filteredText.remove(
        QRegularExpression(R"(//[^\n]*|/\*.*?\*/)",
                           QRegularExpression::DotMatchesEverythingOption |
                               QRegularExpression::MultilineOption));
    // 合并多行声明
    filteredText.replace(QRegularExpression(R"(\\\s*\n)"), " ");

    // 支持：模板函数、命名空间、多参数类型
    QRegularExpression functionRegex(
        R"((\b(?:\w+::)+)?\s*((?:\w+<.*?>)|\w+)\s+([*&]*\s*)?(\w+)\s*\([^{]*))");
    functionRegex.setPatternOptions(
        QRegularExpression::DotMatchesEverythingOption);

    if (!functionRegex.isValid()) {
      qDebug() << "解析函数: 正则表达式无效: " << functionRegex.errorString();
      return;
    }

    // 常见的C/C++关键字列表
    static const QSet<QString> keywords = {
        "if",       "for",       "while",    "switch",   "return", "else",
        "do",       "case",      "break",    "continue", "goto",   "sizeof",
        "typedef",  "volatile",  "register", "extern",   "static", "auto",
        "const",    "struct",    "union",    "enum",     "class",  "template",
        "typename", "namespace", "using",    "try",      "catch",  "throw",
        "new",      "delete"};

    // 遍历代码行 (使用原始代码以便获取正确的行号)
    QStringList lines = code.split('\n');
    int lineCount = lines.size();

    // 同时也对每一行进行简单的正则匹配，或者在filteredText中查找并映射回行号
    // 为了简单且保持行号准确，我们还是逐行处理，但使用更强的正则

    // 注意：上面的filteredText逻辑在逐行处理中较难直接应用，除非我们解析整个文本的偏移量
    // 这里我们仅在单行或临近几行中匹配

    for (int i = 0; i < lineCount; i++) {
      QString line = lines[i].trimmed();

      if (line.isEmpty() || line.startsWith("//") || line.startsWith("/*") ||
          line.contains("*/")) {
        continue; // 跳过空行和注释行
      }

      // 匹配函数定义
      QRegularExpressionMatch match = functionRegex.match(line);
      if (match.hasMatch()) {
        // 获取函数名 (索引 4: (\w+)\s*\()
        QString functionName = match.captured(4);
        if (functionName.isEmpty())
          functionName = match.captured(2); // Fallback

        // 跳过关键字和空函数名
        if (functionName.isEmpty() || keywords.contains(functionName)) {
          continue;
        }

        // 创建函数信息对象
        FunctionInfo function;
        function.name = functionName;
        function.line = i;

        // 获取完整的函数签名（可能跨越多行）
        QString signature = line;

        // 如果当前行没有分号或大括号，可能是多行函数声明
        if (!line.contains(";") && !line.contains("{")) {
          // 向下查找几行，直到找到分号或大括号
          for (int j = i + 1; j < qMin(i + 10, lineCount); j++) {
            QString nextLine = lines[j].trimmed();
            signature += " " + nextLine;
            if (nextLine.contains(";") || nextLine.contains("{")) {
              break;
            }
          }
        }

        function.signature = signature.trimmed();

        // 添加到函数列表
        m_functions.append(function);
      }
    }

    qDebug() << "解析函数: 找到" << m_functions.size() << "个函数";
  } catch (const std::exception &e) {
    qDebug() << "解析函数时发生异常: " << e.what();
  } catch (...) {
    qDebug() << "解析函数时发生未知异常";
  }
}

// void CodeEditor::updateFunctionList()//该版本需要保留，解决了跨行函数问题
// {
//     if (!m_currentEditor || !m_functionList) {
//         return;
//     }

//     // 清空函数列表
//     m_functionList->clear();

//     // 获取编辑器文本
//     QString text = m_currentEditor->text();
//     QStringList lines = text.split('\n');

//     // 存储函数名和行号的映射
//     QMap<QString, int> functionLineMap;

//     // 改进的多行函数识别
//     // 先预处理，将跨行的函数定义合并为单行
//     QStringList processedLines;
//     QList<int> originalLineNumbers;

//     for (int i = 0; i < lines.size(); i++) {
//         QString line = lines[i];
//         QString trimmed = line.trimmed();

//         // 跳过空行和注释
//         if (trimmed.isEmpty() || trimmed.startsWith("//") ||
//         trimmed.startsWith("/*")) {
//             processedLines.append(line);
//             originalLineNumbers.append(i);
//             continue;
//         }
//         // 检查是否是函数定义的开始（包含函数名和左括号）
//         if (trimmed.contains("(") && !trimmed.contains(";") &&
//             !trimmed.startsWith("#") && !trimmed.startsWith("if") &&
//             !trimmed.startsWith("for") && !trimmed.startsWith("while") &&
//             !trimmed.startsWith("switch")) {

//             QString multiLineFunction = line;
//             int startLineNum = i;
//             bool foundClosingParen = line.contains(")");
//             bool foundOpenBrace = line.contains("{");
//             bool isComplete = false;

//             // 第一步：如果当前行没有右括号，继续合并后续行直到找到右括号
//             if (!foundClosingParen) {
//                 for (int j = i + 1; j < lines.size(); j++) {
//                     QString nextLine = lines[j];
//                     multiLineFunction += " " + nextLine.trimmed();

//                     if (nextLine.contains(")")) {
//                         foundClosingParen = true;
//                         if (nextLine.contains("{")) {
//                             foundOpenBrace = true;
//                         }
//                         i = j; // 更新外层循环的索引
//                         break;
//                     }

//                     // 防止无限循环，最多合并15行
//                     if (j - i > 15) {
//                         break;
//                     }
//                 }
//             }

//             // 第二步：如果找到了右括号但还没找到开括号，继续搜索
//             if (foundClosingParen && !foundOpenBrace) {
//                 // 继续搜索函数修饰符和函数体
//                 for (int j = i + 1; j < lines.size(); j++) {
//                     QString nextLine = lines[j].trimmed();

//                     // 跳过空行
//                     if (nextLine.isEmpty()) {
//                         continue;
//                     }

//                     // 合并这一行
//                     multiLineFunction += " " + nextLine;

//                     // 检查各种可能的情况
//                     if (nextLine.startsWith(":")) {
//                         // 构造函数初始化列表
//                         continue;
//                     }
//                     else if (nextLine.contains("{")) {
//                         // 找到函数体开始
//                         i = j;
//                         foundOpenBrace = true;
//                         isComplete = true;
//                         break;
//                     }
//                     else if (nextLine.contains(";")) {
//                         // 函数声明，不是定义
//                         isComplete = false;
//                         break;
//                     }
//                     else if (nextLine.startsWith("const") ||
//                              nextLine.startsWith("override") ||
//                              nextLine.startsWith("final") ||
//                              nextLine.startsWith("noexcept") ||
//                              nextLine.startsWith("->")) {
//                         // 函数修饰符，继续合并
//                         continue;
//                     }
//                     else {
//                         // 其他情况，可能是多行初始化列表或其他内容
//                         continue;
//                     }

//                     // 防止无限循环
//                     if (j - i > 10) {
//                         break;
//                     }
//                 }
//             } else if (foundClosingParen && foundOpenBrace) {
//                 // 单行函数定义
//                 isComplete = true;
//             }

//             // 只有完整的函数定义才添加到处理列表
//             if (foundClosingParen && (foundOpenBrace || isComplete)) {
//                 processedLines.append(multiLineFunction);
//                 originalLineNumbers.append(startLineNum);
//             } else {
//                 // 不完整的函数定义，按原样处理
//                 processedLines.append(line);
//                 originalLineNumbers.append(i);
//             }
//         } else {
//             processedLines.append(line);
//             originalLineNumbers.append(i);
//         }
//     }

//     //改进的C++函数识别正则表达式
//     QRegularExpression functionRegex(
//         R"(^\s*(?:(?:static|inline|virtual|extern|const|explicit|friend|template\s*<[^>]*>)\s+)*(?:[\w:]+(?:\s*[*&]+)?\s+)+([A-Za-z_]\w*(?:::\w+)*)\s*\([^;]*\)\s*(?:const\s*)?(?:override\s*)?(?:final\s*)?(?:noexcept\s*)?(?:->\s*[\w:]+\s*)?)"
//         );

//     // 构造函数识别正则表达式 - 支持类前缀
//     QRegularExpression constructorRegex(
//         R"(^\s*(?:explicit\s+)?(?:([A-Za-z_]\w*)::)?([A-Za-z_]\w*)\s*\([^;]*\)\s*(?::\s*[^{;]*)?\s*(?:\{|$))"
//         );

//     // 析构函数识别正则表达式 - 支持类前缀
//     QRegularExpression destructorRegex(
//         R"(^\s*(?:virtual\s+)?(?:([A-Za-z_]\w*)::)?~([A-Za-z_]\w*)\s*\(\s*\)\s*(?:override\s*)?(?:final\s*)?(?:noexcept\s*)?)"
//         );

//     // 类定义正则表达式
//     QRegularExpression classRegex(
//         R"(^\s*(?:class|struct|enum)\s+(\w+)(?:\s*:[^{]*)?)"
//         );

//     // 宏定义正则表达式
//     QRegularExpression macroRegex(
//         R"(^\s*#define\s+(\w+)(?:\([^)]*\))?)"
//         );

//     // 存储已识别的类名，用于构造函数识别
//     QSet<QString> classNames;

//     // 关键字过滤列表
//     QSet<QString> keywords = {
//         "if", "for", "while", "switch", "catch", "return", "else", "do",
//         "break", "continue", "goto", "sizeof", "typedef", "volatile",
//         "register", "extern", "static", "auto", "const", "struct", "union",
//         "enum", "class", "template", "typename", "namespace", "using",
//         "try", "throw", "new", "delete", "case", "default"
//     };

//     // // 逐行分析
//     // for (int lineNum = 0; lineNum < lines.size(); lineNum++) {
//     //     QString line = lines[lineNum];
//     //     QString trimmedLine = line.trimmed();

//     //     // 跳过空行和注释行
//     //     if (trimmedLine.isEmpty() || trimmedLine.startsWith("//") ||
//     trimmedLine.startsWith("/*")) {
//     //         continue;
//     //     }

//     //     // 跳过多行注释块
//     //     if (trimmedLine.contains("/*") && !trimmedLine.contains("*/")) {
//     //         while (lineNum < lines.size() - 1) {
//     //             lineNum++;
//     //             if (lines[lineNum].contains("*/")) {
//     //                 break;
//     //             }
//     //         }
//     //         continue;
//     //     }

//     //     // 跳过预处理指令（除了#define）
//     //     if (trimmedLine.startsWith("#") &&
//     !trimmedLine.startsWith("#define")) {
//     //         continue;
//     //     }

//     // 逐行分析处理后的行
//     for (int lineNum = 0; lineNum < processedLines.size(); lineNum++) {
//         QString line = processedLines[lineNum];
//         QString trimmedLine = line.trimmed();
//         int originalLineNum = originalLineNumbers[lineNum];

//         // 跳过空行和注释行
//         if (trimmedLine.isEmpty() || trimmedLine.startsWith("//") ||
//         trimmedLine.startsWith("/*")) {
//             continue;
//         }

//         // 跳过多行注释块
//         if (trimmedLine.contains("/*") && !trimmedLine.contains("*/")) {
//             while (lineNum < processedLines.size() - 1) {
//                 lineNum++;
//                 if (processedLines[lineNum].contains("*/")) {
//                     break;
//                 }
//             }
//             continue;
//         }

//         // 跳过预处理指令（除了#define）
//         if (trimmedLine.startsWith("#") &&
//         !trimmedLine.startsWith("#define")) {
//             continue;
//         }

//         // 1. 识别类定义
//         QRegularExpressionMatch classMatch = classRegex.match(line);
//         if (classMatch.hasMatch()) {
//             QString className = classMatch.captured(1);
//             if (!keywords.contains(className)) {
//                 functionLineMap["📦🏗️ class " + className] = originalLineNum;
//                 classNames.insert(className);  // 记录类名
//             }
//             continue;
//         }

//         // 首先匹配析构函数
//         QRegularExpressionMatch destructorMatch =
//         destructorRegex.match(line); if (destructorMatch.hasMatch()) {
//             QString classPrefix = destructorMatch.captured(1);  // 类前缀
//             QString className = destructorMatch.captured(2);    // 类名

//             if (!keywords.contains(className) &&
//             isValidFunctionDefinition(line, lineNum, processedLines)) {
//                 QString displayName;
//                 if (!classPrefix.isEmpty()) {
//                     displayName = classPrefix + "::~" + className + "()";
//                 } else {
//                     displayName = "~" + className + "()";
//                 }
//                 functionLineMap["🔧💥 " + displayName] = originalLineNum;
//             }
//         }
//         // 然后匹配构造函数
//         else {
//             QRegularExpressionMatch constructorMatch =
//             constructorRegex.match(line); if (constructorMatch.hasMatch()) {
//                 QString classPrefix = constructorMatch.captured(1);  //
//                 类前缀 QString functionName = constructorMatch.captured(2);
//                 // 函数名

//                 if (!keywords.contains(functionName) &&
//                     isValidFunctionDefinition(line, lineNum, processedLines)
//                     && isLikelyConstructor(functionName, line)) {

//                     QString displayName;
//                     if (!classPrefix.isEmpty()) {
//                         displayName = classPrefix + "::" + functionName +
//                         "()";
//                     } else {
//                         displayName = functionName + "()";
//                     }
//                     functionLineMap["🏗️🔨 " + displayName] = originalLineNum;
//                 }
//             }
//             // 最后匹配普通C++函数定义
//             else {
//                 QRegularExpressionMatch funcMatch =
//                 functionRegex.match(line); if (funcMatch.hasMatch()) {
//                     QString functionName = funcMatch.captured(1);

//                     // 移除命名空间前缀以获取纯函数名
//                     QString pureFunctionName = functionName;
//                     if (functionName.contains("::")) {
//                         pureFunctionName = functionName.split("::").last();
//                     }

//                     // 过滤关键字和已识别的构造函数
//                     if (!keywords.contains(functionName) &&
//                         !classNames.contains(functionName) &&
//                         isValidFunctionDefinition(line, lineNum,
//                         processedLines)) {

//                         // 检查是否为main函数
//                         if (functionName == "main") {
//                             //functionLineMap["🚀🎯 " + functionName + "()"]
//                             = originalLineNum; functionLineMap["ⓜ  " +
//                             functionName + "()"] = originalLineNum;

//                         } else {
//                             //functionLineMap["⚡⚙️ " + functionName + "()"] =
//                             originalLineNum; functionLineMap["ƒ  " +
//                             functionName + "()"] = originalLineNum;
//                         }
//                     }
//                     continue;
//                 }
//             }
//         }

//         // 匹配宏定义
//         QRegularExpressionMatch macroMatch = macroRegex.match(line);
//         if (macroMatch.hasMatch()) {
//             QString macroName = macroMatch.captured(1);
//             if (!keywords.contains(macroName)) {
//                 functionLineMap["🔧📝 #define " + macroName] =
//                 originalLineNum;
//             }
//         }
//     }

//     // 按函数名排序并添加到列表
//     QStringList functionNames = functionLineMap.keys();
//     functionNames.sort(Qt::CaseInsensitive);

//     for (const QString& functionName : functionNames) {
//         QListWidgetItem* item = new QListWidgetItem(functionName);
//         item->setData(Qt::UserRole, functionLineMap[functionName]);
//         m_functionList->addItem(item);
//     }

//     // 同时更新函数名高亮
//     if (m_currentEditor) {
//         highlightFunctionNames(m_currentEditor, FUNCTION_INDICATOR);
//     }

// }

void CodeEditor::updateFunctionList() {
  if (!m_currentEditor || !m_functionList) {
    return;
  }

  // 清空函数列表
  m_functionList->clear();

  // 获取编辑器文本
  QString text = m_currentEditor->text();
  QStringList lines = text.split('\n');

  // 存储函数名和行号的映射
  QMap<QString, int> functionLineMap;

  // 改进的多行函数识别
  // 先预处理，将跨行的函数定义合并为单行
  QStringList processedLines;
  QList<int> originalLineNumbers;

  for (int i = 0; i < lines.size(); i++) {
    QString line = lines[i];
    QString trimmed = line.trimmed();

    // 跳过空行和注释
    if (trimmed.isEmpty() || trimmed.startsWith("//") ||
        trimmed.startsWith("/*")) {
      processedLines.append(line);
      originalLineNumbers.append(i);
      continue;
    }

    // 检查是否是函数定义的开始（包含函数名和左括号）
    if (trimmed.contains("(") && !trimmed.contains(";") &&
        !trimmed.startsWith("#") && !trimmed.startsWith("if") &&
        !trimmed.startsWith("for") && !trimmed.startsWith("while") &&
        !trimmed.startsWith("switch")) {

      QString multiLineFunction = line;
      int startLineNum = i;

      // 如果当前行没有右括号，继续合并后续行
      if (!line.contains(")")) {
        for (int j = i + 1; j < lines.size(); j++) {
          QString nextLine = lines[j];
          multiLineFunction += " " + nextLine.trimmed();

          if (nextLine.contains(")")) {
            i = j; // 更新外层循环的索引
            break;
          }

          // 防止无限循环，最多合并10行
          if (j - i > 10) {
            break;
          }
        }
      }

      processedLines.append(multiLineFunction);
      originalLineNumbers.append(startLineNum);
    } else {
      processedLines.append(line);
      originalLineNumbers.append(i);
    }
  }

  // 改进的C++函数识别正则表达式
  QRegularExpression functionRegex(
      R"(^\s*(?:(?:static|inline|virtual|extern|const|explicit|friend|template\s*<[^>]*>)\s+)*(?:[\w:]+(?:\s*[*&]+)?\s+)+([A-Za-z_]\w*(?:::\w+)*)\s*\([^;]*\)\s*(?:const\s*)?(?:override\s*)?(?:final\s*)?(?:noexcept\s*)?(?:->\s*[\w:]+\s*)?)");

  // 构造函数识别正则表达式 - 支持类前缀
  QRegularExpression constructorRegex(
      R"(^\s*(?:explicit\s+)?(?:([A-Za-z_]\w*)::)?([A-Za-z_]\w*)\s*\([^;]*\)\s*(?::\s*[^{;]*)?\s*(?:\{|$))");

  // 析构函数识别正则表达式 - 支持类前缀
  QRegularExpression destructorRegex(
      R"(^\s*(?:virtual\s+)?(?:([A-Za-z_]\w*)::)?~([A-Za-z_]\w*)\s*\(\s*\)\s*(?:override\s*)?(?:final\s*)?(?:noexcept\s*)?)");

  // 类定义正则表达式
  QRegularExpression classRegex(
      R"(^\s*(?:class|struct|enum)\s+(\w+)(?:\s*:[^{]*)?)");

  // 宏定义正则表达式
  QRegularExpression macroRegex(R"(^\s*#define\s+(\w+)(?:\([^)]*\))?)");

  // 存储已识别的类名，用于构造函数识别
  QSet<QString> classNames;

  // 关键字过滤列表
  QSet<QString> keywords = {
      "if",       "for",       "while",    "switch",   "catch",  "return",
      "else",     "do",        "break",    "continue", "goto",   "sizeof",
      "typedef",  "volatile",  "register", "extern",   "static", "auto",
      "const",    "struct",    "union",    "enum",     "class",  "template",
      "typename", "namespace", "using",    "try",      "throw",  "new",
      "delete",   "case",      "default"};

  // 逐行分析
  for (int lineNum = 0; lineNum < lines.size(); lineNum++) {
    QString line = lines[lineNum];
    QString trimmedLine = line.trimmed();

    // 跳过空行和注释行
    if (trimmedLine.isEmpty() || trimmedLine.startsWith("//") ||
        trimmedLine.startsWith("/*")) {
      continue;
    }

    // 跳过多行注释块
    if (trimmedLine.contains("/*") && !trimmedLine.contains("*/")) {
      while (lineNum < lines.size() - 1) {
        lineNum++;
        if (lines[lineNum].contains("*/")) {
          break;
        }
      }
      continue;
    }

    // 跳过预处理指令（除了#define）
    if (trimmedLine.startsWith("#") && !trimmedLine.startsWith("#define")) {
      continue;
    }

    // 1. 识别类定义
    QRegularExpressionMatch classMatch = classRegex.match(line);
    if (classMatch.hasMatch()) {
      QString className = classMatch.captured(1);
      if (!keywords.contains(className)) {
        functionLineMap["📦 class " + className] = lineNum;
        classNames.insert(className); // 记录类名
      }
      continue;
    }

    // 匹配析构函数
    QRegularExpressionMatch destructorMatch = destructorRegex.match(line);
    if (destructorMatch.hasMatch()) {
      QString classPrefix = destructorMatch.captured(1); // 类前缀
      QString className = destructorMatch.captured(2);   // 类名

      if (!keywords.contains(className) &&
          isValidFunctionDefinition(line, lineNum, lines)) {
        QString displayName;
        if (!classPrefix.isEmpty()) {
          displayName = classPrefix + "::~" + className + "()";
        } else {
          displayName = "~" + className + "()";
        }
        functionLineMap["🔧 " + displayName] = lineNum;
      }
    }
    // 匹配构造函数
    else {
      QRegularExpressionMatch constructorMatch = constructorRegex.match(line);
      if (constructorMatch.hasMatch()) {
        QString classPrefix = constructorMatch.captured(1);  // 类前缀
        QString functionName = constructorMatch.captured(2); // 函数名

        if (!keywords.contains(functionName) &&
            isValidFunctionDefinition(line, lineNum, lines) &&
            isLikelyConstructor(functionName, line)) {

          QString displayName;
          if (!classPrefix.isEmpty()) {
            displayName = classPrefix + "::" + functionName + "()";
          } else {
            displayName = functionName + "()";
          }
          functionLineMap["🏗️ " + displayName] = lineNum;
        }
      }
      // 最后匹配普通C++函数定义
      else {
        QRegularExpressionMatch funcMatch = functionRegex.match(line);
        if (funcMatch.hasMatch()) {
          QString functionName = funcMatch.captured(1);

          // 移除命名空间前缀以获取纯函数名
          QString pureFunctionName = functionName;
          if (functionName.contains("::")) {
            pureFunctionName = functionName.split("::").last();
          }

          // 过滤关键字和已识别的构造函数
          if (!keywords.contains(functionName) &&
              !classNames.contains(functionName) &&
              isValidFunctionDefinition(line, lineNum, lines)) {

            // 检查是否为main函数
            if (functionName == "main") {
              functionLineMap["🚀 " + functionName + "()"] = lineNum;
            } else {
              // functionLineMap["⚙️" + functionName + "()"] = lineNum;
              //  functionLineMap["🟥" + functionName + "()"] = lineNum;
              functionLineMap["⚡" + functionName + "()"] = lineNum;
            }
          }
          continue;
        }
      }
    }

    // 匹配宏定义
    QRegularExpressionMatch macroMatch = macroRegex.match(line);
    if (macroMatch.hasMatch()) {
      QString macroName = macroMatch.captured(1);
      if (!keywords.contains(macroName)) {
        functionLineMap["🔧 #define " + macroName] = lineNum; // 添加宏图标
      }
    }
  }

  // 按函数名排序并添加到列表
  QStringList functionNames = functionLineMap.keys();
  functionNames.sort(Qt::CaseInsensitive);

  for (const QString &functionName : functionNames) {
    QListWidgetItem *item = new QListWidgetItem(functionName);
    item->setData(Qt::UserRole, functionLineMap[functionName]);
    m_functionList->addItem(item);
  }
  // 同时更新函数名高亮
  if (m_currentEditor) {
    highlightFunctionNames(m_currentEditor, FUNCTION_INDICATOR);
  }
}

void CodeEditor::updateVariableList() {
  // 检查编辑器是否存在
  if (!m_currentEditor || !m_lexerCPP) {
    return;
  }

  try {
    // 获取当前文本
    QString text = m_currentEditor->text();

    // 创建一个新的API对象
    QsciAPIs *newApi = new QsciAPIs(m_lexerCPP);
    if (!newApi) {
      qDebug() << "创建API对象失败";
      return;
    }

    // 添加C/C++关键字和STM32相关API (重新添加所有标准关键字)
    QStringList keywords;
    // C/C++关键字
    keywords << "auto"
             << "break"
             << "case"
             << "char"
             << "const"
             << "continue"
             << "default"
             << "do"
             << "double"
             << "else"
             << "enum"
             << "extern"
             << "float"
             << "for"
             << "goto"
             << "if"
             << "int"
             << "long"
             << "register"
             << "return"
             << "short"
             << "signed"
             << "sizeof"
             << "static"
             << "struct"
             << "switch"
             << "typedef"
             << "union"
             << "unsigned"
             << "void"
             << "volatile"
             << "while";

    // STM32数据类型
    keywords << "uint8_t"
             << "uint16_t"
             << "uint32_t"
             << "int8_t"
             << "int16_t"
             << "int32_t"
             << "GPIO_InitTypeDef"
             << "USART_InitTypeDef"
             << "TIM_TimeBaseInitTypeDef";

    // STM32特定函数和宏
    keywords << "GPIO_Init"
             << "GPIO_SetBits"
             << "GPIO_ResetBits"
             << "GPIO_ReadInputDataBit"
             << "TIM_TimeBaseInit"
             << "TIM_Cmd"
             << "TIM_ITConfig"
             << "TIM_GetCounter"
             << "USART_Init"
             << "USART_Cmd"
             << "USART_SendData"
             << "USART_ReceiveData"
             << "ADC_Init"
             << "ADC_Cmd"
             << "ADC_StartConversion"
             << "ADC_GetConversionValue"
             << "RCC_APB1PeriphClockCmd"
             << "RCC_APB2PeriphClockCmd"
             << "RCC_AHB1PeriphClockCmd"
             << "NVIC_Init"
             << "NVIC_EnableIRQ"
             << "NVIC_DisableIRQ"
             << "SysTick_Config"
             << "HAL_Delay"
             << "HAL_GPIO_WritePin"
             << "HAL_GPIO_ReadPin";

    // STM32 HAL库函数
    keywords << "HAL_Init()"
             << "HAL_GPIO_Init()"
             << "HAL_GPIO_WritePin()"
             << "HAL_GPIO_ReadPin()"
             << "HAL_Delay()"
             << "HAL_UART_Init()"
             << "HAL_UART_Transmit()"
             << "HAL_UART_Receive()"
             << "HAL_TIM_Base_Init()"
             << "HAL_TIM_Base_Start()"
             << "HAL_TIM_Base_Stop()";

    // 添加关键字到新API
    for (const QString &keyword : keywords) {
      newApi->add(keyword);
    }

    // 使用正则表达式查找变量定义 - 改进的正则表达式
    QRegExp varRegex(
        "\\b(int|char|float|double|uint8_t|uint16_t|uint32_t|int8_t|int16_t|"
        "int32_t|bool|void|unsigned|long|short|signed|struct|enum|union)\\s+(["
        "a-zA-Z_][a-zA-Z0-9_]*)\\s*[;\\[=,)]");

    int pos = 0;
    QSet<QString> variables; // 使用集合避免重复

    while ((pos = varRegex.indexIn(text, pos)) != -1) {
      QString varName = varRegex.cap(2);
      if (!varName.isEmpty() && !keywords.contains(varName)) {
        variables.insert(varName);
        qDebug() << "找到变量: " << varName;
      }
      pos += varRegex.matchedLength();
    }

    // 查找函数参数 - 改进的正则表达式
    QRegExp funcRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^\\)]*)\\)");
    pos = 0;

    while ((pos = funcRegex.indexIn(text, pos)) != -1) {
      QString params = funcRegex.cap(2);
      QStringList paramList = params.split(',');

      for (const QString &param : paramList) {
        // 解析参数定义，例如 "int value" 或 "char* buffer"
        QRegExp paramRegex(
            "\\b(\\w+)\\s+([\\*&]*)\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\b");
        if (paramRegex.indexIn(param) != -1) {
          QString varName = paramRegex.cap(3);
          if (!varName.isEmpty() && !keywords.contains(varName)) {
            variables.insert(varName);
            qDebug() << "找到参数: " << varName;
          }
        }
      }

      pos += funcRegex.matchedLength();
    }

    // 查找for循环中的变量 - 改进的正则表达式
    QRegExp forRegex("for\\s*\\(\\s*(?:int|char|float|double|uint8_t|uint16_t|"
                     "uint32_t)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=");
    pos = 0;

    while ((pos = forRegex.indexIn(text, pos)) != -1) {
      QString varName = forRegex.cap(1);
      if (!varName.isEmpty() && !keywords.contains(varName)) {
        variables.insert(varName);
        qDebug() << "找到for循环变量: " << varName;
      }
      pos += forRegex.matchedLength();
    }

    // 查找结构体和类定义中的成员变量
    QRegExp structRegex("struct\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\{([^}]*)\\}");
    pos = 0;

    while ((pos = structRegex.indexIn(text, pos)) != -1) {
      QString structBody = structRegex.cap(2);
      QRegExp memberRegex("\\b(\\w+)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*[;\\[]");
      int memberPos = 0;

      while ((memberPos = memberRegex.indexIn(structBody, memberPos)) != -1) {
        QString memberName = memberRegex.cap(2);
        if (!memberName.isEmpty() && !keywords.contains(memberName)) {
          variables.insert(memberName);
          qDebug() << "找到结构体成员: " << memberName;
        }
        memberPos += memberRegex.matchedLength();
      }

      pos += structRegex.matchedLength();
    }

    // 将找到的变量添加到API
    for (const QString &var : variables) {
      newApi->add(var);
    }

    // 准备新API
    newApi->prepare();

    // 替换旧的API
    if (m_apiCPP) {
      delete m_apiCPP;
    }
    m_apiCPP = newApi;

    // 设置新的API到词法分析器
    m_lexerCPP->setAPIs(m_apiCPP);

    // 设置自动补全模式，确保变量补全生效
    for (QsciScintilla *editor : m_editors) {
      editor->setAutoCompletionSource(
          QsciScintilla::AcsAll); // 使用所有可用的补全源
    }
  } catch (const std::exception &e) {
    qDebug() << "更新变量列表时发生异常: " << e.what();
    // 确保在异常情况下释放资源
  } catch (...) {
    qDebug() << "更新变量列表时发生未知异常";
    // 确保在异常情况下释放资源
  }
}

void CodeEditor::onEditorChanged(QsciScintilla *editor) {
  if (!editor) {
    qDebug() << "编辑器切换失败: 编辑器指针为空";
    return;
  }

  try {
    if (m_editors.contains(editor)) {
      m_currentEditor = editor;
    }
  } catch (const std::exception &e) {
    qDebug() << "更新变量列表时发生异常: " << e.what();
    // 确保在异常情况下释放资源
  } catch (...) {
    qDebug() << "更新变量列表时发生未知异常";
    // 确保在异常情况下释放资源
  }
}

// 添加创建工具栏的方法
void CodeEditor::createToolBar() {
  m_toolBar = new QToolBar("编辑器工具栏", this);
  m_toolBar->setMovable(true);
  m_toolBar->setIconSize(QSize(16, 16));

  // 创建动作

  // 使用Qt内置图标
  QStyle *style = QApplication::style();

  // 文件操作
  // QAction* newFileAction = new QAction("新建文件", this);
  // newFileAction->setIcon(style->standardIcon(QStyle::SP_FileIcon));
  // newFileAction->setToolTip("创建新文件");
  // connect(newFileAction, &QAction::triggered, [this]() {
  //     // 实现新建文件功能
  //     emit newFileRequested();
  // });

  QAction *newFileAction = new QAction("新建文件", this);
  newFileAction->setIcon(style->standardIcon(QStyle::SP_FileIcon));
  newFileAction->setToolTip("创建新文件");
  connect(newFileAction, &QAction::triggered, this, &CodeEditor::createNewFile);
  m_toolBar->addAction(newFileAction);

  QAction *openFileAction = new QAction("打开文件", this);
  openFileAction->setIcon(style->standardIcon(QStyle::SP_DialogOpenButton));
  openFileAction->setToolTip("打开文件");
  connect(openFileAction, &QAction::triggered, [this]() {
    // 实现打开文件功能
    emit openFileRequested();
  });
  m_toolBar->addAction(openFileAction);

  QAction *saveFileAction = new QAction("保存文件", this);
  saveFileAction->setIcon(style->standardIcon(QStyle::SP_DialogSaveButton));
  saveFileAction->setToolTip("保存文件");
  connect(saveFileAction, &QAction::triggered, [this]() {
    // 实现保存文件功能
    emit saveFileRequested();
  });
  m_toolBar->addAction(saveFileAction);

  m_toolBar->addSeparator();

  // 水平分栏动作
  QAction *horizontalSplitAction =
      new QAction(QIcon(":/icons/horizontal_split.png"), "水平分栏", this);
  horizontalSplitAction->setToolTip("创建水平分栏");
  connect(horizontalSplitAction, &QAction::triggered,
          [this]() { createSplitView(Qt::Horizontal); });
  m_toolBar->addAction(horizontalSplitAction);

  // 垂直分栏动作
  QAction *verticalSplitAction =
      new QAction(QIcon(":/icons/vertical_split.png"), "垂直分栏", this);
  verticalSplitAction->setToolTip("创建垂直分栏");
  connect(verticalSplitAction, &QAction::triggered,
          [this]() { createSplitView(Qt::Vertical); });
  m_toolBar->addAction(verticalSplitAction);

  // 关闭分栏动作
  QAction *closeSplitAction =
      new QAction(QIcon(":/icons/close_split.png"), "关闭分栏", this);
  closeSplitAction->setToolTip("关闭当前分栏");
  connect(closeSplitAction, &QAction::triggered,
          [this]() { closeSplitView(); });
  m_toolBar->addAction(closeSplitAction);

  m_toolBar->addSeparator();

  // 撤销动作
  QAction *undoAction = new QAction(QIcon(":/icons/undo.png"), "撤销", this);
  undoAction->setToolTip("撤销上一步操作");
  connect(undoAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->undo();
    }
  });
  m_toolBar->addAction(undoAction);

  // 重做动作
  QAction *redoAction = new QAction(QIcon(":/icons/redo.png"), "重做", this);
  redoAction->setToolTip("重做上一步操作");
  connect(redoAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->redo();
    }
  });
  m_toolBar->addAction(redoAction);

  m_toolBar->addSeparator();

  // 剪切动作
  QAction *cutAction = new QAction(QIcon(":/icons/cut.png"), "剪切", this);
  cutAction->setToolTip("剪切选中的文本");
  connect(cutAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->cut();
    }
  });
  m_toolBar->addAction(cutAction);

  // 复制动作
  QAction *copyAction = new QAction(QIcon(":/icons/copy.png"), "复制", this);
  copyAction->setToolTip("复制选中的文本");
  connect(copyAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->copy();
    }
  });
  m_toolBar->addAction(copyAction);

  // 粘贴动作
  QAction *pasteAction = new QAction(QIcon(":/icons/paste.png"), "粘贴", this);
  pasteAction->setToolTip("粘贴文本");
  connect(pasteAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->paste();
    }
  });
  m_toolBar->addAction(pasteAction);

  m_toolBar->addSeparator();

  // 查找动作
  QAction *findAction = new QAction(QIcon(":/icons/find.png"), "查找", this);
  findAction->setToolTip("查找文本");
  connect(findAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // 这里可以实现查找功能，或者调用已有的查找方法
      // 暂时使用简单的实现
      m_currentEditor->findFirst("", false, false, false, true);
    }
  });
  m_toolBar->addAction(findAction);

  // 替换动作
  QAction *replaceAction =
      new QAction(QIcon(":/icons/replace.png"), "替换", this);
  replaceAction->setToolTip("替换文本");
  connect(replaceAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // 这里可以实现替换功能，或者调用已有的替换方法
      // 暂时使用简单的实现
      // 需要更复杂的实现可以添加一个替换对话框
    }
  });
  m_toolBar->addAction(replaceAction);

  m_toolBar->addSeparator();

  // 缩进动作
  QAction *indentAction =
      new QAction(QIcon(":/icons/indent.png"), "增加缩进", this);
  indentAction->setToolTip("增加选中文本的缩进");
  connect(indentAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // QsciScintilla doesn't have an indent() method
      // Instead, we need to manually insert spaces or tabs at the beginning of
      // selected lines
      int lineFrom, indexFrom, lineTo, indexTo;
      m_currentEditor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

      // If no selection, use current line
      if (lineFrom == -1) {
        // Fix: getCursorPosition requires two parameters
        m_currentEditor->getCursorPosition(&lineFrom, &indexFrom);
        lineTo = lineFrom;
      }

      // Begin undo action
      m_currentEditor->beginUndoAction();

      // Add indentation to each line
      for (int line = lineFrom; line <= lineTo; ++line) {
        m_currentEditor->insertAt("    ", line, 0); // Insert 4 spaces
      }

      // End undo action
      m_currentEditor->endUndoAction();
    }
  });
  m_toolBar->addAction(indentAction);

  // 取消缩进动作
  QAction *unindentAction =
      new QAction(QIcon(":/icons/unindent.png"), "减少缩进", this);
  unindentAction->setToolTip("减少选中文本的缩进");
  connect(unindentAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // QsciScintilla doesn't have an unindent() method
      // Instead, we need to manually remove spaces or tabs from the beginning
      // of selected lines
      int lineFrom, indexFrom, lineTo, indexTo;
      m_currentEditor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

      // If no selection, use current line
      if (lineFrom == -1) {
        // Fix: getCursorPosition requires two parameters
        m_currentEditor->getCursorPosition(&lineFrom, &indexFrom);
        lineTo = lineFrom;
      }

      // Begin undo action
      m_currentEditor->beginUndoAction();

      // Remove indentation from each line
      for (int line = lineFrom; line <= lineTo; ++line) {
        QString lineText = m_currentEditor->text(line);
        if (lineText.startsWith("    ")) {
          // Remove 4 spaces
          m_currentEditor->setSelection(line, 0, line, 4);
          m_currentEditor->removeSelectedText();
        } else if (lineText.startsWith("\t")) {
          // Remove tab
          m_currentEditor->setSelection(line, 0, line, 1);
          m_currentEditor->removeSelectedText();
        } else if (lineText.startsWith("  ")) {
          // Remove 2 spaces
          m_currentEditor->setSelection(line, 0, line, 2);
          m_currentEditor->removeSelectedText();
        } else if (lineText.startsWith(" ")) {
          // Remove 1 space
          m_currentEditor->setSelection(line, 0, line, 1);
          m_currentEditor->removeSelectedText();
        }
      }

      // End undo action
      m_currentEditor->endUndoAction();
    }
  });
  m_toolBar->addAction(unindentAction);

  m_toolBar->addSeparator();

  // 注释动作
  QAction *commentAction =
      new QAction(QIcon(":/icons/comment.png"), "注释", this);
  commentAction->setToolTip("注释选中的代码");
  connect(commentAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // 获取选中的文本范围
      int lineFrom, indexFrom, lineTo, indexTo;
      m_currentEditor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

      // 如果没有选中文本，则使用当前行
      if (lineFrom == -1) {
        // Fix: getCursorPosition requires two parameters
        m_currentEditor->getCursorPosition(&lineFrom, &indexFrom);
        lineTo = lineFrom;
        indexTo = indexFrom;
      }

      // 为每一行添加注释
      for (int line = lineFrom; line <= lineTo; ++line) {
        m_currentEditor->insertAt("// ", line, 0);
      }
    }
  });
  m_toolBar->addAction(commentAction);
  // 取消注释动作
  QAction *uncommentAction =
      new QAction(QIcon(":/icons/uncomment.png"), "取消注释", this);
  uncommentAction->setToolTip("取消选中代码的注释");
  connect(uncommentAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // 获取选中的文本范围
      int lineFrom, indexFrom, lineTo, indexTo;
      m_currentEditor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

      // 如果没有选中文本，则使用当前行
      if (lineFrom == -1) {
        // Fix: getCursorPosition requires two parameters
        m_currentEditor->getCursorPosition(&lineFrom, &indexFrom);
        lineTo = lineFrom;
        indexTo = indexFrom;
      }

      // 为每一行移除注释
      for (int line = lineFrom; line <= lineTo; ++line) {
        QString lineText = m_currentEditor->text(line);
        if (lineText.startsWith("// ")) {
          m_currentEditor->setSelection(line, 0, line, 3);
          m_currentEditor->removeSelectedText();
        } else if (lineText.startsWith("//")) {
          m_currentEditor->setSelection(line, 0, line, 2);
          m_currentEditor->removeSelectedText();
        }
      }
    }
  });
  m_toolBar->addAction(uncommentAction);

  // 构建操作
  QAction *buildAction = new QAction("构建", this);
  buildAction->setIcon(style->standardIcon(QStyle::SP_ComputerIcon));
  buildAction->setToolTip("构建项目");
  connect(buildAction, &QAction::triggered, [this]() {
    // 实现构建功能
    emit buildRequested();
  });
  m_toolBar->addAction(buildAction);

  QAction *cleanAction = new QAction("清理", this);
  cleanAction->setIcon(style->standardIcon(QStyle::SP_TrashIcon));
  cleanAction->setToolTip("清理构建文件");
  connect(cleanAction, &QAction::triggered, [this]() {
    // 实现清理功能
    emit cleanRequested();
  });
  m_toolBar->addAction(cleanAction);

  m_toolBar->addSeparator();

  // 调试操作
  QAction *debugAction = new QAction("调试", this);
  debugAction->setIcon(style->standardIcon(QStyle::SP_BrowserReload));
  debugAction->setToolTip("开始调试");
  connect(debugAction, &QAction::triggered, [this]() {
    // 实现调试功能
    emit debugRequested();
  });
  m_toolBar->addAction(debugAction);

  QAction *runAction = new QAction("运行", this);
  runAction->setIcon(style->standardIcon(QStyle::SP_MediaPlay));
  runAction->setToolTip("运行程序");
  connect(runAction, &QAction::triggered, [this]() {
    // 实现运行功能
    emit runRequested();
  });
  m_toolBar->addAction(runAction);

  QAction *stopAction = new QAction("停止", this);
  stopAction->setIcon(style->standardIcon(QStyle::SP_MediaStop));
  stopAction->setToolTip("停止运行");
  connect(stopAction, &QAction::triggered, [this]() {
    // 实现停止功能
    emit stopRequested();
  });
  m_toolBar->addAction(stopAction);

  m_toolBar->addSeparator();

  // 工具操作
  QAction *serialMonitorAction = new QAction("串口监视器", this);
  serialMonitorAction->setIcon(style->standardIcon(QStyle::SP_ComputerIcon));
  serialMonitorAction->setToolTip("打开串口监视器");
  connect(serialMonitorAction, &QAction::triggered, [this]() {
    // 实现串口监视器功能
    emit serialMonitorRequested();
  });
  m_toolBar->addAction(serialMonitorAction);

  QAction *settingsAction = new QAction("设置", this);
  settingsAction->setIcon(
      style->standardIcon(QStyle::SP_FileDialogDetailedView));
  settingsAction->setToolTip("打开设置");
  connect(settingsAction, &QAction::triggered, [this]() {
    // 实现设置功能
    emit settingsRequested();
  });
  m_toolBar->addAction(settingsAction);

  m_toolBar->addSeparator();

  // 视图操作
  QAction *zoomInAction = new QAction("放大", this);
  zoomInAction->setIcon(style->standardIcon(QStyle::SP_TitleBarMaxButton));
  zoomInAction->setToolTip("放大视图");
  connect(zoomInAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->zoomIn();
    }
  });
  m_toolBar->addAction(zoomInAction);

  QAction *zoomOutAction = new QAction("缩小", this);
  zoomOutAction->setIcon(style->standardIcon(QStyle::SP_TitleBarMinButton));
  zoomOutAction->setToolTip("缩小视图");
  connect(zoomOutAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->zoomOut();
    }
  });
  m_toolBar->addAction(zoomOutAction);

  QAction *resetZoomAction = new QAction("重置缩放", this);
  resetZoomAction->setIcon(
      style->standardIcon(QStyle::SP_TitleBarNormalButton));
  resetZoomAction->setToolTip("重置缩放级别");
  connect(resetZoomAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->zoomTo(0);
    }
  });
  m_toolBar->addAction(resetZoomAction);

  m_toolBar->addSeparator();

  // 帮助操作
  QAction *helpAction = new QAction("帮助", this);
  helpAction->setIcon(style->standardIcon(QStyle::SP_MessageBoxQuestion));
  helpAction->setToolTip("查看帮助");
  connect(helpAction, &QAction::triggered, [this]() {
    // 实现帮助功能
    emit helpRequested();
  });
  m_toolBar->addAction(helpAction);

  QAction *aboutAction = new QAction("关于", this);
  aboutAction->setIcon(style->standardIcon(QStyle::SP_MessageBoxInformation));
  aboutAction->setToolTip("关于STM32IDE");
  connect(aboutAction, &QAction::triggered, [this]() {
    // 实现关于功能
    emit aboutRequested();
  });
  m_toolBar->addAction(aboutAction);
}

void CodeEditor::createNewFile() {
  // 清空当前编辑器内容
  if (m_currentEditor) {
    m_currentEditor->clear();
  }

  // 重置当前文件路径
  m_currentFilePath = "";

  // 发出信号通知主窗口
  emit newFileRequested();
}

// 添加辅助函数来判断是否为构造函数
bool CodeEditor::isLikelyConstructor(const QString &functionName,
                                     const QString &line) {
  // 构造函数通常满足以下条件：
  // 1. 函数名首字母大写（类名规范）
  // 2. 不包含返回类型
  // 3. 可能包含初始化列表

  if (functionName.isEmpty()) {
    return false;
  }

  // 检查首字母是否大写
  if (!functionName[0].isUpper()) {
    return false;
  }

  // 检查行中是否没有明显的返回类型
  QString trimmed = line.trimmed();

  // 如果行以常见的返回类型开始，则不太可能是构造函数
  QStringList returnTypes = {
      "int",     "void",     "char",     "float",  "double",  "bool",
      "uint8_t", "uint16_t", "uint32_t", "int8_t", "int16_t", "int32_t",
      "std::",   "const",    "static",   "inline", "virtual"};

  for (const QString &type : returnTypes) {
    if (trimmed.startsWith(type + " ")) {
      return false;
    }
  }

  // 如果包含初始化列表（冒号），更可能是构造函数
  if (line.contains(":") && !line.contains("?")) { // 排除三元运算符
    return true;
  }

  return true; // 默认认为可能是构造函数
}

// bool CodeEditor::isValidFunctionDefinition(const QString& line, int lineNum,
// const QStringList& allLines) {
//     QString trimmed = line.trimmed();

//     // 排除预处理指令（除了函数式宏）
//     if (trimmed.startsWith("#") && !trimmed.startsWith("#define")) {
//         return false;
//     }

//     // 排除明显的控制结构
//     if (trimmed.startsWith("if") || trimmed.startsWith("for") ||
//         trimmed.startsWith("while") || trimmed.startsWith("switch") ||
//         trimmed.startsWith("else") || trimmed.startsWith("do")) {
//         return false;
//     }

//     // 排除变量声明（包含分号但不包含括号的行）
//     if (trimmed.endsWith(";") && !trimmed.contains("(")) {
//         return false;
//     }

//     // 排除函数声明（以分号结尾）
//     if (trimmed.endsWith(";")) {
//         return false;
//     }

//     // 检查是否有函数体（大括号）
//     if (line.contains("{")) {
//         return true;
//     }

//     // 检查接下来的更多行是否有开括号（扩展搜索范围）
//     for (int i = 1; i <= 10 && (lineNum + i) < allLines.size(); i++) {
//         QString nextLine = allLines[lineNum + i].trimmed();
//         if (nextLine.startsWith("{")) {
//             return true;
//         }
//         // 如果遇到其他函数定义或类定义，停止搜索
//         if (nextLine.contains("(") && nextLine.contains(")") &&
//         !nextLine.startsWith("//")) {
//             break;
//         }
//         // 如果遇到分号，可能是函数声明，停止搜索
//         if (nextLine.endsWith(";")) {
//             break;
//         }
//     }

//     return false;
// }

bool CodeEditor::isValidFunctionDefinition(const QString &line, int lineNum,
                                           const QStringList &allLines) {
  QString trimmed = line.trimmed();

  // 排除预处理指令（除了函数式宏）
  if (trimmed.startsWith("#") && !trimmed.startsWith("#define")) {
    return false;
  }

  // 排除明显的控制结构
  if (trimmed.startsWith("if") || trimmed.startsWith("for") ||
      trimmed.startsWith("while") || trimmed.startsWith("switch") ||
      trimmed.startsWith("else") || trimmed.startsWith("do")) {
    return false;
  }

  // 排除变量声明（包含分号但不包含括号的行）
  if (trimmed.endsWith(";") && !trimmed.contains("(")) {
    return false;
  }

  // 排除函数声明（以分号结尾）
  if (trimmed.endsWith(";")) {
    return false;
  }

  // 检查是否有函数体（大括号）
  if (line.contains("{")) {
    return true;
  }

  // 扩展搜索范围到20行，并改进搜索逻辑
  for (int i = 1; i <= 20 && (lineNum + i) < allLines.size(); i++) {
    QString nextLine = allLines[lineNum + i].trimmed();

    // 跳过空行和注释
    if (nextLine.isEmpty() || nextLine.startsWith("//") ||
        nextLine.startsWith("/*")) {
      continue;
    }

    if (nextLine.startsWith("{")) {
      return true;
    }

    // 如果遇到明显的新函数定义，停止搜索
    if (nextLine.contains("{") &&
        (nextLine.contains("class") || nextLine.contains("struct"))) {
      break;
    }

    // 如果遇到分号且不是在括号内，可能是函数声明
    if (nextLine.endsWith(";") && !nextLine.contains("(")) {
      break;
    }

    // 如果遇到另一个函数定义的开始（包含返回类型和函数名模式）
    if (i > 5 && nextLine.contains("(") && nextLine.contains(")") &&
        !nextLine.startsWith("if") && !nextLine.startsWith("for") &&
        !nextLine.startsWith("while") && !nextLine.startsWith("switch")) {
      // 检查是否像函数定义
      QRegularExpression funcPattern(R"(\b\w+\s*\([^)]*\))");
      if (funcPattern.match(nextLine).hasMatch()) {
        break;
      }
    }
  }

  return false;
}

void CodeEditor::setupRainbowBrackets(QsciScintilla *editor) {
  // Define unique indicators for varying nesting levels
  // Use indicators 21-26

  QList<QColor> colors;
  if (m_isDarkTheme) {
    // Dark Theme Colors (Vibrant)
    colors << QColor("#FFD700")  // Level 1: Gold
           << QColor("#DA70D6")  // Level 2: Orchid
           << QColor("#179FFF")  // Level 3: SkyBlue
           << QColor("#FFB6C1")  // Level 4: LightPink
           << QColor("#00FFFF")  // Level 5: Cyan
           << QColor("#7CFC00"); // Level 6: LawnGreen
  } else {
    // Light Theme Colors (Darker for contrast)
    colors << QColor("#CC9900")  // Level 1
           << QColor("#800080")  // Level 2
           << QColor("#0000FF")  // Level 3
           << QColor("#C71585")  // Level 4
           << QColor("#008080")  // Level 5
           << QColor("#228B22"); // Level 6
  }

  for (int i = 0; i < 6; ++i) {
    int indicator = RAINBOW_LEVEL_1 + i;
    editor->indicatorDefine(QsciScintilla::PlainIndicator,
                            indicator); // Simple text coloring
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, indicator,
                          QsciScintilla::INDIC_TEXTFORE);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, indicator,
                          (colors[i].blue() << 16) | (colors[i].green() << 8) |
                              colors[i].red());
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, indicator, 255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, indicator, false);
  }

  // Connect signals for dynamic highlighting
  // Do not disconnect everything!
  // disconnect(editor, &QsciScintilla::textChanged, this, nullptr);
  // Note: Use a unique connection for this specific lambda/slot to avoid
  // duplicates if called multiple times, but lambda pointers are different. We
  // rely on the fact that existing connections are not easily removable by
  // lambda. However, if we call setup multiple times (theme change), we might
  // stack connections. Solution: Connect to a named slot or manage connections.
  // For now, we will assume this is acceptable or rely on a "setup" flag if
  // needed. actually, let's use a timer for debounce like function highlight.

  // We'll reuse the debounce mechanism if possible, or create a new one.
  // Since we are inside setup, let's just connect directly for now with a
  // simple debounce check.

  // Better: Connect to a slot `onTextChangedForRainbow` if we had one.
  // Or just connect once.
  // We already have `updateFunctionList` and `updateVariableList` connected to
  // textChanged. Let's add one more connection.
  connect(editor, &QsciScintilla::textChanged, [this, editor]() {
    QTimer::singleShot(100,
                       [this, editor]() { highlightRainbowBrackets(editor); });
  });

  // Initial highlight
  highlightRainbowBrackets(editor);
}

void CodeEditor::highlightRainbowBrackets(QsciScintilla *editor) {
  if (!editor)
    return;

  // Clear existing rainbow indicators
  int len = editor->length();
  for (int i = 0; i < 6; ++i) {
    editor->clearIndicatorRange(0, 0, editor->lines(), len,
                                RAINBOW_LEVEL_1 + i);
  }

  QString text = editor->text();
  // QsciAPIs or Lexer might be needed for style check.
  // We can use SendScintilla(SCI_GETSTYLEAT, pos)

  // We will scan the text.
  // To be efficient, we might want to get all text.
  // Note: iterating char by char on large files in main thread is slow.
  // But for this task, it's the standard way unless we write a C++ Scintilla
  // Lexer.

  int depth = 0;
  // We need to track actual positions.
  // text index matches Scintilla position closely for UTF-8?
  // QsciScintilla uses bytes?
  // text() returns QString (UTF-16). SCI_GETSTYLEAT expects byte position.
  // This is a mismatch risk.
  // Safest is to loop bytes if we use SCI_GETSTYLEAT.

  // Let's use Scintilla's byte-based access.
  int length = editor->SendScintilla(QsciScintilla::SCI_GETTEXTLENGTH);
  if (length <= 0)
    return;

  std::vector<char> buffer(length + 1);
  editor->SendScintilla(QsciScintilla::SCI_GETTEXT, length + 1, buffer.data());

  // Define bracket chars
  // ( ) [ ] { }

  for (int i = 0; i < length; ++i) {
    char c = buffer[i];

    bool isOpener = (c == '(' || c == '[' || c == '{');
    bool isCloser = (c == ')' || c == ']' || c == '}');

    if (!isOpener && !isCloser)
      continue;

    // Check style
    int style = editor->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, i);
    bool isCommentOrString =
        (style == QsciLexerCPP::Comment || style == QsciLexerCPP::CommentLine ||
         style == QsciLexerCPP::CommentDoc ||
         style == QsciLexerCPP::DoubleQuotedString ||
         style == QsciLexerCPP::SingleQuotedString);

    if (isCommentOrString)
      continue;

    int colorLevel = 0;

    if (isOpener) {
      colorLevel = depth % 6;
      depth++;
    } else if (isCloser) {
      depth--;
      if (depth < 0)
        depth = 0;
      colorLevel = depth % 6;
    }

    // Apply indicator
    // calculate line and index? No, SCI_INDICATORFILLRANGE takes pos and
    // length. But QsciScintilla wrapper functions take line/index. We can use
    // low-level SCI calls.
    int indicator = RAINBOW_LEVEL_1 + colorLevel;

    editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, indicator);
    editor->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, i, 1);
  }
}
