/*
 * @Description:
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-03-26 21:02:58
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-04-08 22:35:51
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QActionGroup>
#include <QComboBox>
#include <QFileSystemModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTextEdit>
#include <QTreeView>

// 在其他包含之后添加
#include "toolchaindialog.h"
// #include "foldableeditor.h"

// 添加QCodeEditor头文件
// #include <QCodeEditor>
// #include <QCXXHighlighter>
// #include <QSyntaxStyle>
// #include <QGLSLCompleter>

// 在头文件开始处添加QScintilla相关头文件
#include <Qsci/qsciapis.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexermakefile.h>
#include <Qsci/qsciscintilla.h>

#include "codeeditor.h"

// 添加BuildSystem头文件
#include "buildsystem.h"
#include "cantool.h"
#include "terminalwidget.h"

class SerialPortPlot;
class SerialPortContainer;
class IAPTool;
class OscilloscopeWindow;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();
  // void newFile();
  bool isCurrentFileModified() const;

private slots:
  void openProject();
  void newProject();
  void saveProject();
  void buildProject();
  void cleanProject();
  void flashProject();
  void startDebug();
  void stopDebug();
  void continueDebug();
  void stepOver();
  void stepInto();
  void stepOut();
  void setBreakpoint();
  void configureToolchain();
  void processOutput();
  void processError();
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void updateProjectTree(const QString &path);
  void downloaderChanged(int index);
  void toggleFullScreen();
  void changeTheme(int themeIndex);
  void showAboutDialog();
  void updateStatusInfo();
  // Add this line for the onFileDoubleClicked slot
  void onFileDoubleClicked(const QModelIndex &index);
  void showContextMenu(const QPoint &pos); // Add this line for context menu
  // void updateVariableList();

  // 添加主题切换槽函数
  void setDarkTheme();
  void setLightTheme();
  void setOneDarkTheme();
  void setAtomMaterialTheme();
  void setAtomOneTheme();
  void setGerryTheme();
  void setMaterialIconsTheme();

  // 新增的主题切换方法
  void setGithubDarkTheme();
  void setXcodeDarkTheme();
  void setVueTheme();
  void setMonokaiProTheme();
  void setDraculaTheme();
  void setNordTheme();
  void setNoctisTheme();
  void setNightOwlTheme();
  void setSolarizedLightTheme();
  void setMaterialLightTheme();

  void applyTheme(const QString &themeName);
  void openFile(const QModelIndex &index);
  void saveCurrentFile();   // 添加保存当前文件的方法声明
  void saveFileAs();        // 添加另存为方法声明
  void executeGdbCommand(); // 添加这一行声明

  // File operations
  void newFile();
  void saveFile();

  void appendBuildOutput(const QString &output);
  void onBuildFinished(bool success);

  // 关闭当前工程
  void closeProject();

  // 处理工程关闭信号
  void onProjectClosed();

  void openSerialTool();  // 打开串口调试助手
  void openNetworkTool(); // 打开网络调试助手
  void openCANTool();     // 打开CAN调试助手
  void openIAPTool();     // 打开IAP升级工具
  void openOscilloscopeTool(); // 打开独立多通信接口数字示波器
  void openAppHub();      // 打开/切换至应用工作台主界面

  void showWelcomeScreen(); // 显示欢迎界面
  void checkForUpdates();   // 检查更新
  void newTerminal();       // 新建终端

private:
  void setupUi();
  void createActions();
  void createMenus();
  void createToolbars();
  void createCentralWidget(); // 添加这一行声明
  void loadSettings();
  void saveSettings();
  void executeCommand(const QString &command, const QStringList &arguments);

  void createStatusBar();
  void updateRecentFileActions();
  void setCurrentFile(const QString &fileName);
  void updateWindowTitle();

  // 添加菜单相关的成员变量
  QAction *m_openProjectAction;
  QAction *m_newProjectAction;
  QAction *m_saveProjectAction;
  QAction *m_saveFileAction;
  QAction *m_saveFileAsAction;
  QAction *m_exitAction;
  QAction *m_saveAction;
  QAction *m_saveAllAction;
  QAction *m_closeProjectAction;

  QAction *m_buildAction;
  QAction *m_cleanAction;
  QAction *m_flashAction;

  QAction *m_debugAction;
  QAction *m_stopDebugAction;
  QAction *m_continueAction;
  QAction *m_stepOverAction;
  QAction *m_stepIntoAction;
  QAction *m_stepOutAction;
  QAction *m_breakpointAction;

  QAction *m_configureToolchainAction;
  QAction *m_fullScreenAction;

  // Help menu actions
  QAction *m_welcomeAction;
  QAction *m_checkUpdatesAction;
  QAction *m_aboutAction;

  // Terminal actions
  QAction *m_newTerminalAction;

  QTabWidget *m_tabWidget;
  QTextEdit *m_outputConsole;
  QTextEdit *m_debugConsole;
  QLineEdit *m_commandLine;
  QPushButton *m_buildButton;
  QPushButton *m_cleanButton;
  QPushButton *m_flashButton;
  QPushButton *m_debugButton;
  QPushButton *m_stopButton;
  QPushButton *m_continueButton;
  QPushButton *m_stepOverButton;
  QPushButton *m_stepIntoButton;
  QPushButton *m_stepOutButton;
  QComboBox *m_targetComboBox;
  QTreeView *m_projectTreeView;
  QFileSystemModel *m_fileSystemModel;
  QProcess *m_process;
  QString m_projectPath;
  QString m_gccPath;
  QString m_openocdPath;
  QString m_openocdConfig;
  QSettings *m_settings;
  bool m_isDebugging;

  // 添加新的私有方法
  void setupStyle();
  void setupStatusBar();
  void setupDockWidgets();
  void setupThemeMenu();
  void loadStyleSheet(const QString &sheetName);

  QString m_currentTheme;
  QAction *m_darkThemeAction;
  QAction *m_lightThemeAction;
  QAction *m_oneDarkThemeAction;
  QAction *m_atomMaterialThemeAction;
  QAction *m_atomOneThemeAction;
  QAction *m_gerryThemeAction;
  QAction *m_materialIconsThemeAction;
  QActionGroup *m_themeActionGroup;

  // 新增的主题动作变量
  QAction *m_githubDarkThemeAction;
  QAction *m_xcodeDarkThemeAction;
  QAction *m_vueThemeAction;
  QAction *m_monokaiProThemeAction;
  QAction *m_draculaThemeAction;
  QAction *m_nordThemeAction;
  QAction *m_noctisThemeAction;
  QAction *m_nightOwlThemeAction;
  QAction *m_solarizedLightThemeAction;
  QAction *m_materialLightThemeAction;

  // 添加新的私有方法
  QDockWidget *m_projectDock;
  QDockWidget *m_consoleDock;
  QButtonGroup *m_themeGroup;
  QLabel *m_statusProjectLabel;
  QLabel *m_statusTargetLabel;
  QLabel *m_statusBuildLabel;
  // 在private成员变量部分添加
  QComboBox *m_downloaderComboBox; // 下载工具选择下拉框
  QString m_currentDownloader;     // 当前选择的下载工具
  QString m_currentFilePath;

  // 添加代码编辑器
  CodeEditor *m_codeEditor;
  // QsciScintilla* m_codeEditor2;

  // 在MainWindow类的private部分添加QScintilla相关成员变量
  QsciScintilla *m_editor; // QScintilla编辑器

  QsciLexerCPP *m_lexerCPP;       // C++语法高亮器
  QsciLexerMakefile *m_lexerMake; // Makefile语法高亮器
  QsciAPIs *m_apiCPP;             // C++自动补全API

  // 添加设置编辑器的方法
  void setupEditor();
  // void setupAutoCompletion();
  void loadFile(const QString &filePath);

  // 分栏动作
  QAction *m_horizontalSplitAction;
  QAction *m_verticalSplitAction;
  QAction *m_closeSplitAction;

  // 构建系统
  BuildSystem *m_buildSystem;

  // 输出窗口
  QTextEdit *m_outputWindow;

  // 检查是否有未保存的更改
  bool hasUnsavedChanges();

  // 保存所有文件
  void saveAllFiles();

  // 关闭所有编辑器
  void closeAllEditors();

  // 清空项目树
  void clearProjectTree();

  // 更新菜单状态
  void updateMenuState();

  bool saveEditorContent(QsciScintilla *editor, const QString &filePath);

  /* Existing code */
  QAction *m_appHubAction;      // 应用工作台主界面动作
  QAction *m_serialToolAction;  // 串口调试助手动作
  QAction *m_networkToolAction; // 网络调试助手动作
  QAction *m_canToolAction;     // CAN调试助手动作
  QAction *m_iapToolAction;     // IAP升级工具动作
  QAction *m_oscilloscopeAction;// 独立多通信接口数字示波器动作

  SerialPortContainer *m_serialPlot; // 串口调试助手窗口
  IAPTool *m_iapTool;                // IAP升级工具窗口
  OscilloscopeWindow *m_oscilloscopeWindow = nullptr; // 独立多通信接口数字示波器窗口

protected:
  void keyPressEvent(QKeyEvent *event) override;

protected:
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  bool event(QEvent *event) override; // 添加这一行
};

#endif // MAINWINDOW_H
