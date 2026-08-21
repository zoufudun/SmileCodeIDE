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
#include <QDir>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QSet>
#include <QSplitter>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <Qsci/qsciapis.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qsciscintilla.h>

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

  // 面包屑导航栏控件 (VS Code 风格)
  QWidget *m_breadcrumbBar;
  QWidget *m_bcPathContainer;
  QHBoxLayout *m_bcPathLayout;
  QToolButton *m_bcFileButton;
  QToolButton *m_bcFuncButton;
  QToolButton *m_outlineToggleBtn;

  // 函数大纲列表容器与控件 (升级为 QTreeWidget 支持类与成员二级收缩/展开)
  QWidget *m_functionListContainer;
  QLabel *m_outlineTitleLabel;
  QTreeWidget *m_functionTree;

public:
  // 符号类型定义 (函数/全局变量/宏定义/类/结构体/联合体/枚举)
  enum SymbolType {
    SymbolFunction,
    SymbolVariable,
    SymbolMacro,
    SymbolClass,
    SymbolStruct,
    SymbolUnion,
    SymbolEnum
  };

  // 存储符号信息的结构体
  struct FunctionInfo {
    QString scopedName; // 符号名 (如 Key_Scan, uart_buff, RoomWidget, roomId)
    QString returnType; // 返回值/类型 (如 void, uint8_t, class, QString)
    QString params;     // 参数列表/数组大小/修饰符 (如 "(int)", "[1024]", "() const")
    SymbolType type = SymbolFunction; // 符号类型
    int startLine = 0;  // 符号起始行号 (0-indexed)
    int startCol = 0;   // 符号名称起始列 (0-indexed)
    int endLine = 0;    // 结束行号 (0-indexed)
    int refCount = 0;   // 在当前文件中的引用/调用次数 (如 +9, 6, 2, 1)
    int indentLevel = 0;// 缩进层级 (0为顶级，1为类成员)
    QString parentClass;// 所属父类名称

    bool operator==(const FunctionInfo &other) const {
      return scopedName == other.scopedName &&
             startLine == other.startLine &&
             parentClass == other.parentClass &&
             type == other.type;
    }
  };

private:
  // 符号信息列表
  QList<FunctionInfo> m_functions;

  // 主题相关
  bool m_isDarkTheme;

  // 解析代码中的符号（函数、全局/静态变量、宏定义、类与成员）
  void parseFunctions(const QString &code);

  // 更新函数列表与大纲
  void updateFunctionList();

  // 精准跳转并高亮符号名称
  void navigateToSymbol(const FunctionInfo &info);

  // 创建函数列表控件
  void createFunctionList();

  // 控制函数大纲显隐
  void setFunctionOutlineVisible(bool visible);
  void toggleFunctionOutline();

  // 创建面包屑导航栏
  void createBreadcrumbBar();

  // 更新面包屑显示
  void updateBreadcrumb(int cursorLine = -1);

  // 弹出同目录其他 C/C++ 源文件与头文件切换菜单
  void showBreadcrumbFilesMenu();

  // 弹出函数快速跳转菜单
  void showBreadcrumbFunctionsMenu();

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
