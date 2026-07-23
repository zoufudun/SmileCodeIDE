/*
 * @Description:
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-05 21:43:38
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-05-03 17:49:12
 */
#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QDebug>
#include <QDockWidget> // 添加停靠窗口头文件
#include <QListWidget> // 添加列表控件头文件
#include <QSet>
#include <QSplitter>
#include <QToolBar> // 添加工具栏头文件
#include <QWidget>
#include <Qsci/qsciapis.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qsciscintilla.h>

// Constants
// Constants
static const int FUNCTION_INDICATOR = 20;

// Rainbow Brackets Indicators (21-26)
static const int RAINBOW_LEVEL_1 = 21;
static const int RAINBOW_LEVEL_2 = 22;
static const int RAINBOW_LEVEL_3 = 23;
static const int RAINBOW_LEVEL_4 = 24;
static const int RAINBOW_LEVEL_5 = 25;
static const int RAINBOW_LEVEL_6 = 26;

class CodeEditor : public QWidget {
  Q_OBJECT

public:
  explicit CodeEditor(QWidget *parent = nullptr);
  ~CodeEditor();

  // 获取当前编辑器
  QsciScintilla *currentEditor() const;

  // 获取所有编辑器
  QList<QsciScintilla *> allEditors() const;

  // 设置文本内容
  void setText(const QString &text);

  // 获取文本内容
  QString text() const;

  // 打开文件
  bool openFile(const QString &filePath);

  // 保存文件
  bool saveFile(const QString &filePath);

  // 应用主题
  void applyTheme(const QString &themeName);

  // 创建新的编辑器分栏
  void createSplitView(Qt::Orientation orientation = Qt::Horizontal);

  // 关闭当前分栏
  void closeSplitView();

  void createNewFile();
  void setupFunctionHighlight(QsciScintilla *editor);
  void setDarkTheme(bool isDark) { m_isDarkTheme = isDark; }
  bool isDarkTheme() const { return m_isDarkTheme; }

public slots:
  // 更新变量列表用于自动补全
  void updateVariableList();
  bool isValidFunctionDefinition(const QString &line, int lineNum,
                                 const QStringList &allLines);
  bool isLikelyConstructor(const QString &functionName, const QString &line);

private slots:
  // 当前编辑器变更
  void onEditorChanged(QsciScintilla *editor);
  // Slot to handle line number highlighting
  void highlightCurrentLineNumber();

private:
  // 设置编辑器基本属性
  void setupEditor(QsciScintilla *editor);

  // 设置自动补全
  void setupAutoCompletion(QsciScintilla *editor);

  // 创建工具栏
  void createToolBar();

  // 主分割器
  QSplitter *m_mainSplitter;

  // 当前活动的编辑器
  QsciScintilla *m_currentEditor;

  // 所有编辑器列表
  QList<QsciScintilla *> m_editors;

  // C++词法分析器
  QsciLexerCPP *m_lexerCPP;

  // API对象用于自动补全
  QsciAPIs *m_apiCPP;

  // 当前文件路径
  QString m_currentFilePath;

  // 工具栏
  QToolBar *m_toolBar;

  // 函数列表控件
  QListWidget *m_functionList;

  // 存储函数信息的结构体
  struct FunctionInfo {
    QString name;      // 函数名
    int line;          // 行号
    QString signature; // 函数签名
  };

  // 函数信息列表
  QList<FunctionInfo> m_functions;

  // 主题相关
  bool m_isDarkTheme;

  // 解析代码中的函数
  void parseFunctions(const QString &code);

  // 更新函数列表
  void updateFunctionList();

  // 创建函数列表控件
  void createFunctionList();

  // 括号高亮
  void updateBracketHighlighting(QsciScintilla *editor);

  // Track the last active line for highlighting
  int m_previousLine = -1;

  // 函数名高亮
  void setupFunctionNameHighlighting(bool isDarkTheme);
  void highlightFunctionNames(QsciScintilla *editor, int indicatorId);

  // Rainbow Brackets Setting
  void setupRainbowBrackets(QsciScintilla *editor);

  QsciScintilla *findFirstEditor(QSplitter *splitter);

signals:
  // 文件操作信号
  void newFileRequested();
  void openFileRequested();
  void saveFileRequested();

  // 构建操作信号
  void buildRequested();
  void cleanRequested();

  // 调试操作信号
  void debugRequested();
  void runRequested();
  void stopRequested();

  // 工具操作信号
  void serialMonitorRequested();
  void settingsRequested();

  // 帮助操作信号
  void helpRequested();
  void aboutRequested();
};

#endif // CODEEDITOR_H
