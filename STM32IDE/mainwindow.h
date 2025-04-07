/*
 * @Description:
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-03-26 21:02:58
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-04-07 23:46:29
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QSettings>
#include <QTreeView>
#include <QFileSystemModel>
#include <QTextEdit>
#include <QTabWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QActionGroup>
#include <QKeyEvent>
// 在其他包含之后添加
#include "toolchaindialog.h"
#include "foldableeditor.h"

// 添加QCodeEditor头文件
#include <QCodeEditor>
#include <QCXXHighlighter>
#include <QSyntaxStyle>
#include <QGLSLCompleter>


// 在头文件开始处添加QScintilla相关头文件
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexermakefile.h>
#include <Qsci/qsciapis.h>


#include "codeeditor.h"

// 添加BuildSystem头文件
#include "buildsystem.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    //void newFile();
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
    //void updateVariableList();

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
    void saveCurrentFile(); // 添加保存当前文件的方法声明
    void saveFileAs(); // 添加另存为方法声明
    void executeGdbCommand(); // 添加这一行声明

    // File operations
    void newFile();
    void saveFile();

    void appendBuildOutput(const QString &output);
    void onBuildFinished(bool success);

private:
    void setupUi();
    void createActions();
    void createMenus();
    void createToolbars();
    void createCentralWidget(); // 添加这一行声明
    void loadSettings();
    void saveSettings();
    void executeCommand(const QString &command, const QStringList &arguments);

    // 添加菜单相关的成员变量
    QAction *m_openProjectAction;
    QAction *m_newProjectAction;
    QAction *m_saveProjectAction;
    QAction *m_saveFileAction;
    QAction *m_saveFileAsAction;
    QAction *m_exitAction;

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
    QAction *m_aboutAction;

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
    QComboBox *m_downloaderComboBox;  // 下载工具选择下拉框
    QString m_currentDownloader;      // 当前选择的下载工具
    QString m_currentFilePath;

    // 添加代码编辑器
    QsciScintilla* m_codeEditor2;

    //FoldableEditor *m_codeEditor;

    // // 代码折叠相关方法
    // void setupCodeFolding();
    // void updateCodeFolding(const QRect &rect, int dy);
    // bool isFoldable(int line);
    // void toggleFold(int line);

    // // 代码折叠相关成员变量
    // QMap<int, bool> m_foldedBlocks; // 存储已折叠的代码块 <行号, 是否折叠>

    // 在MainWindow类的private部分添加QScintilla相关成员变量
    QsciScintilla *m_editor;           // QScintilla编辑器

    QsciLexerCPP *m_lexerCPP;          // C++语法高亮器
    QsciLexerMakefile *m_lexerMake;    // Makefile语法高亮器
    QsciAPIs *m_apiCPP;                // C++自动补全API

    // 添加设置编辑器的方法
    void setupEditor();
    //void setupAutoCompletion();
    void loadFile(const QString &filePath);

    CodeEditor *m_codeEditor;

    // 分栏动作
    QAction *m_horizontalSplitAction;
    QAction *m_verticalSplitAction;
    QAction *m_closeSplitAction;

    // 构建系统
    BuildSystem *m_buildSystem;

    // 输出窗口
    QTextEdit *m_outputWindow;

protected:
    void keyPressEvent(QKeyEvent *event) override;
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override; // 添加这一行
};


#endif // MAINWINDOW_H


