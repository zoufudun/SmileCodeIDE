#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMenu>
#include <QMenuBar>
#include <QApplication>
#include <QSplitter>
#include <QGroupBox>
#include <QThread>  // Add this include for QThread
#include <QStatusBar>
#include <QToolBar>
#include <QDebug>
#include <QMenu>
#include <QInputDialog>
#include <QDir>

// #include <QCodeEditor>
// #include <QCXXHighlighter>
// #include <QSyntaxStyle>
// #include <QGLSLCompleter>
#include <QDockWidget>
#include <QStandardItemModel>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QCheckBox>
#include <QSpinBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_isDebugging(false)
{

    //setupEditor();

    setupUi();
    setupThemeMenu(); // 移到createMenus()之前
    createActions();
    createMenus();
    createToolbars();
//    createCentralWidget();
    connect(m_codeEditor, &CodeEditor::newFileRequested, this, &MainWindow::newFile);

    // 创建构建系统
    m_buildSystem = new BuildSystem(this);

    // 创建输出窗口
    m_outputWindow = new QTextEdit(this);
    m_outputWindow->setReadOnly(true);

    // 添加输出窗口到底部
    QDockWidget *outputDock = new QDockWidget("编译输出", this);
    outputDock->setWidget(m_outputWindow);
    addDockWidget(Qt::BottomDockWidgetArea, outputDock);

    // 连接构建系统信号
    connect(m_buildSystem, &BuildSystem::buildOutput, this, &MainWindow::appendBuildOutput);
    connect(m_buildSystem, &BuildSystem::buildFinished, this, &MainWindow::onBuildFinished);
    connect(m_buildSystem, &BuildSystem::projectClosed, this, &MainWindow::onProjectClosed);

    // 连接编辑器信号
    connect(m_codeEditor, &CodeEditor::buildRequested, this, &MainWindow::buildProject);
    connect(m_codeEditor, &CodeEditor::cleanRequested, this, &MainWindow::cleanProject);
    
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::processOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &MainWindow::processError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
            this, &MainWindow::processFinished);
    
    m_settings = new QSettings("STM32IDE", "Settings", this);
    loadSettings();
    
    // 加载上次使用的主题或默认使用深色主题
    QString theme = m_settings->value("theme", "dark").toString();
    applyTheme(theme);
    
    statusBar()->showMessage("就绪");

    // 设置窗口属性
    setWindowTitle("STM32 IDE");
    resize(1200, 800);

    // 确保快捷键能够正常工作
    setFocusPolicy(Qt::StrongFocus);

}

MainWindow::~MainWindow()
{
    saveSettings();
    if (m_process->state() == QProcess::Running) {
        m_process->terminate();
        m_process->waitForFinished(1000);
    }
    // if (m_apiCPP) {
    //     delete m_apiCPP;
    //     m_apiCPP = nullptr;
    // }
}
// void MainWindow::createMenus()
// {
//     // Create main menu bar
//     QMenuBar *menuBar = this->menuBar();

//     // File menu
//     QMenu *fileMenu = menuBar->addMenu("文件");
//     fileMenu->addAction(findChild<QAction*>("新建项目"));
//     fileMenu->addAction(findChild<QAction*>("打开项目"));
//     fileMenu->addAction(findChild<QAction*>("保存项目"));
//     fileMenu->addSeparator();
//     fileMenu->addAction(findChild<QAction*>("保存文件"));
//     fileMenu->addAction(findChild<QAction*>("文件另存为"));
//     fileMenu->addSeparator();
//     fileMenu->addAction(findChild<QAction*>("退出"));

//     fileMenu->addAction(m_newProjectAction);
//     fileMenu->addAction(m_openProjectAction);
//     fileMenu->addAction(m_saveProjectAction);

//     // Build menu
//     QMenu *buildMenu = menuBar->addMenu("构建");
//     buildMenu->addAction(findChild<QAction*>("编译"));
//     buildMenu->addAction(findChild<QAction*>("清理"));
//     buildMenu->addAction(findChild<QAction*>("烧录"));

//     // Debug menu
//     QMenu *debugMenu = menuBar->addMenu("调试");
//     debugMenu->addAction(findChild<QAction*>("开始调试"));
//     debugMenu->addAction(findChild<QAction*>("停止调试"));
//     debugMenu->addSeparator();
//     debugMenu->addAction(findChild<QAction*>("继续"));
//     debugMenu->addAction(findChild<QAction*>("单步跳过"));
//     debugMenu->addAction(findChild<QAction*>("单步进入"));
//     debugMenu->addAction(findChild<QAction*>("单步跳出"));
//     debugMenu->addSeparator();
//     debugMenu->addAction(findChild<QAction*>("设置断点"));

//     // Tools menu
//     QMenu *toolsMenu = menuBar->addMenu("工具");
//     toolsMenu->addAction(findChild<QAction*>("配置工具链"));

//     // View menu
//     QMenu *viewMenu = menuBar->addMenu("视图");
//     viewMenu->addAction(findChild<QAction*>("全屏模式"));

//     // Theme submenu
//     QMenu *themeMenu = viewMenu->addMenu("主题");
//     themeMenu->addAction(m_darkThemeAction);
//     themeMenu->addAction(m_lightThemeAction);
//     themeMenu->addAction(m_oneDarkThemeAction);
//     themeMenu->addAction(m_atomMaterialThemeAction);
//     themeMenu->addAction(m_atomOneThemeAction);
//     themeMenu->addAction(m_gerryThemeAction);
//     themeMenu->addAction(m_materialIconsThemeAction);

//     // Help menu
//     QMenu *helpMenu = menuBar->addMenu("帮助");
//     helpMenu->addAction(findChild<QAction*>("关于"));
// }

void MainWindow::newFile()
{
    // 检查当前文件是否已修改，如果已修改则提示保存
    if (isCurrentFileModified()) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  "保存更改", "当前文件已修改，是否保存更改？",
                                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            saveFile();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    // 创建新文件
    m_codeEditor->setText("");
    m_currentFilePath = "";
    setWindowTitle("STM32IDE - 新文件");
    statusBar()->showMessage("已创建新文件", 2000);
}

bool MainWindow::isCurrentFileModified() const
{
    // Check if the current editor has modifications
    if (m_codeEditor && m_codeEditor->currentEditor()) {
        return m_codeEditor->currentEditor()->isModified();
    }
    return false;
}
void MainWindow::createMenus()
{
    // 创建主菜单栏
    QMenuBar *menuBar = this->menuBar();

    // 文件菜单
    QMenu *fileMenu = menuBar->addMenu("文件");

    fileMenu->addAction(m_newProjectAction);
    fileMenu->addAction(m_openProjectAction);
    fileMenu->addAction(m_saveProjectAction);
    fileMenu->addAction(m_closeProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_saveFileAction);
    fileMenu->addAction(m_saveFileAsAction);
    fileMenu->addAction(m_saveAllAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);



    // 构建菜单
    QMenu *buildMenu = menuBar->addMenu("构建");
    buildMenu->addAction(m_buildAction);
    buildMenu->addAction(m_cleanAction);
    buildMenu->addAction(m_flashAction);

    // 调试菜单
    QMenu *debugMenu = menuBar->addMenu("调试");
    debugMenu->addAction(m_debugAction);
    debugMenu->addAction(m_stopDebugAction);
    debugMenu->addSeparator();
    debugMenu->addAction(m_continueAction);
    debugMenu->addAction(m_stepOverAction);
    debugMenu->addAction(m_stepIntoAction);
    debugMenu->addAction(m_stepOutAction);
    debugMenu->addSeparator();
    debugMenu->addAction(m_breakpointAction);

    // 工具菜单全屏模式
    QMenu *toolsMenu = menuBar->addMenu("工具");
    toolsMenu->addAction(m_configureToolchainAction);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_serialToolAction);     // 添加串口调试助手工具按钮
    toolsMenu->addAction(m_networkToolAction);    // 添加网络调试助手工具按钮

    // 视图菜单
    QMenu *viewMenu = menuBar->addMenu("视图");
    viewMenu->addAction(m_fullScreenAction);

    // 添加分栏子菜单
    QMenu *splitMenu = viewMenu->addMenu("分栏");
    splitMenu->addAction(m_horizontalSplitAction);
    splitMenu->addAction(m_verticalSplitAction);
    splitMenu->addAction(m_closeSplitAction);
    // 主题子菜单
    QMenu *themeMenu = viewMenu->addMenu("主题");

    themeMenu->addSeparator();

    // 添加深色主题组
    QMenu *darkThemesMenu = themeMenu->addMenu("深色主题");
    darkThemesMenu->addAction(m_darkThemeAction);
    darkThemesMenu->addAction(m_oneDarkThemeAction);
    darkThemesMenu->addAction(m_githubDarkThemeAction);
    darkThemesMenu->addAction(m_xcodeDarkThemeAction);
    darkThemesMenu->addAction(m_vueThemeAction);
    darkThemesMenu->addAction(m_monokaiProThemeAction);
    darkThemesMenu->addAction(m_draculaThemeAction);
    darkThemesMenu->addAction(m_nordThemeAction);
    darkThemesMenu->addAction(m_noctisThemeAction);
    darkThemesMenu->addAction(m_nightOwlThemeAction);
    darkThemesMenu->addAction(m_atomMaterialThemeAction);
    darkThemesMenu->addAction(m_atomOneThemeAction);
    darkThemesMenu->addAction(m_gerryThemeAction);
    darkThemesMenu->addAction(m_materialIconsThemeAction);

    // 添加浅色主题组
    QMenu *lightThemesMenu = themeMenu->addMenu("浅色主题");
    lightThemesMenu->addAction(m_lightThemeAction);
    lightThemesMenu->addAction(m_solarizedLightThemeAction);
    lightThemesMenu->addAction(m_materialLightThemeAction);
    // 帮助菜单
    QMenu *helpMenu = menuBar->addMenu("帮助");
    helpMenu->addAction(m_aboutAction);
}

void MainWindow::createToolbars()
{
    // Create main toolbar
    QToolBar *mainToolbar = addToolBar("主工具栏");
    mainToolbar->setMovable(true);

    // Add file actions
    mainToolbar->addAction(findChild<QAction*>("新建项目"));
    mainToolbar->addAction(findChild<QAction*>("打开项目"));
    mainToolbar->addAction(findChild<QAction*>("保存项目"));
    mainToolbar->addAction(findChild<QAction*>("关闭项目"));
    mainToolbar->addAction(findChild<QAction*>("保存文件"));
    mainToolbar->addSeparator();

    // Add build actions
    mainToolbar->addAction(findChild<QAction*>("编译"));
    mainToolbar->addAction(findChild<QAction*>("清理"));
    mainToolbar->addAction(findChild<QAction*>("烧录"));
    mainToolbar->addSeparator();

    // Add debug actions
    mainToolbar->addAction(findChild<QAction*>("开始调试"));
    mainToolbar->addAction(findChild<QAction*>("停止调试"));
    mainToolbar->addAction(findChild<QAction*>("继续"));
    mainToolbar->addAction(findChild<QAction*>("单步跳过"));
    mainToolbar->addAction(findChild<QAction*>("单步进入"));
    mainToolbar->addAction(findChild<QAction*>("单步跳出"));
    mainToolbar->addSeparator();


    // Tools toolbar
    QToolBar *toolsToolbar = addToolBar("调试工具");
    toolsToolbar->setMovable(true);
    toolsToolbar->addAction(m_configureToolchainAction);
    toolsToolbar->addSeparator();
    toolsToolbar->addAction(m_serialToolAction);     // 添加串口调试助手工具按钮
    toolsToolbar->addAction(m_networkToolAction);    // 添加网络调试助手工具按钮

    // Add view actions
    mainToolbar->addAction(findChild<QAction*>("全屏模式"));

    // 添加分栏工具栏
    QToolBar *splitToolbar = addToolBar("分栏工具栏");
    splitToolbar->setMovable(true);
    splitToolbar->addAction(m_horizontalSplitAction);
    splitToolbar->addAction(m_verticalSplitAction);
    splitToolbar->addAction(m_closeSplitAction);
}

// 在目标选择部分之后添加下载工具选择
// void MainWindow::setupUi()
// {
//     setWindowTitle("STM32 编译与调试工具");
//     resize(1024, 768);
    
//     // 创建中央部件
//     QWidget *centralWidget = new QWidget(this);
//     setCentralWidget(centralWidget);
    
//     // 主布局
//     QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
//     // 创建分割器
//     QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
//     mainLayout->addWidget(mainSplitter);
    
//     // 左侧项目树
//     QGroupBox *projectGroup = new QGroupBox("项目文件");
//     QVBoxLayout *projectLayout = new QVBoxLayout(projectGroup);
//     m_projectTreeView = new QTreeView();
//     m_fileSystemModel = new QFileSystemModel();
//     m_projectTreeView->setModel(m_fileSystemModel);
//     m_projectTreeView->setColumnWidth(0, 250);
//     m_projectTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
//     projectLayout->addWidget(m_projectTreeView);
//     mainSplitter->addWidget(projectGroup);
    
//     // 右侧区域
//     QWidget *rightWidget = new QWidget();
//     QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
//     mainSplitter->addWidget(rightWidget);
    
//     // 目标选择
//     QHBoxLayout *targetLayout = new QHBoxLayout();
//     targetLayout->addWidget(new QLabel("目标芯片:"));
//     m_targetComboBox = new QComboBox();
//     m_targetComboBox->addItems({"STM32F103C8", "STM32F407VG", "STM32F429ZI", "STM32L476RG"});
//     targetLayout->addWidget(m_targetComboBox);
//     targetLayout->addStretch();
    
//     // 添加下载工具选择
//     targetLayout->addWidget(new QLabel("下载工具:"));
//     m_downloaderComboBox = new QComboBox();
//     m_downloaderComboBox->addItems({"STLINK", "Jlink"});
//     targetLayout->addWidget(m_downloaderComboBox);
    
//     rightLayout->addLayout(targetLayout);
    
//     // 工具按钮
//     QHBoxLayout *toolLayout = new QHBoxLayout();
//     m_buildButton = new QPushButton("编译");
//     m_cleanButton = new QPushButton("清理");
//     m_flashButton = new QPushButton("烧录");
//     m_debugButton = new QPushButton("调试");
//     m_stopButton = new QPushButton("停止");
//     m_continueButton = new QPushButton("继续");
//     m_stepOverButton = new QPushButton("单步跳过");
//     m_stepIntoButton = new QPushButton("单步进入");
//     m_stepOutButton = new QPushButton("单步跳出");
    
//     toolLayout->addWidget(m_buildButton);
//     toolLayout->addWidget(m_cleanButton);
//     toolLayout->addWidget(m_flashButton);
//     toolLayout->addWidget(m_debugButton);
//     toolLayout->addWidget(m_stopButton);
//     toolLayout->addWidget(m_continueButton);
//     toolLayout->addWidget(m_stepOverButton);
//     toolLayout->addWidget(m_stepIntoButton);
//     toolLayout->addWidget(m_stepOutButton);
    
//     m_stopButton->setEnabled(false);
//     m_continueButton->setEnabled(false);
//     m_stepOverButton->setEnabled(false);
//     m_stepIntoButton->setEnabled(false);
//     m_stepOutButton->setEnabled(false);
    
//     rightLayout->addLayout(toolLayout);
    
//     // 添加代码编辑器和输出选项卡的分割器
//     QSplitter *editorOutputSplitter = new QSplitter(Qt::Vertical);
//     rightLayout->addWidget(editorOutputSplitter);
    
//     // 创建代码编辑器
//     m_codeEditor2 = new QsciScintilla(this);
//     // 设置编辑器字体
//     QFont font("Consolas", 10);
//     font.setFixedPitch(true);
//     m_codeEditor2->setFont(font);
//     m_codeEditor2->setMarginsFont(font);

//     // 设置行号边距
//     m_codeEditor2->setMarginWidth(0, "0000");
//     m_codeEditor2->setMarginLineNumbers(0, true);

//     // 设置折叠边距
//     m_codeEditor2->setMarginWidth(2, 14);
//     m_codeEditor2->setMarginType(2, QsciScintilla::SymbolMargin);
//     m_codeEditor2->setMarginSensitivity(2, true);
//     m_codeEditor2->setFolding(QsciScintilla::BoxedTreeFoldStyle);

//     // 设置C++语法高亮
//     m_lexerCPP = new QsciLexerCPP();
//     m_lexerCPP->setFont(font);
//     m_codeEditor2->setLexer(m_lexerCPP);

//     // 设置自动缩进
//     m_codeEditor2->setAutoIndent(true);
//     m_codeEditor2->setIndentationWidth(4);
//     m_codeEditor2->setTabWidth(4);
//     m_codeEditor2->setTabIndents(true);
//     m_codeEditor2->setBackspaceUnindents(true);

//     // // 设置自动补全
//     // m_codeEditor2->setAutoCompletionSource(QsciScintilla::AcsAll);
//     // m_codeEditor2->setAutoCompletionThreshold(2);
//     // m_codeEditor2->setAutoCompletionCaseSensitivity(false);
//     // m_codeEditor2->setAutoCompletionReplaceWord(true);

//     // // 创建API对象并添加关键字
//     // m_apiCPP = new QsciAPIs(m_lexerCPP);

//     // // 添加C/C++关键字和STM32特定关键字
//     // QStringList keywords;
//     // keywords << "auto" << "break" << "case" << "char" << "const" << "continue" << "default"
//     //          << "do" << "double" << "else" << "enum" << "extern" << "float" << "for"
//     //          << "goto" << "if" << "int" << "long" << "register" << "return" << "short"
//     //          << "signed" << "sizeof" << "static" << "struct" << "switch" << "typedef"
//     //          << "union" << "unsigned" << "void" << "volatile" << "while"
//     //          // STM32特定关键字
//     //          << "uint8_t" << "uint16_t" << "uint32_t" << "int8_t" << "int16_t" << "int32_t"
//     //          << "GPIO_InitTypeDef" << "USART_InitTypeDef" << "TIM_TimeBaseInitTypeDef"
//     //          << "HAL_GPIO_WritePin" << "HAL_GPIO_ReadPin" << "HAL_Delay" << "delay" << "HAL_Delay";

//     // // 添加关键字到API
//     // for (const QString &keyword : keywords) {
//     //     m_apiCPP->add(keyword);
//     // }

//     // // 准备API
//     // m_apiCPP->prepare();

//     setupAutoCompletion();

//     // 设置示例代码
//     m_codeEditor2->setText("// STM32 代码编辑器\n#include <stdint.h>\n\nint main(void) {\n    // 初始化代码\n    while(1) {\n        // 主循环\n    }\n    return 0;\n}");

//     // 添加到分割器
//     editorOutputSplitter->addWidget(m_codeEditor2);
    
//     //createCentralWidget();
    
//     // 输出和调试选项卡
//     m_tabWidget = new QTabWidget();
//     m_outputConsole = new QTextEdit();
//     m_outputConsole->setReadOnly(true);
//     m_outputConsole->setFont(QFont("Consolas", 10));
//     m_debugConsole = new QTextEdit();
//     m_debugConsole->setReadOnly(true);
//     m_debugConsole->setFont(QFont("Consolas", 10));
    
//     m_tabWidget->addTab(m_outputConsole, "编译输出");
//     m_tabWidget->addTab(m_debugConsole, "调试输出");
//     editorOutputSplitter->addWidget(m_tabWidget);
    
//     // 设置分割器比例
//     editorOutputSplitter->setStretchFactor(0, 2);
//     editorOutputSplitter->setStretchFactor(1, 1);
    
//     // 命令行
//     QHBoxLayout *commandLayout = new QHBoxLayout();
//     commandLayout->addWidget(new QLabel("GDB命令:"));
//     m_commandLine = new QLineEdit();
//     m_commandLine->setEnabled(false);
//     commandLayout->addWidget(m_commandLine);
//     rightLayout->addLayout(commandLayout);
    
//     // 连接信号和槽
//     connect(m_buildButton, &QPushButton::clicked, this, &MainWindow::buildProject);
//     connect(m_cleanButton, &QPushButton::clicked, this, &MainWindow::cleanProject);
//     connect(m_flashButton, &QPushButton::clicked, this, &MainWindow::flashProject);
//     connect(m_debugButton, &QPushButton::clicked, this, &MainWindow::startDebug);
//     connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopDebug);
//     connect(m_continueButton, &QPushButton::clicked, this, &MainWindow::continueDebug);
//     connect(m_stepOverButton, &QPushButton::clicked, this, &MainWindow::stepOver);
//     connect(m_stepIntoButton, &QPushButton::clicked, this, &MainWindow::stepInto);
//     connect(m_stepOutButton, &QPushButton::clicked, this, &MainWindow::stepOut);
    
//     // 连接项目树的双击信号，用于打开文件
//     connect(m_projectTreeView, &QTreeView::doubleClicked, this, &MainWindow::openFile);

//     // 连接下载工具选择信号
//     connect(m_downloaderComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &MainWindow::downloaderChanged);

//     // 设置分割器比例
//     mainSplitter->setStretchFactor(0, 1);
//     mainSplitter->setStretchFactor(1, 3);
    
//     // 初始化状态栏
//     // Add this implementation after setupUi() method
//     setupStatusBar();
// }

void MainWindow::setupUi()
{
    setWindowTitle("STM32 编译与调试工具");
    resize(1024, 768);



    // 创建中央部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 创建分割器
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(mainSplitter);

    // 左侧项目树
    QGroupBox *projectGroup = new QGroupBox("项目文件");
    QVBoxLayout *projectLayout = new QVBoxLayout(projectGroup);
    m_projectTreeView = new QTreeView();
    m_fileSystemModel = new QFileSystemModel();
    m_projectTreeView->setModel(m_fileSystemModel);
    m_projectTreeView->setColumnWidth(0, 250);
    m_projectTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    projectLayout->addWidget(m_projectTreeView);
    mainSplitter->addWidget(projectGroup);

    // 右侧区域
    QWidget *rightWidget = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    mainSplitter->addWidget(rightWidget);

    // 添加代码编辑器和输出选项卡的分割器
    QSplitter *editorOutputSplitter = new QSplitter(Qt::Vertical);
    rightLayout->addWidget(editorOutputSplitter);

    // 创建代码编辑器 - 使用新的 CodeEditor 类替换原来的 QsciScintilla
    m_codeEditor = new CodeEditor(this);
    editorOutputSplitter->addWidget(m_codeEditor);

    // 目标选择
    QHBoxLayout *targetLayout = new QHBoxLayout();
    targetLayout->addWidget(new QLabel("目标芯片:"));
    m_targetComboBox = new QComboBox();
    m_targetComboBox->addItems({"STM32F103C8", "STM32F407VG", "STM32F429ZI", "STM32L476RG"});
    targetLayout->addWidget(m_targetComboBox);
    targetLayout->addStretch();

    // 添加下载工具选择
    targetLayout->addWidget(new QLabel("下载工具:"));
    m_downloaderComboBox = new QComboBox();
    m_downloaderComboBox->addItems({"STLINK", "Jlink"});
    targetLayout->addWidget(m_downloaderComboBox);

    rightLayout->addLayout(targetLayout);

    // 工具按钮
    QHBoxLayout *toolLayout = new QHBoxLayout();
    m_buildButton = new QPushButton("编译");
    m_cleanButton = new QPushButton("清理");
    m_flashButton = new QPushButton("烧录");
    m_debugButton = new QPushButton("调试");
    m_stopButton = new QPushButton("停止");
    m_continueButton = new QPushButton("继续");
    m_stepOverButton = new QPushButton("单步跳过");
    m_stepIntoButton = new QPushButton("单步进入");
    m_stepOutButton = new QPushButton("单步跳出");

    toolLayout->addWidget(m_buildButton);
    toolLayout->addWidget(m_cleanButton);
    toolLayout->addWidget(m_flashButton);
    toolLayout->addWidget(m_debugButton);
    toolLayout->addWidget(m_stopButton);
    toolLayout->addWidget(m_continueButton);
    toolLayout->addWidget(m_stepOverButton);
    toolLayout->addWidget(m_stepIntoButton);
    toolLayout->addWidget(m_stepOutButton);

    m_stopButton->setEnabled(false);
    m_continueButton->setEnabled(false);
    m_stepOverButton->setEnabled(false);
    m_stepIntoButton->setEnabled(false);
    m_stepOutButton->setEnabled(false);

    rightLayout->addLayout(toolLayout);



    // 输出和调试选项卡
    m_tabWidget = new QTabWidget();
    m_outputConsole = new QTextEdit();
    m_outputConsole->setReadOnly(true);
    m_outputConsole->setFont(QFont("Consolas", 10));
    m_debugConsole = new QTextEdit();
    m_debugConsole->setReadOnly(true);
    m_debugConsole->setFont(QFont("Consolas", 10));

    m_tabWidget->addTab(m_outputConsole, "编译输出");
    m_tabWidget->addTab(m_debugConsole, "调试输出");
    editorOutputSplitter->addWidget(m_tabWidget);

    // 设置分割器比例
    editorOutputSplitter->setStretchFactor(0, 2);
    editorOutputSplitter->setStretchFactor(1, 1);

    // 命令行
    QHBoxLayout *commandLayout = new QHBoxLayout();
    commandLayout->addWidget(new QLabel("GDB命令:"));
    m_commandLine = new QLineEdit();
    m_commandLine->setEnabled(false);
    commandLayout->addWidget(m_commandLine);
    rightLayout->addLayout(commandLayout);

    // 连接信号和槽
    connect(m_buildButton, &QPushButton::clicked, this, &MainWindow::buildProject);
    connect(m_cleanButton, &QPushButton::clicked, this, &MainWindow::cleanProject);
    connect(m_flashButton, &QPushButton::clicked, this, &MainWindow::flashProject);
    connect(m_debugButton, &QPushButton::clicked, this, &MainWindow::startDebug);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopDebug);
    connect(m_continueButton, &QPushButton::clicked, this, &MainWindow::continueDebug);
    connect(m_stepOverButton, &QPushButton::clicked, this, &MainWindow::stepOver);
    connect(m_stepIntoButton, &QPushButton::clicked, this, &MainWindow::stepInto);
    connect(m_stepOutButton, &QPushButton::clicked, this, &MainWindow::stepOut);

    // 连接项目树的双击信号，用于打开文件
    connect(m_projectTreeView, &QTreeView::doubleClicked, this, &MainWindow::openFile);

    // 连接下载工具选择信号
    connect(m_downloaderComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::downloaderChanged);

    // 设置分割器比例
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 3);

    // 初始化状态栏
    setupStatusBar();
}
// void MainWindow::setupAutoCompletion()
// {
//     // 创建API对象
//     m_apiCPP = new QsciAPIs(m_lexerCPP);

//     // 添加C/C++关键字和STM32相关API
//     QStringList keywords;
//     // C/C++关键字
//     keywords << "auto" << "break" << "case" << "char" << "const" << "continue" << "default"
//              << "do" << "double" << "else" << "enum" << "extern" << "float" << "for"
//              << "goto" << "if" << "int" << "long" << "register" << "return" << "short"
//              << "signed" << "sizeof" << "static" << "struct" << "switch" << "typedef"
//              << "union" << "unsigned" << "void" << "volatile" << "while";

//     // STM32数据类型
//     keywords << "uint8_t" << "uint16_t" << "uint32_t" << "int8_t" << "int16_t" << "int32_t"
//              << "GPIO_InitTypeDef" << "USART_InitTypeDef" << "TIM_TimeBaseInitTypeDef";

//     // STM32特定函数和宏
//     keywords <<"GPIO_Init"<< "GPIO_SetBits"<< "GPIO_ResetBits"<< "GPIO_ReadInputDataBit"
//              <<"TIM_TimeBaseInit"<< "TIM_Cmd"<<"TIM_ITConfig"<< "TIM_GetCounter"
//              <<"USART_Init"<<"USART_Cmd"<< "USART_SendData"<< "USART_ReceiveData"
//              <<"ADC_Init"<< "ADC_Cmd"<< "ADC_StartConversion"<< "ADC_GetConversionValue"
//              <<"RCC_APB1PeriphClockCmd"<< "RCC_APB2PeriphClockCmd"<< "RCC_AHB1PeriphClockCmd"
//              <<"NVIC_Init"<<"NVIC_EnableIRQ"<< "NVIC_DisableIRQ"
//              <<"SysTick_Config"<< "HAL_Delay"<< "HAL_GPIO_WritePin"<< "HAL_GPIO_ReadPin";

//     // STM32 HAL库函数
//     keywords << "HAL_Init()" << "HAL_GPIO_Init()" << "HAL_GPIO_WritePin()" << "HAL_GPIO_ReadPin()"
//              << "HAL_Delay()" << "HAL_UART_Init()" << "HAL_UART_Transmit()" << "HAL_UART_Receive()"
//              << "HAL_TIM_Base_Init()" << "HAL_TIM_Base_Start()" << "HAL_TIM_Base_Stop()";

//     // 添加到API
//     for (const QString &keyword : keywords) {
//         m_apiCPP->add(keyword);
//     }

//     // 准备API
//     m_apiCPP->prepare();

//     // 设置自动补全
//     m_codeEditor2->setAutoCompletionThreshold(2); // 输入2个字符后显示补全
//     m_codeEditor2->setAutoCompletionSource(QsciScintilla::AcsAPIs); // 使用API源
//     m_codeEditor2->setAutoCompletionCaseSensitivity(false); // 不区分大小写
//     m_codeEditor2->setAutoCompletionReplaceWord(true); // 替换当前单词
//     m_codeEditor2->setAutoCompletionUseSingle(QsciScintilla::AcusNever); // 不自动选择唯一匹配项

//     // 连接文本变更信号，以便在编辑时更新变量列表
//     connect(m_codeEditor2, &QsciScintilla::textChanged, this, &MainWindow::updateVariableList);
// }


// // 添加新方法：解析当前文件中的变量定义并更新自动补全列表
// void MainWindow::updateVariableList()
// {
//     // 获取当前文本
//     QString text = m_codeEditor2->text();

//     // 创建一个新的API对象（保留原来的关键字）
//     QsciAPIs* newApi = new QsciAPIs(m_lexerCPP);

//     // 复制原有API中的所有项目到新API
//     for (int i = 0; i < m_apiCPP->apiEntryCount(); ++i) {
//         newApi->add(m_apiCPP->apiEntry(i));
//     }

//     // 使用正则表达式查找变量定义
//     QRegExp varRegex("\\b(int|char|float|double|uint8_t|uint16_t|uint32_t|int8_t|int16_t|int32_t|bool|void|unsigned|long|short|signed|struct|enum|union)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\b");

//     int pos = 0;
//     QSet<QString> variables; // 使用集合避免重复

//     while ((pos = varRegex.indexIn(text, pos)) != -1) {
//         QString varName = varRegex.cap(2);
//         variables.insert(varName);
//         pos += varRegex.matchedLength();
//     }

//     // 查找函数参数
//     QRegExp funcRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^\\)]*)\\)");
//     pos = 0;

//     while ((pos = funcRegex.indexIn(text, pos)) != -1) {
//         QString params = funcRegex.cap(2);
//         QStringList paramList = params.split(',');

//         for (const QString &param : paramList) {
//             // 解析参数定义，例如 "int value" 或 "char* buffer"
//             QRegExp paramRegex("\\b(\\w+)\\s+([\\*&]*)\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\b");
//             if (paramRegex.indexIn(param) != -1) {
//                 QString varName = paramRegex.cap(3);
//                 variables.insert(varName);
//             }
//         }

//         pos += funcRegex.matchedLength();
//     }

//     // 查找for循环中的变量
//     QRegExp forRegex("for\\s*\\(\\s*(?:int|char|float|double|uint8_t|uint16_t|uint32_t)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=");
//     pos = 0;

//     while ((pos = forRegex.indexIn(text, pos)) != -1) {
//         QString varName = forRegex.cap(1);
//         variables.insert(varName);
//         pos += forRegex.matchedLength();
//     }

//     // 将找到的变量添加到API
//     for (const QString &var : variables) {
//         newApi->add(var);
//     }

//     // 准备新API
//     newApi->prepare();

//     // 替换旧的API
//     if (m_apiCPP) {
//         delete m_apiCPP;
//     }
//     m_apiCPP = newApi;

//     // 设置新的API到词法分析器
//     m_lexerCPP->setAPIs(m_apiCPP);
// }


// 添加新方法：解析当前文件中的变量定义并更新自动补全列表
// void MainWindow::updateVariableList()
// {
//     // 获取当前文本
//     QString text = m_codeEditor2->text();

//     // 创建一个新的API对象
//     QsciAPIs* newApi = new QsciAPIs(m_lexerCPP);

//     // 添加C/C++关键字和STM32相关API (重新添加所有标准关键字)
//     QStringList keywords;
//     // C/C++关键字
//     keywords << "auto" << "break" << "case" << "char" << "const" << "continue" << "default"
//              << "do" << "double" << "else" << "enum" << "extern" << "float" << "for"
//              << "goto" << "if" << "int" << "long" << "register" << "return" << "short"
//              << "signed" << "sizeof" << "static" << "struct" << "switch" << "typedef"
//              << "union" << "unsigned" << "void" << "volatile" << "while";

//     // STM32数据类型
//     keywords << "uint8_t" << "uint16_t" << "uint32_t" << "int8_t" << "int16_t" << "int32_t"
//              << "GPIO_InitTypeDef" << "USART_InitTypeDef" << "TIM_TimeBaseInitTypeDef";

//     // STM32特定函数和宏
//     keywords <<"GPIO_Init"<< "GPIO_SetBits"<< "GPIO_ResetBits"<< "GPIO_ReadInputDataBit"
//              <<"TIM_TimeBaseInit"<< "TIM_Cmd"<<"TIM_ITConfig"<< "TIM_GetCounter"
//              <<"USART_Init"<<"USART_Cmd"<< "USART_SendData"<< "USART_ReceiveData"
//              <<"ADC_Init"<< "ADC_Cmd"<< "ADC_StartConversion"<< "ADC_GetConversionValue"
//              <<"RCC_APB1PeriphClockCmd"<< "RCC_APB2PeriphClockCmd"<< "RCC_AHB1PeriphClockCmd"
//              <<"NVIC_Init"<<"NVIC_EnableIRQ"<< "NVIC_DisableIRQ"
//              <<"SysTick_Config"<< "HAL_Delay"<< "HAL_GPIO_WritePin"<< "HAL_GPIO_ReadPin";

//     // STM32 HAL库函数
//     keywords << "HAL_Init()" << "HAL_GPIO_Init()" << "HAL_GPIO_WritePin()" << "HAL_GPIO_ReadPin()"
//              << "HAL_Delay()" << "HAL_UART_Init()" << "HAL_UART_Transmit()" << "HAL_UART_Receive()"
//              << "HAL_TIM_Base_Init()" << "HAL_TIM_Base_Start()" << "HAL_TIM_Base_Stop()";

//     // 添加关键字到新API
//     for (const QString &keyword : keywords) {
//         newApi->add(keyword);
//     }

//     // 使用正则表达式查找变量定义
//     QRegExp varRegex("\\b(int|char|float|double|uint8_t|uint16_t|uint32_t|int8_t|int16_t|int32_t|bool|void|unsigned|long|short|signed|struct|enum|union)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\b");

//     int pos = 0;
//     QSet<QString> variables; // 使用集合避免重复

//     while ((pos = varRegex.indexIn(text, pos)) != -1) {
//         QString varName = varRegex.cap(2);
//         variables.insert(varName);
//         pos += varRegex.matchedLength();
//     }

//     // 查找函数参数
//     QRegExp funcRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^\\)]*)\\)");
//     pos = 0;

//     while ((pos = funcRegex.indexIn(text, pos)) != -1) {
//         QString params = funcRegex.cap(2);
//         QStringList paramList = params.split(',');

//         for (const QString &param : paramList) {
//             // 解析参数定义，例如 "int value" 或 "char* buffer"
//             QRegExp paramRegex("\\b(\\w+)\\s+([\\*&]*)\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\b");
//             if (paramRegex.indexIn(param) != -1) {
//                 QString varName = paramRegex.cap(3);
//                 variables.insert(varName);
//             }
//         }

//         pos += funcRegex.matchedLength();
//     }

//     // 查找for循环中的变量
//     QRegExp forRegex("for\\s*\\(\\s*(?:int|char|float|double|uint8_t|uint16_t|uint32_t)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=");
//     pos = 0;

//     while ((pos = forRegex.indexIn(text, pos)) != -1) {
//         QString varName = forRegex.cap(1);
//         variables.insert(varName);
//         pos += forRegex.matchedLength();
//     }

//     // 将找到的变量添加到API
//     for (const QString &var : variables) {
//         newApi->add(var);
//     }

//     // 准备新API
//     newApi->prepare();

//     // 替换旧的API
//     if (m_apiCPP) {
//         delete m_apiCPP;
//     }
//     m_apiCPP = newApi;

//     // 设置新的API到词法分析器
//     m_lexerCPP->setAPIs(m_apiCPP);
// }


// 添加新方法：解析当前文件中的变量定义并更新自动补全列表

// 主要改进包括：

//     1. 改进了变量定义的正则表达式，使其能够更准确地匹配各种变量定义形式
//     2. 添加了对结构体成员变量的识别
//     3. 增加了调试输出，帮助跟踪变量识别过程
//     4. 添加了对关键字的过滤，避免将关键字误识别为变量
//     5. 将自动补全阈值从2改为1，使补全更加灵敏
//     6. 设置自动补全源为 AcsAll ，确保同时使用API和当前文档中的标识符
//     7. 启用了自动补全填充功能
// void MainWindow::updateVariableList()
// {
//     // 获取当前文本
//     QString text = m_codeEditor2->text();

//     // 创建一个新的API对象
//     QsciAPIs* newApi = new QsciAPIs(m_lexerCPP);

//     // 添加C/C++关键字和STM32相关API (重新添加所有标准关键字)
//     QStringList keywords;
//     // C/C++关键字
//     keywords << "auto" << "break" << "case" << "char" << "const" << "continue" << "default"
//              << "do" << "double" << "else" << "enum" << "extern" << "float" << "for"
//              << "goto" << "if" << "int" << "long" << "register" << "return" << "short"
//              << "signed" << "sizeof" << "static" << "struct" << "switch" << "typedef"
//              << "union" << "unsigned" << "void" << "volatile" << "while";

//     // STM32数据类型
//     keywords << "uint8_t" << "uint16_t" << "uint32_t" << "int8_t" << "int16_t" << "int32_t"
//              << "GPIO_InitTypeDef" << "USART_InitTypeDef" << "TIM_TimeBaseInitTypeDef";

//     // STM32特定函数和宏
//     keywords <<"GPIO_Init"<< "GPIO_SetBits"<< "GPIO_ResetBits"<< "GPIO_ReadInputDataBit"
//              <<"TIM_TimeBaseInit"<< "TIM_Cmd"<<"TIM_ITConfig"<< "TIM_GetCounter"
//              <<"USART_Init"<<"USART_Cmd"<< "USART_SendData"<< "USART_ReceiveData"
//              <<"ADC_Init"<< "ADC_Cmd"<< "ADC_StartConversion"<< "ADC_GetConversionValue"
//              <<"RCC_APB1PeriphClockCmd"<< "RCC_APB2PeriphClockCmd"<< "RCC_AHB1PeriphClockCmd"
//              <<"NVIC_Init"<<"NVIC_EnableIRQ"<< "NVIC_DisableIRQ"
//              <<"SysTick_Config"<< "HAL_Delay"<< "HAL_GPIO_WritePin"<< "HAL_GPIO_ReadPin";

//     // STM32 HAL库函数
//     keywords << "HAL_Init()" << "HAL_GPIO_Init()" << "HAL_GPIO_WritePin()" << "HAL_GPIO_ReadPin()"
//              << "HAL_Delay()" << "HAL_UART_Init()" << "HAL_UART_Transmit()" << "HAL_UART_Receive()"
//              << "HAL_TIM_Base_Init()" << "HAL_TIM_Base_Start()" << "HAL_TIM_Base_Stop()";

//     // 添加关键字到新API
//     for (const QString &keyword : keywords) {
//         newApi->add(keyword);
//     }

//     // 使用正则表达式查找变量定义 - 改进的正则表达式
//     QRegExp varRegex("\\b(int|char|float|double|uint8_t|uint16_t|uint32_t|int8_t|int16_t|int32_t|bool|void|unsigned|long|short|signed|struct|enum|union)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*[;\\[=,)]");

//     int pos = 0;
//     QSet<QString> variables; // 使用集合避免重复

//     while ((pos = varRegex.indexIn(text, pos)) != -1) {
//         QString varName = varRegex.cap(2);
//         if (!varName.isEmpty() && !keywords.contains(varName)) {
//             variables.insert(varName);
//             qDebug() << "找到变量: " << varName;
//         }
//         pos += varRegex.matchedLength();
//     }

//     // 查找函数参数 - 改进的正则表达式
//     QRegExp funcRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^\\)]*)\\)");
//     pos = 0;

//     while ((pos = funcRegex.indexIn(text, pos)) != -1) {
//         QString params = funcRegex.cap(2);
//         QStringList paramList = params.split(',');

//         for (const QString &param : paramList) {
//             // 解析参数定义，例如 "int value" 或 "char* buffer"
//             QRegExp paramRegex("\\b(\\w+)\\s+([\\*&]*)\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\b");
//             if (paramRegex.indexIn(param) != -1) {
//                 QString varName = paramRegex.cap(3);
//                 if (!varName.isEmpty() && !keywords.contains(varName)) {
//                     variables.insert(varName);
//                     qDebug() << "找到参数: " << varName;
//                 }
//             }
//         }

//         pos += funcRegex.matchedLength();
//     }

//     // 查找for循环中的变量 - 改进的正则表达式
//     QRegExp forRegex("for\\s*\\(\\s*(?:int|char|float|double|uint8_t|uint16_t|uint32_t)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=");
//     pos = 0;

//     while ((pos = forRegex.indexIn(text, pos)) != -1) {
//         QString varName = forRegex.cap(1);
//         if (!varName.isEmpty() && !keywords.contains(varName)) {
//             variables.insert(varName);
//             qDebug() << "找到for循环变量: " << varName;
//         }
//         pos += forRegex.matchedLength();
//     }

//     // 查找结构体和类定义中的成员变量
//     QRegExp structRegex("struct\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\{([^}]*)\\}");
//     pos = 0;

//     while ((pos = structRegex.indexIn(text, pos)) != -1) {
//         QString structBody = structRegex.cap(2);
//         QRegExp memberRegex("\\b(\\w+)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*[;\\[]");
//         int memberPos = 0;

//         while ((memberPos = memberRegex.indexIn(structBody, memberPos)) != -1) {
//             QString memberName = memberRegex.cap(2);
//             if (!memberName.isEmpty() && !keywords.contains(memberName)) {
//                 variables.insert(memberName);
//                 qDebug() << "找到结构体成员: " << memberName;
//             }
//             memberPos += memberRegex.matchedLength();
//         }

//         pos += structRegex.matchedLength();
//     }

//     // 将找到的变量添加到API
//     for (const QString &var : variables) {
//         newApi->add(var);
//     }

//     // 准备新API
//     newApi->prepare();

//     // 替换旧的API
//     if (m_apiCPP) {
//         delete m_apiCPP;
//     }
//     m_apiCPP = newApi;

//     // 设置新的API到词法分析器
//     m_lexerCPP->setAPIs(m_apiCPP);

//     // 设置自动补全模式，确保变量补全生效
//     m_codeEditor2->setAutoCompletionSource(QsciScintilla::AcsAll); // 使用所有可用的补全源
// }

// void MainWindow::setupAutoCompletion()
// {
//     // 创建API对象
//     m_apiCPP = new QsciAPIs(m_lexerCPP);

//     // 添加C/C++关键字和STM32相关API
//     QStringList keywords;
//     // C/C++关键字
//     keywords << "auto" << "break" << "case" << "char" << "const" << "continue" << "default"
//              << "do" << "double" << "else" << "enum" << "extern" << "float" << "for"
//              << "goto" << "if" << "int" << "long" << "register" << "return" << "short"
//              << "signed" << "sizeof" << "static" << "struct" << "switch" << "typedef"
//              << "union" << "unsigned" << "void" << "volatile" << "while";

//     // STM32数据类型
//     keywords << "uint8_t" << "uint16_t" << "uint32_t" << "int8_t" << "int16_t" << "int32_t"
//              << "GPIO_InitTypeDef" << "USART_InitTypeDef" << "TIM_TimeBaseInitTypeDef";

//     // STM32特定函数和宏
//     keywords <<"GPIO_Init"<< "GPIO_SetBits"<< "GPIO_ResetBits"<< "GPIO_ReadInputDataBit"
//              <<"TIM_TimeBaseInit"<< "TIM_Cmd"<<"TIM_ITConfig"<< "TIM_GetCounter"
//              <<"USART_Init"<<"USART_Cmd"<< "USART_SendData"<< "USART_ReceiveData"
//              <<"ADC_Init"<< "ADC_Cmd"<< "ADC_StartConversion"<< "ADC_GetConversionValue"
//              <<"RCC_APB1PeriphClockCmd"<< "RCC_APB2PeriphClockCmd"<< "RCC_AHB1PeriphClockCmd"
//              <<"NVIC_Init"<<"NVIC_EnableIRQ"<< "NVIC_DisableIRQ"
//              <<"SysTick_Config"<< "HAL_Delay"<< "HAL_GPIO_WritePin"<< "HAL_GPIO_ReadPin";

//     // STM32 HAL库函数
//     keywords << "HAL_Init()" << "HAL_GPIO_Init()" << "HAL_GPIO_WritePin()" << "HAL_GPIO_ReadPin()"
//              << "HAL_Delay()" << "HAL_UART_Init()" << "HAL_UART_Transmit()" << "HAL_UART_Receive()"
//              << "HAL_TIM_Base_Init()" << "HAL_TIM_Base_Start()" << "HAL_TIM_Base_Stop()";

//     // 添加到API
//     for (const QString &keyword : keywords) {
//         m_apiCPP->add(keyword);
//     }

//     // 准备API
//     m_apiCPP->prepare();

//     // 设置自动补全
//     m_codeEditor2->setAutoCompletionThreshold(1); // 输入1个字符后显示补全
//     m_codeEditor2->setAutoCompletionSource(QsciScintilla::AcsAll); // 使用所有可用的补全源
//     m_codeEditor2->setAutoCompletionCaseSensitivity(false); // 不区分大小写
//     m_codeEditor2->setAutoCompletionReplaceWord(true); // 替换当前单词
//     m_codeEditor2->setAutoCompletionUseSingle(QsciScintilla::AcusNever); // 不自动选择唯一匹配项

//     // 启用自动补全弹出
//     m_codeEditor2->setAutoCompletionFillupsEnabled(true);

//     // 连接文本变更信号，以便在编辑时更新变量列表
//     connect(m_codeEditor2, &QsciScintilla::textChanged, this, &MainWindow::updateVariableList);
// }

void MainWindow::setupStatusBar()
{
    // Create status labels
    m_statusProjectLabel = new QLabel("项目: 无");
    m_statusTargetLabel = new QLabel("目标: " + m_targetComboBox->currentText());
    m_statusBuildLabel = new QLabel("状态: 就绪");

    // Add permanent widgets to status bar
    statusBar()->addPermanentWidget(m_statusProjectLabel);
    statusBar()->addPermanentWidget(m_statusTargetLabel);
    statusBar()->addPermanentWidget(m_statusBuildLabel);

    // Set initial status message
    statusBar()->showMessage("就绪");

    // Update status info
    updateStatusInfo();
}

// // 添加打开文件的方法
// void MainWindow::openFile(const QModelIndex &index)
// {
//     // 获取文件路径
//     QString filePath = m_fileSystemModel->filePath(index);
//     QFileInfo fileInfo(filePath);
    
//     // 只处理文件，不处理目录
//     if (fileInfo.isFile())
//     {
//         // 打开文件
//         QFile file(filePath);
//         if (file.open(QIODevice::ReadOnly | QIODevice::Text))
//         {
//             QTextStream in(&file);
//             QString content = in.readAll();
//             file.close();
            
//             // 在代码编辑器中显示文件内容
//             m_codeEditor2->setText(content);
            
//             // 更新当前文件路径
//             m_currentFilePath = filePath;
            
//             // 更新状态栏显示当前文件名
//             statusBar()->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));
//         }
//         else
//         {
//             QMessageBox::warning(this, tr("Error"), tr("Cannot open file: %1").arg(filePath));
//         }
//     }
// }


// void MainWindow::openFile(const QModelIndex &index)
// {
//     // 获取文件路径
//     QString filePath = m_fileSystemModel->filePath(index);
//     QFileInfo fileInfo(filePath);

//     // 只处理文件，不处理目录
//     if (fileInfo.isFile())
//     {
//         // 打开文件
//         QFile file(filePath);
//         if (file.open(QIODevice::ReadOnly | QIODevice::Text))
//         {
//             QTextStream in(&file);
//             QString content = in.readAll();
//             file.close();

//             // 在代码编辑器中显示文件内容
//             m_codeEditor2->setText(content);

//             // 更新当前文件路径
//             m_currentFilePath = filePath;

//             // 更新状态栏显示当前文件名
//             statusBar()->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));

//             // 解析文件中的变量并更新自动补全
//             updateVariableList();
//         }
//         else
//         {
//             QMessageBox::warning(this, tr("Error"), tr("Cannot open file: %1").arg(filePath));
//         }
//     }
// }


void MainWindow::openFile(const QModelIndex &index)
{
    // 获取文件路径
    QString filePath = m_fileSystemModel->filePath(index);
    QFileInfo fileInfo(filePath);

    // 只处理文件，不处理目录
    if (fileInfo.isFile())
    {
        // 使用 CodeEditor 的 openFile 方法打开文件
        if (m_codeEditor->openFile(filePath))
        {
            // 更新当前文件路径
            m_currentFilePath = filePath;

            // 更新状态栏显示当前文件名
            statusBar()->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));
        }
        else
        {
            QMessageBox::warning(this, tr("Error"), tr("Cannot open file: %1").arg(filePath));
        }
    }
}


// void MainWindow::applyTheme(const QString &themeName)
// {
//     // 保存当前主题名称
//     m_currentTheme = themeName;

//     // 根据主题名称设置对应的选中状态和加载样式表
//     if (themeName == "dark") {
//         m_darkThemeAction->setChecked(true);
//         loadStyleSheet("dark");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #007ACC; color: #FFFFFF;");

//         // 更新代码编辑器样式
//         if (m_editor && m_lexerCPP) {
//             // 设置编辑器背景色和默认文本颜色
//             m_editor->setColor(QColor("#DCDCDC"));
//             m_editor->setPaper(QColor("#1E1E1E"));

//             // 设置行号边距颜色
//             m_editor->setMarginsBackgroundColor(QColor("#1E1E1E"));
//             m_editor->setMarginsForegroundColor(QColor("#858585"));

//             // 设置折叠边距颜色
//             m_editor->setFoldMarginColors(QColor("#1E1E1E"), QColor("#1E1E1E"));

//             // 设置选中文本的颜色
//             m_editor->setSelectionBackgroundColor(QColor("#264F78"));
//             m_editor->setSelectionForegroundColor(QColor("#FFFFFF"));

//             // 设置语法高亮颜色
//             m_lexerCPP->setColor(QColor("#569CD6"), QsciLexerCPP::Keyword); // 关键字
//             m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::DoubleQuotedString); // 字符串
//             m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::SingleQuotedString); // 字符
//             m_lexerCPP->setColor(QColor("#B5CEA8"), QsciLexerCPP::Number); // 数字
//             m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::Comment); // 注释
//             m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::CommentLine); // 行注释
//             m_lexerCPP->setColor(QColor("#C586C0"), QsciLexerCPP::PreProcessor); // 预处理器
//             m_lexerCPP->setColor(QColor("#4EC9B0"), QsciLexerCPP::GlobalClass); // 类名

//             // 设置背景色
//             m_lexerCPP->setPaper(QColor("#1E1E1E"));

//             // 设置默认字体
//             QFont font("Consolas", 10);
//             m_lexerCPP->setFont(font);
//         }
//     } else if (themeName == "light") {
//         // ... existing code ...
//         m_lightThemeAction->setChecked(true);
//         loadStyleSheet("light");
//         statusBar()->setStyleSheet("background-color: #EEEEEE; color: #546E7A;");
//         // 更新代码编辑器样式 (浅色主题)
//         // if (m_codeEditor && m_lexerCPP) {
//         //     // 设置编辑器背景色和默认文本颜色
//         //     m_codeEditor->setColor(QColor("#000000"));
//         //     m_codeEditor->setPaper(QColor("#FFFFFF"));

//         //     // 设置行号边距颜色
//         //     m_codeEditor->setMarginsBackgroundColor(QColor("#F0F0F0"));
//         //     m_codeEditor->setMarginsForegroundColor(QColor("#2B91AF"));

//         //     // 设置折叠边距颜色
//         //     m_codeEditor->setFoldMarginColors(QColor("#F0F0F0"), QColor("#F0F0F0"));

//         //     // 设置选中文本的颜色
//         //     m_codeEditor->setSelectionBackgroundColor(QColor("#ADD6FF"));
//         //     m_codeEditor->setSelectionForegroundColor(QColor("#000000"));

//         //     // 设置语法高亮颜色
//         //     m_lexerCPP->setColor(QColor("#0000FF"), QsciLexerCPP::Keyword); // 关键字
//         //     m_lexerCPP->setColor(QColor("#A31515"), QsciLexerCPP::DoubleQuotedString); // 字符串
//         //     m_lexerCPP->setColor(QColor("#A31515"), QsciLexerCPP::SingleQuotedString); // 字符
//         //     m_lexerCPP->setColor(QColor("#098658"), QsciLexerCPP::Number); // 数字
//         //     m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::Comment); // 注释
//         //     m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::CommentLine); // 行注释
//         //     m_lexerCPP->setColor(QColor("#800000"), QsciLexerCPP::PreProcessor); // 预处理器
//         //     m_lexerCPP->setColor(QColor("#267F99"), QsciLexerCPP::GlobalClass); // 类名

//         //     // 设置背景色
//         //     m_lexerCPP->setPaper(QColor("#FFFFFF"));

//         //     // 设置默认字体
//         //     QFont font("Consolas", 10);
//         //     m_lexerCPP->setFont(font);
//         // }
//     } else if (themeName == "onedark") {
//         m_oneDarkThemeAction->setChecked(true);
//         loadStyleSheet("onedark");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #282C34; color: #ABB2BF;");
//     } else if (themeName == "githubdark") {
//         m_githubDarkThemeAction->setChecked(true);
//         loadStyleSheet("githubdark");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #161B22; color: #C9D1D9;");
//     } else if (themeName == "xcodedark") {
//         m_xcodeDarkThemeAction->setChecked(true);
//         loadStyleSheet("xcodedark");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #2D2D32; color: #FFFFFF;");
//     } else if (themeName == "vue") {
//         m_vueThemeAction->setChecked(true);
//         loadStyleSheet("vue");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #1A1A1A; color: #EEFFFF;");
//     } else if (themeName == "monokaipro") {
//         m_monokaiProThemeAction->setChecked(true);
//         loadStyleSheet("monokaipro");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #221F22; color: #FCFCFA;");
//     } else if (themeName == "dracula") {
//         m_draculaThemeAction->setChecked(true);
//         loadStyleSheet("dracula");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #1E1F29; color: #F8F8F2;");
//     } else if (themeName == "nord") {
//         m_nordThemeAction->setChecked(true);
//         loadStyleSheet("nord");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #2E3440; color: #D8DEE9;");
//     } else if (themeName == "noctis") {
//         m_noctisThemeAction->setChecked(true);
//         loadStyleSheet("noctis");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #1B2932; color: #C2CCDB;");
//     } else if (themeName == "nightowl") {
//         m_nightOwlThemeAction->setChecked(true);
//         loadStyleSheet("nightowl");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #011627; color: #D6DEEB;");
//     } else if (themeName == "solarizedlight") {
//         m_solarizedLightThemeAction->setChecked(true);
//         loadStyleSheet("solarizedlight");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #EEE8D5; color: #657B83;");
//     } else if (themeName == "materiallight") {
//         m_materialLightThemeAction->setChecked(true);
//         loadStyleSheet("materiallight");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #EEEEEE; color: #546E7A;");
//     } else if (themeName == "atommaterial") {
//         m_atomMaterialThemeAction->setChecked(true);
//         loadStyleSheet("atommaterial");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #263238; color: #EEFFFF;");
//     } else if (themeName == "atomone") {
//         m_atomOneThemeAction->setChecked(true);
//         loadStyleSheet("atomone");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #2D2D2D; color: #F8F8F2;");
//     } else if (themeName == "gerry") {
//         m_gerryThemeAction->setChecked(true);
//         loadStyleSheet("gerry");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #1E1E1E; color: #D4D4D4;");
//     } else if (themeName == "materialicons") {
//         m_materialIconsThemeAction->setChecked(true);
//         loadStyleSheet("materialicons");

//         // 更新状态栏颜色
//         statusBar()->setStyleSheet("background-color: #212121; color: #FFFFFF;");
//     }

//     // 保存主题设置到配置文件
//     m_settings->setValue("theme", m_currentTheme);

//     // 更新状态信息
//     if (m_statusProjectLabel && m_statusTargetLabel && m_statusBuildLabel) {
//         updateStatusInfo();
//     }
// }



void MainWindow::applyTheme(const QString &themeName)
{
    // 保存当前主题名称
    m_currentTheme = themeName;

    // 根据主题名称设置对应的选中状态和加载样式表
    if (themeName == "dark") {
        m_darkThemeAction->setChecked(true);
        loadStyleSheet("dark");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #007ACC; color: #FFFFFF;");

        // 更新代码编辑器样式
        if (m_codeEditor && m_lexerCPP) {
            m_codeEditor->applyTheme(themeName);
        }
    } else if (themeName == "light") {
        // ... existing code ...
        m_lightThemeAction->setChecked(true);
        loadStyleSheet("light");
        statusBar()->setStyleSheet("background-color: #EEEEEE; color: #546E7A;");
        // 更新代码编辑器样式 (浅色主题)
        // 更新代码编辑器样式
        if (m_codeEditor && m_lexerCPP) {
            m_codeEditor->applyTheme(themeName);
        }
    } else if (themeName == "onedark") {
        m_oneDarkThemeAction->setChecked(true);
        loadStyleSheet("onedark");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #282C34; color: #ABB2BF;");
    } else if (themeName == "githubdark") {
        m_githubDarkThemeAction->setChecked(true);
        loadStyleSheet("githubdark");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #161B22; color: #C9D1D9;");
    } else if (themeName == "xcodedark") {
        m_xcodeDarkThemeAction->setChecked(true);
        loadStyleSheet("xcodedark");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #2D2D32; color: #FFFFFF;");
    } else if (themeName == "vue") {
        m_vueThemeAction->setChecked(true);
        loadStyleSheet("vue");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #1A1A1A; color: #EEFFFF;");
    } else if (themeName == "monokaipro") {
        m_monokaiProThemeAction->setChecked(true);
        loadStyleSheet("monokaipro");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #221F22; color: #FCFCFA;");
    } else if (themeName == "dracula") {
        m_draculaThemeAction->setChecked(true);
        loadStyleSheet("dracula");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #1E1F29; color: #F8F8F2;");
    } else if (themeName == "nord") {
        m_nordThemeAction->setChecked(true);
        loadStyleSheet("nord");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #2E3440; color: #D8DEE9;");
    } else if (themeName == "noctis") {
        m_noctisThemeAction->setChecked(true);
        loadStyleSheet("noctis");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #1B2932; color: #C2CCDB;");
    } else if (themeName == "nightowl") {
        m_nightOwlThemeAction->setChecked(true);
        loadStyleSheet("nightowl");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #011627; color: #D6DEEB;");
    } else if (themeName == "solarizedlight") {
        m_solarizedLightThemeAction->setChecked(true);
        loadStyleSheet("solarizedlight");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #EEE8D5; color: #657B83;");
    } else if (themeName == "materiallight") {
        m_materialLightThemeAction->setChecked(true);
        loadStyleSheet("materiallight");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #EEEEEE; color: #546E7A;");
    } else if (themeName == "atommaterial") {
        m_atomMaterialThemeAction->setChecked(true);
        loadStyleSheet("atommaterial");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #263238; color: #EEFFFF;");
    } else if (themeName == "atomone") {
        m_atomOneThemeAction->setChecked(true);
        loadStyleSheet("atomone");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #2D2D2D; color: #F8F8F2;");
    } else if (themeName == "gerry") {
        m_gerryThemeAction->setChecked(true);
        loadStyleSheet("gerry");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #1E1E1E; color: #D4D4D4;");
    } else if (themeName == "materialicons") {
        m_materialIconsThemeAction->setChecked(true);
        loadStyleSheet("materialicons");

        // 更新状态栏颜色
        statusBar()->setStyleSheet("background-color: #212121; color: #FFFFFF;");
    }

    // 保存主题设置到配置文件
    m_settings->setValue("theme", m_currentTheme);

    // 更新状态信息
    if (m_statusProjectLabel && m_statusTargetLabel && m_statusBuildLabel) {
        updateStatusInfo();
    }
}

void MainWindow::loadStyleSheet(const QString &sheetName)
{
    // 从资源文件中加载样式表
    QFile file(":/resources/styles/" + sheetName + ".qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        // 读取样式表内容并应用到应用程序
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);
        file.close();
        
        // 输出加载成功信息
        qDebug() << "已加载" << sheetName << "主题样式表";
    } else {
        // 如果加载失败，清除样式表并输出错误信息
        qApp->setStyleSheet("");
        qDebug() << "无法加载" << sheetName << "主题样式表";
    }
}

void MainWindow::openProject()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择项目目录", 
                                                  QDir::homePath(),
                                                  QFileDialog::ShowDirsOnly | 
                                                  QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_projectPath = dir;
        updateProjectTree(dir);
        statusBar()->showMessage("已打开项目: " + dir);
    }
}

void MainWindow::updateProjectTree(const QString &path)
{
    m_fileSystemModel->setRootPath(path);
    m_projectTreeView->setRootIndex(m_fileSystemModel->index(path));
}

// void MainWindow::newProject()
// {
//     // 实现新建项目功能
//     QString dir = QFileDialog::getExistingDirectory(this, "选择项目目录",
//                                                   QDir::homePath(),
//                                                   QFileDialog::ShowDirsOnly |
//                                                   QFileDialog::DontResolveSymlinks);
//     if (dir.isEmpty()) {
//         return;
//     }
    
//     QString projectName = QInputDialog::getText(this, "项目名称", "请输入项目名称:");
//     if (projectName.isEmpty()) {
//         return;
//     }
    
//     QString projectPath = dir + "/" + projectName;
//     QDir projectDir(projectPath);
    
//     if (projectDir.exists()) {
//         QMessageBox::warning(this, "错误", "项目目录已存在!");
//         return;
//     }
    
//     // 创建项目目录结构
//     projectDir.mkpath(".");
//     projectDir.mkpath("src");
//     projectDir.mkpath("inc");
//     projectDir.mkpath("build");
    
//     // 创建基本的Makefile
//     QFile makeFile(projectPath + "/Makefile");
//     if (makeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
//         QTextStream out(&makeFile);
//         out << "# 自动生成的Makefile\n";
//         out << "TARGET = " << projectName << "\n";
//         out << "SRCS = $(wildcard src/*.c)\n";
//         out << "OBJS = $(SRCS:.c=.o)\n";
//         out << "CC = arm-none-eabi-gcc\n";
//         out << "CFLAGS = -mcpu=cortex-m3 -mthumb -Wall -g\n";
//         out << "LDFLAGS = -Wl,-Map=$(TARGET).map -Wl,--gc-sections\n\n";
//         out << "all: $(TARGET).elf\n\n";
//         out << "$(TARGET).elf: $(OBJS)\n";
//         out << "\t$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^\n\n";
//         out << "%.o: %.c\n";
//         out << "\t$(CC) $(CFLAGS) -c -o $@ $<\n\n";
//         out << "clean:\n";
//         out << "\trm -f $(OBJS) $(TARGET).elf $(TARGET).map\n\n";
//         out << "flash:\n";
//         out << "\topenocd -f board/stm32f103c8_blue_pill.cfg -c \"program $(TARGET).elf verify reset exit\"\n\n";
//         out << ".PHONY: all clean flash\n";
//         makeFile.close();
//     }
    
//     // 创建示例源文件
//     QFile srcFile(projectPath + "/src/main.c");
//     if (srcFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
//         QTextStream out(&srcFile);
//         out << "#include <stdint.h>\n\n";
//         out << "int main(void) {\n";
//         out << "    // 初始化代码\n";
//         out << "    while(1) {\n";
//         out << "        // 主循环\n";
//         out << "    }\n";
//         out << "    return 0;\n";
//         out << "}\n";
//         srcFile.close();
//     }
    
//     m_projectPath = projectPath;
//     updateProjectTree(projectPath);
//     statusBar()->showMessage("已创建新项目: " + projectPath);
// }

void MainWindow::newProject()
{
    // 选择项目目录
    QString projectDir = QFileDialog::getExistingDirectory(this, "选择STM32项目目录",
                                                           QDir::homePath(),
                                                           QFileDialog::ShowDirsOnly);
    if (projectDir.isEmpty()) {
        return;
    }

    // 设置项目路径
    m_projectPath = projectDir;
    m_buildSystem->setProjectPath(projectDir);
    m_buildSystem->setOutputPath(projectDir + "/build");

    // 创建标准STM32工程目录结构
    m_buildSystem->generateMakefile();

    // 更新项目树
    updateProjectTree(projectDir);

    // 更新状态栏
    updateStatusInfo();

    // 保存项目设置
    m_settings->setValue("project/path", projectDir);

    statusBar()->showMessage("已创建新STM32项目: " + projectDir, 3000);
}

void MainWindow::saveProject()
{
    // 保存项目配置
    if (m_projectPath.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有打开的项目!");
        return;
    }
    
    // 这里可以保存项目特定的配置
    statusBar()->showMessage("项目已保存");
}

// void MainWindow::buildProject()
// {
//     if (m_projectPath.isEmpty()) {
//         QMessageBox::warning(this, "错误", "没有打开的项目!");
//         return;
//     }
    
//     m_outputConsole->clear();
//     m_outputConsole->append("开始编译项目...\n");
    
//     // 设置工作目录
//     m_process->setWorkingDirectory(m_projectPath);
    
//     // 执行make命令
//     QStringList arguments;
//     executeCommand("make", arguments);
    
//     statusBar()->showMessage("正在编译...");
// }

// 实现构建相关的槽函数
void MainWindow::buildProject()
{
    // 清空输出窗口
    m_outputWindow->clear();
    m_outputWindow->append("开始构建项目...");

    // 设置项目路径
    QString projectPath = QFileInfo(m_currentFilePath).absolutePath();
    m_buildSystem->setProjectPath(projectPath);

    // 设置输出路径
    QString outputPath = projectPath + "/build";
    m_buildSystem->setOutputPath(outputPath);

    // 开始构建
    if (!m_buildSystem->buildProject()) {
        m_outputWindow->append("错误: " + m_buildSystem->lastError());
    }
}

// void MainWindow::cleanProject()
// {
//     if (m_projectPath.isEmpty()) {
//         QMessageBox::warning(this, "错误", "没有打开的项目!");
//         return;
//     }
    
//     m_outputConsole->clear();
//     m_outputConsole->append("清理项目...\n");
    
//     // 设置工作目录
//     m_process->setWorkingDirectory(m_projectPath);
    
//     // 执行make clean命令
//     QStringList arguments;
//     arguments << "clean";
//     executeCommand("make", arguments);
    
//     statusBar()->showMessage("正在清理...");
// }

void MainWindow::cleanProject()
{
    m_outputWindow->clear();
    m_outputWindow->append("清理项目...");

    // 设置输出路径
    QString projectPath = QFileInfo(m_currentFilePath).absolutePath();
    QString outputPath = projectPath + "/build";
    m_buildSystem->setOutputPath(outputPath);

    // 清理项目
    if (m_buildSystem->cleanProject()) {
        m_outputWindow->append("项目清理完成");
    } else {
        m_outputWindow->append("错误: " + m_buildSystem->lastError());
    }
}


void MainWindow::appendBuildOutput(const QString &output)
{
    m_outputWindow->append(output);
}

void MainWindow::onBuildFinished(bool success)
{
    if (success) {
        m_outputWindow->append("构建成功");
    } else {
        m_outputWindow->append("构建失败: " + m_buildSystem->lastError());
    }
}

// void MainWindow::flashProject()
// {
//     if (m_projectPath.isEmpty()) {
//         QMessageBox::warning(this, "错误", "请先打开或创建项目!");
//         return;
//     }
    
//     if (m_openocdPath.isEmpty()) {
//         QMessageBox::warning(this, "错误", "请先配置OpenOCD路径!");
//         return;
//     }
    
//     m_outputConsole->append("开始烧录...");
//     m_tabWidget->setCurrentIndex(0); // 切换到输出选项卡
    
//     QString target = m_targetComboBox->currentText();
//     QString binFile = m_projectPath + "/build/" + QFileInfo(m_projectPath).fileName() + ".bin";
    
//     QStringList arguments;
    
//     // 根据选择的下载工具构建不同的烧录命令
//     if (m_currentDownloader == "STLINK") {
//         // 使用STLINK烧录
//         arguments << "-f" << "board/stm32f4discovery.cfg"
//                  << "-c" << "program " + binFile + " verify reset exit";
        
//         m_outputConsole->append("使用STLINK烧录固件...");
//         executeCommand(m_openocdPath, arguments);
//     } else if (m_currentDownloader == "Jlink") {
//         // 使用Jlink烧录
//         // 创建JLink命令脚本
//         QString scriptPath = m_projectPath + "/jlink_flash.script";
//         QFile scriptFile(scriptPath);
//         if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
//             QTextStream out(&scriptFile);
//             out << "loadbin " << binFile << " 0x08000000\n";
//             out << "verifybin " << binFile << " 0x08000000\n";
//             out << "r\n";
//             out << "g\n";
//             out << "exit\n";
//             scriptFile.close();
            
//             // 构建JLink命令行参数
//             QString jlinkPath = m_settings->value("jlinkPath", "JLink.exe").toString();
//             QStringList jlinkArgs;
            
//             // 根据目标芯片选择正确的设备名称
//             QString deviceName;
//             if (target.startsWith("STM32F103")) {
//                 deviceName = "STM32F103C8";
//             } else if (target.startsWith("STM32F407")) {
//                 deviceName = "STM32F407VG";
//             } else if (target.startsWith("STM32F429")) {
//                 deviceName = "STM32F429ZI";
//             } else if (target.startsWith("STM32L476")) {
//                 deviceName = "STM32L476RG";
//             } else {
//                 deviceName = "STM32F103C8"; // 默认设备
//             }
            
//             jlinkArgs << "-device" << deviceName
//                      << "-if" << "SWD"
//                      << "-speed" << "4000"
//                      << "-CommanderScript" << scriptPath;
            
//             m_outputConsole->append("使用Jlink烧录固件...");
//             executeCommand(jlinkPath, jlinkArgs);
//         } else {
//             QMessageBox::warning(this, "错误", "无法创建JLink命令脚本!");
//         }
//     }
// }

// 实现flashProject方法
void MainWindow::flashProject()
{
    // 确保项目已经构建
    QFile file(m_projectPath + "/build/firmware.bin");
    if (!file.exists()) {
        QMessageBox::warning(this, "烧录失败", "请先构建项目");
        return;
    }

    m_outputWindow->clear();
    m_outputWindow->append("开始烧录...");

    // 根据选择的下载工具执行不同的命令
    QString program;
    QStringList arguments;

    if (m_currentDownloader == "ST-Link") {
        program = m_openocdPath + "/bin/openocd.exe";
        arguments << "-f" << m_openocdConfig
                  << "-c" << "program build/firmware.bin 0x8000000 verify reset exit";
    } else if (m_currentDownloader == "J-Link") {
        program = "JLinkExe";
        arguments << "-device" << m_targetComboBox->currentText()
                  << "-if" << "SWD"
                  << "-speed" << "4000"
                  << "-autoconnect" << "1"
                  << "-CommanderScript" << "flash.jlink";

        // 创建J-Link命令脚本
        QFile jlinkScript(m_projectPath + "/flash.jlink");
        if (jlinkScript.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&jlinkScript);
            out << "loadbin build/firmware.bin, 0x8000000\n";
            out << "verifybin build/firmware.bin, 0x8000000\n";
            out << "r\n";
            out << "q\n";
            jlinkScript.close();
        }
    } else {
        QMessageBox::warning(this, "烧录失败", "不支持的下载工具: " + m_currentDownloader);
        return;
    }

    // 设置工作目录
    m_process->setWorkingDirectory(m_projectPath);

    // 启动烧录进程
    m_process->start(program, arguments);

    if (!m_process->waitForStarted()) {
        m_outputWindow->append("错误: 无法启动烧录进程");
    } else {
        m_outputWindow->append("烧录进程已启动，请等待完成...");
    }
}

void MainWindow::startDebug()
{
    if (m_projectPath.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有打开的项目!");
        return;
    }
    
    m_debugConsole->clear();
    m_debugConsole->append("启动调试会话...\n");
    m_tabWidget->setCurrentIndex(1); // 切换到调试选项卡
    
    // 启用调试相关按钮
    m_stopButton->setEnabled(true);
    m_continueButton->setEnabled(true);
    m_stepOverButton->setEnabled(true);
    m_stepIntoButton->setEnabled(true);
    m_stepOutButton->setEnabled(true);
    m_commandLine->setEnabled(true);
    
    // 禁用其他按钮
    m_buildButton->setEnabled(false);
    m_cleanButton->setEnabled(false);
    m_flashButton->setEnabled(false);
    m_debugButton->setEnabled(false);
    
    m_isDebugging = true;
    
    // 启动OpenOCD
    QProcess *openocdProcess = new QProcess(this);
    openocdProcess->setProcessChannelMode(QProcess::MergedChannels);
    
    QString openocdCmd = m_openocdPath.isEmpty() ? "openocd" : m_openocdPath;
    QStringList openocdArgs;
    openocdArgs << "-f" << (m_openocdConfig.isEmpty() ? "board/stm32f103c8_blue_pill.cfg" : m_openocdConfig);
    
    m_debugConsole->append("启动OpenOCD: " + openocdCmd + " " + openocdArgs.join(" ") + "\n");
    openocdProcess->start(openocdCmd, openocdArgs);
    
    // 等待OpenOCD启动
    QThread::sleep(2);
    
    // 启动GDB
    QString gdbCmd = m_gccPath.isEmpty() ? "arm-none-eabi-gdb" : m_gccPath + "/arm-none-eabi-gdb";
    QStringList gdbArgs;
    gdbArgs << m_projectPath + "/" + QFileInfo(m_projectPath).fileName() + ".elf";
    
    m_debugConsole->append("启动GDB: " + gdbCmd + " " + gdbArgs.join(" ") + "\n");
    
    m_process->start(gdbCmd, gdbArgs);
    
    // 连接到OpenOCD
    QByteArray command = "target remote localhost:3333\n";
    m_process->write(command);
    
    statusBar()->showMessage("调试会话已启动");
}

void MainWindow::stopDebug()
{
    if (!m_isDebugging) {
        return;
    }
    
    // 发送退出命令
    m_process->write("quit\ny\n");
    
    // 禁用调试相关按钮
    m_stopButton->setEnabled(false);
    m_continueButton->setEnabled(false);
    m_stepOverButton->setEnabled(false);
    m_stepIntoButton->setEnabled(false);
    m_stepOutButton->setEnabled(false);
    m_commandLine->setEnabled(false);
    
    // 启用其他按钮
    m_buildButton->setEnabled(true);
    m_cleanButton->setEnabled(true);
    m_flashButton->setEnabled(true);
    m_debugButton->setEnabled(true);
    
    m_isDebugging = false;
    
    statusBar()->showMessage("调试会话已结束");
}

void MainWindow::continueDebug()
{
    if (!m_isDebugging) {
        return;
    }
    
    m_process->write("continue\n");
}

void MainWindow::stepOver()
{
    if (!m_isDebugging) {
        return;
    }
    
    m_process->write("next\n");
}

void MainWindow::stepInto()
{
    if (!m_isDebugging) {
        return;
    }
    
    m_process->write("step\n");
}

void MainWindow::stepOut()
{
    if (!m_isDebugging) {
        return;
    }
    
    m_process->write("finish\n");
}

void MainWindow::setBreakpoint()
{
    // 实现设置断点功能
}

void MainWindow::configureToolchain()
{
        // 创建工具链配置对话框
        ToolchainDialog dialog(this, m_settings);

        // 如果用户点击了确定按钮
        if (dialog.exec() == QDialog::Accepted) {
            // 更新主窗口中的工具链路径
            m_gccPath = dialog.getGccPath();
            m_openocdPath = dialog.getOpenocdPath();
            m_openocdConfig = dialog.getOpenocdConfig();

            // 更新状态栏信息
            statusBar()->showMessage("工具链配置已更新", 2000);

            // 输出日志
            m_outputConsole->append("工具链配置已更新:");
            m_outputConsole->append("GCC 路径: " + m_gccPath);
            m_outputConsole->append("OpenOCD 路径: " + m_openocdPath);
            m_outputConsole->append("OpenOCD 配置: " + m_openocdConfig);
        }

}

 // 实现configureToolchain方法
// void MainWindow::configureToolchain()
// {
//     ToolchainDialog dialog(this);

//     if (dialog.exec() == QDialog::Accepted) {
//         // Get the new configuration using getter methods
//         m_gccPath = dialog.getGccPath();
//         m_openocdPath = dialog.getOpenocdPath();
//         m_openocdConfig = dialog.getOpenocdConfig();

//         // Save configuration
//         m_settings->setValue("toolchain/gcc", m_gccPath);
//         m_settings->setValue("toolchain/openocd", m_openocdPath);
//         m_settings->setValue("toolchain/openocd_config", m_openocdConfig);

//         // Update status bar
//         updateStatusInfo();
//     }
// }

void MainWindow::executeCommand(const QString &command, const QStringList &arguments)
{
    if (m_process->state() == QProcess::Running) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
    
    m_process->start(command, arguments);
}

void MainWindow::processOutput()
{
    QByteArray output = m_process->readAllStandardOutput();
    if (m_isDebugging) {
        m_debugConsole->append(QString::fromUtf8(output));
    } else {
        m_outputConsole->append(QString::fromUtf8(output));
    }
}

void MainWindow::processError()
{
    QByteArray error = m_process->readAllStandardError();
    if (m_isDebugging) {
        m_debugConsole->append(QString::fromUtf8(error));
    } else {
        m_outputConsole->append(QString::fromUtf8(error));
    }
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        if (m_isDebugging) {
            m_debugConsole->append("进程崩溃!\n");
        } else {
            m_outputConsole->append("进程崩溃!\n");
        }
    } else {
        if (m_isDebugging) {
            m_debugConsole->append(QString("进程完成，退出代码: %1\n").arg(exitCode));
        } else {
            m_outputConsole->append(QString("进程完成，退出代码: %1\n").arg(exitCode));
        }
    }
    
    statusBar()->showMessage("命令执行完成");
}

void MainWindow::updateStatusInfo()
{
    // 更新状态栏信息
    if (m_statusProjectLabel) {
        QString projectName = m_projectPath.isEmpty() ? "无" : QFileInfo(m_projectPath).fileName();
        m_statusProjectLabel->setText("项目: " + projectName);
    }
    
    if (m_statusTargetLabel) {
        QString targetName = m_targetComboBox->currentText();
        QString downloaderName = m_downloaderComboBox->currentText();
        m_statusTargetLabel->setText(QString("目标: %1 | 下载工具: %2").arg(targetName).arg(downloaderName));
    }
    
    if (m_statusBuildLabel) {
        QString buildStatus = m_isDebugging ? "调试中" : "就绪";
        m_statusBuildLabel->setText("状态: " + buildStatus);
    }
}

// void MainWindow::toggleFullScreen()
// {
//     // 切换全屏/窗口模式
//     if (isFullScreen()) {
//         showNormal();
//     } else {
//         showFullScreen();
//     }
// }

void MainWindow::toggleFullScreen()
{
    // 切换全屏/窗口模式
    if (isFullScreen()) {
        showNormal();
        m_fullScreenAction->setText("全屏模式");
        m_fullScreenAction->setChecked(false);
        statusBar()->showMessage("已退出全屏模式", 2000);
    } else {
        showFullScreen();
        m_fullScreenAction->setText("退出全屏");
        m_fullScreenAction->setChecked(true);
        statusBar()->showMessage("已进入全屏模式", 2000);
    }
}

void MainWindow::changeTheme(int themeIndex)
{
    // 根据索引切换主题
    switch (themeIndex) {
    case 0: // 深色主题
        applyTheme("dark");
        break;
    case 1: // 浅色主题
        applyTheme("light");
        break;
    default:
        applyTheme("dark"); // 默认使用深色主题
        break;
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(this, "关于STM32IDE",
                      "<h3>STM32 编译与调试工具</h3>"
                      "<p>版本: 1.0</p>"
                      "<p>作者: PhodonZou</p>"
                      "<p>这是一个基于Qt的STM32开发工具，用于编译和调试STM32项目。</p>"
                      "<p>支持ARM GCC工具链和OpenOCD调试器。</p>");
    statusBar()->showMessage("已显示关于信息", 2000);
}

void MainWindow::setDarkTheme()
{
    // Switch to dark theme
    applyTheme("dark");
}

void MainWindow::setLightTheme()
{
    // Switch to light theme
    applyTheme("light");
}

void MainWindow::setOneDarkTheme()
{
    // Switch to One Dark theme
    applyTheme("onedark");
}

void MainWindow::setAtomMaterialTheme()
{
    // 切换到Atom Material主题
    applyTheme("atommaterial");
}

void MainWindow::setAtomOneTheme()
{
    // 切换到Atom One主题
    applyTheme("atomone");
}

void MainWindow::setGerryTheme()
{
    // 切换到Gerry主题
    applyTheme("gerry");
}

void MainWindow::setMaterialIconsTheme()
{
    // 切换到Material Icons主题
    applyTheme("materialicons");
}

// 新增的主题切换方法
void MainWindow::setGithubDarkTheme()
{
    applyTheme("githubdark");
}

void MainWindow::setXcodeDarkTheme()
{
    applyTheme("xcodedark");
}

void MainWindow::setVueTheme()
{
    applyTheme("vue");
}

void MainWindow::setMonokaiProTheme()
{
    applyTheme("monokaipro");
}

void MainWindow::setDraculaTheme()
{
    applyTheme("dracula");
}

void MainWindow::setNordTheme()
{
    applyTheme("nord");
}

void MainWindow::setNoctisTheme()
{
    applyTheme("noctis");
}

void MainWindow::setNightOwlTheme()
{
    applyTheme("nightowl");
}

void MainWindow::setSolarizedLightTheme()
{
    applyTheme("solarizedlight");
}

void MainWindow::setMaterialLightTheme()
{
    applyTheme("materiallight");
}

void MainWindow::downloaderChanged(int index)
{
    // 根据索引更新当前下载工具
    m_currentDownloader = m_downloaderComboBox->itemText(index);
    
    // 更新状态栏信息
    updateStatusInfo();
    
    // 输出日志
    m_outputConsole->append(QString("已选择下载工具: %1").arg(m_currentDownloader));
}

// // 在MainWindow类中添加保存文件的方法
// void MainWindow::saveCurrentFile()
// {
//     // 获取当前打开的文件路径
//     QString currentFilePath = statusBar()->currentMessage();
//     if (currentFilePath.startsWith("已打开文件: ")) {
//         currentFilePath = currentFilePath.mid(7); // 去掉前缀
        
//         QFile file(currentFilePath);
//         if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//             QTextStream out(&file);
//             out << m_codeEditor2->toPlainText();
//             file.close();
            
//             statusBar()->showMessage("已保存文件: " + currentFilePath);
//         } else {
//             QMessageBox::warning(this, "错误", "无法保存文件: " + currentFilePath);
//         }
//     } else {
//         // 如果没有当前文件，则执行另存为
//         saveFileAs();
//     }
// }

// 修改saveCurrentFile方法以使用新的编辑器
void MainWindow::saveCurrentFile()
{
    // 获取当前打开的文件路径
    QString currentFilePath = statusBar()->currentMessage();
    if (currentFilePath.startsWith("已打开文件: ")) {
        currentFilePath = currentFilePath.mid(7); // 去掉前缀

        QFile file(currentFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_editor->text();
            file.close();

            m_editor->setModified(false);
            statusBar()->showMessage("已保存文件: " + currentFilePath);
        } else {
            QMessageBox::warning(this, "错误", "无法保存文件: " + currentFilePath);
        }
    } else {
        // 如果没有当前文件，则执行另存为
        saveFileAs();
    }
}


// void MainWindow::saveFileAs()
// {
//     QString filePath = QFileDialog::getSaveFileName(this, "保存文件",
//                                                   m_projectPath,
//                                                   "C/C++源文件 (*.c *.cpp *.h *.hpp);;汇编文件 (*.s *.asm);;所有文件 (*.*)");
//     if (!filePath.isEmpty()) {
//         QFile file(filePath);
//         if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//             QTextStream out(&file);
//             out << m_codeEditor2->toPlainText();
//             file.close();
            
//             statusBar()->showMessage("已保存文件: " + filePath);
//         } else {
//             QMessageBox::warning(this, "错误", "无法保存文件: " + filePath);
//         }
//     }
// }

void MainWindow::saveSettings()
{
    if (!m_settings)
        return;
    
    m_settings->beginGroup("MainWindow");
    m_settings->setValue("geometry", saveGeometry());
    m_settings->setValue("state", saveState());
    m_settings->setValue("projectPath", m_projectPath);
    m_settings->setValue("currentTheme", m_currentTheme);
    m_settings->setValue("currentDownloader", m_currentDownloader);
    m_settings->endGroup();
    
    m_settings->beginGroup("Toolchain");
    m_settings->setValue("gccPath", m_gccPath);
    m_settings->setValue("openocdPath", m_openocdPath);
    m_settings->setValue("openocdConfig", m_openocdConfig);
    m_settings->endGroup();
    
    m_settings->sync();
}


void MainWindow::setupThemeMenu()
{
    // Create theme actions
    m_darkThemeAction = new QAction("Phudon Dark主题", this);
    m_darkThemeAction->setCheckable(true);
    connect(m_darkThemeAction, &QAction::triggered, this, &MainWindow::setDarkTheme);

    m_lightThemeAction = new QAction("Phudon Light主题", this);
    m_lightThemeAction->setCheckable(true);
    connect(m_lightThemeAction, &QAction::triggered, this, &MainWindow::setLightTheme);

    m_oneDarkThemeAction = new QAction("One Dark主题", this);
    m_oneDarkThemeAction->setCheckable(true);
    connect(m_oneDarkThemeAction, &QAction::triggered, this, &MainWindow::setOneDarkTheme);

    m_githubDarkThemeAction = new QAction("Github Dark主题", this);
    m_githubDarkThemeAction->setCheckable(true);
    connect(m_githubDarkThemeAction, &QAction::triggered, this, &MainWindow::setGithubDarkTheme);

    m_xcodeDarkThemeAction = new QAction("Xcode Dark主题", this);
    m_xcodeDarkThemeAction->setCheckable(true);
    connect(m_xcodeDarkThemeAction, &QAction::triggered, this, &MainWindow::setXcodeDarkTheme);

    m_vueThemeAction = new QAction("Vue主题", this);
    m_vueThemeAction->setCheckable(true);
    connect(m_vueThemeAction, &QAction::triggered, this, &MainWindow::setVueTheme);

    m_monokaiProThemeAction = new QAction("Monokai Pro主题", this);
    m_monokaiProThemeAction->setCheckable(true);
    connect(m_monokaiProThemeAction, &QAction::triggered, this, &MainWindow::setMonokaiProTheme);

    m_draculaThemeAction = new QAction("Dracula主题", this);
    m_draculaThemeAction->setCheckable(true);
    connect(m_draculaThemeAction, &QAction::triggered, this, &MainWindow::setDraculaTheme);

    m_nordThemeAction = new QAction("Nord主题", this);
    m_nordThemeAction->setCheckable(true);
    connect(m_nordThemeAction, &QAction::triggered, this, &MainWindow::setNordTheme);

    m_noctisThemeAction = new QAction("Noctis主题", this);
    m_noctisThemeAction->setCheckable(true);
    connect(m_noctisThemeAction, &QAction::triggered, this, &MainWindow::setNoctisTheme);

    m_nightOwlThemeAction = new QAction("Night Owl主题", this);
    m_nightOwlThemeAction->setCheckable(true);
    connect(m_nightOwlThemeAction, &QAction::triggered, this, &MainWindow::setNightOwlTheme);

    m_solarizedLightThemeAction = new QAction("Solarized Light主题", this);
    m_solarizedLightThemeAction->setCheckable(true);
    connect(m_solarizedLightThemeAction, &QAction::triggered, this, &MainWindow::setSolarizedLightTheme);

    m_materialLightThemeAction = new QAction("Material Light主题", this);
    m_materialLightThemeAction->setCheckable(true);
    connect(m_materialLightThemeAction, &QAction::triggered, this, &MainWindow::setMaterialLightTheme);

    m_atomMaterialThemeAction = new QAction("Atom Material主题", this);
    m_atomMaterialThemeAction->setCheckable(true);
    connect(m_atomMaterialThemeAction, &QAction::triggered, this, &MainWindow::setAtomMaterialTheme);

    m_atomOneThemeAction = new QAction("Atom One主题", this);
    m_atomOneThemeAction->setCheckable(true);
    connect(m_atomOneThemeAction, &QAction::triggered, this, &MainWindow::setAtomOneTheme);

    m_gerryThemeAction = new QAction("Gerry主题", this);
    m_gerryThemeAction->setCheckable(true);
    connect(m_gerryThemeAction, &QAction::triggered, this, &MainWindow::setGerryTheme);

    m_materialIconsThemeAction = new QAction("Material Icons主题", this);
    m_materialIconsThemeAction->setCheckable(true);
    connect(m_materialIconsThemeAction, &QAction::triggered, this, &MainWindow::setMaterialIconsTheme);

    // Create action group to make theme actions exclusive
    // m_themeActionGroup = new QActionGroup(this);
    // m_themeActionGroup->addAction(m_darkThemeAction);
    // m_themeActionGroup->addAction(m_lightThemeAction);
    // m_themeActionGroup->addAction(m_oneDarkThemeAction);
    // m_themeActionGroup->addAction(m_githubDarkThemeAction);
    // m_themeActionGroup->addAction(m_xcodeDarkThemeAction);
    // m_themeActionGroup->addAction(m_vueThemeAction);
    // m_themeActionGroup->addAction(m_monokaiProThemeAction);
    // m_themeActionGroup->addAction(m_draculaThemeAction);
    // m_themeActionGroup->addAction(m_nordThemeAction);
    // m_themeActionGroup->addAction(m_noctisThemeAction);
    // m_themeActionGroup->addAction(m_nightOwlThemeAction);
    // m_themeActionGroup->addAction(m_solarizedLightThemeAction);
    // m_themeActionGroup->addAction(m_materialLightThemeAction);
    // m_themeActionGroup->addAction(m_atomMaterialThemeAction);
    // m_themeActionGroup->addAction(m_atomOneThemeAction);
    // m_themeActionGroup->addAction(m_gerryThemeAction);
    // m_themeActionGroup->addAction(m_materialIconsThemeAction);
}

// void MainWindow::createActions()
// {
//     // File menu actions
//     QAction *openProjectAction = new QAction("打开项目", this);
//     openProjectAction->setShortcut(QKeySequence::Open);
//     openProjectAction->setObjectName("openProjectAction");  // 添加对象名称
//     connect(openProjectAction, &QAction::triggered, this, &MainWindow::openProject);
    
//     QAction *newProjectAction = new QAction("新建项目", this);
//     newProjectAction->setShortcut(QKeySequence::New);
//     connect(newProjectAction, &QAction::triggered, this, &MainWindow::newProject);
    
//     QAction *saveProjectAction = new QAction("保存项目", this);
//     saveProjectAction->setShortcut(QKeySequence::Save);
//     connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);
    
//     QAction *saveFileAction = new QAction("保存文件", this);
//     saveFileAction->setShortcut(QKeySequence("Ctrl+S"));
//     connect(saveFileAction, &QAction::triggered, this, &MainWindow::saveCurrentFile);
    
//     QAction *saveFileAsAction = new QAction("文件另存为", this);
//     saveFileAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
//     connect(saveFileAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    
//     QAction *exitAction = new QAction("退出", this);
//     exitAction->setShortcut(QKeySequence::Quit);
//     connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
//     // Build menu actions
//     QAction *buildAction = new QAction("编译", this);
//     buildAction->setShortcut(QKeySequence("F7"));
//     connect(buildAction, &QAction::triggered, this, &MainWindow::buildProject);
    
//     QAction *cleanAction = new QAction("清理", this);
//     connect(cleanAction, &QAction::triggered, this, &MainWindow::cleanProject);
    
//     QAction *flashAction = new QAction("烧录", this);
//     flashAction->setShortcut(QKeySequence("F8"));
//     connect(flashAction, &QAction::triggered, this, &MainWindow::flashProject);
    
//     // Debug menu actions
//     QAction *debugAction = new QAction("开始调试", this);
//     debugAction->setShortcut(QKeySequence("F5"));
//     connect(debugAction, &QAction::triggered, this, &MainWindow::startDebug);
    
//     QAction *stopDebugAction = new QAction("停止调试", this);
//     stopDebugAction->setShortcut(QKeySequence("Shift+F5"));
//     connect(stopDebugAction, &QAction::triggered, this, &MainWindow::stopDebug);
    
//     QAction *continueAction = new QAction("继续", this);
//     continueAction->setShortcut(QKeySequence("F5"));
//     connect(continueAction, &QAction::triggered, this, &MainWindow::continueDebug);
    
//     QAction *stepOverAction = new QAction("单步跳过", this);
//     stepOverAction->setShortcut(QKeySequence("F10"));
//     connect(stepOverAction, &QAction::triggered, this, &MainWindow::stepOver);
    
//     QAction *stepIntoAction = new QAction("单步进入", this);
//     stepIntoAction->setShortcut(QKeySequence("F11"));
//     connect(stepIntoAction, &QAction::triggered, this, &MainWindow::stepInto);
    
//     QAction *stepOutAction = new QAction("单步跳出", this);
//     stepOutAction->setShortcut(QKeySequence("Shift+F11"));
//     connect(stepOutAction, &QAction::triggered, this, &MainWindow::stepOut);
    
//     QAction *breakpointAction = new QAction("设置断点", this);
//     breakpointAction->setShortcut(QKeySequence("F9"));
//     connect(breakpointAction, &QAction::triggered, this, &MainWindow::setBreakpoint);
    
//     // Tools menu actions
//     QAction *configureToolchainAction = new QAction("配置工具链", this);
//     connect(configureToolchainAction, &QAction::triggered, this, &MainWindow::configureToolchain);
    
//     // View menu actions
//     QAction *fullScreenAction = new QAction("全屏模式", this);
//     fullScreenAction->setShortcut(QKeySequence("F11"));
//     fullScreenAction->setCheckable(true);
//     connect(fullScreenAction, &QAction::triggered, this, &MainWindow::toggleFullScreen);
    
//     // Help menu actions
//     QAction *aboutAction = new QAction("关于", this);
//     connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
// }


void MainWindow::createActions()
{
    // File menu actions
    m_openProjectAction = new QAction("打开项目", this);
    m_openProjectAction->setObjectName("打开项目");
    m_openProjectAction->setIcon(QIcon(":/icons/folder-open-outline.png"));  // 需要添加相应图标
    m_openProjectAction->setStatusTip("打开项目");
    m_openProjectAction->setToolTip("打开项目");
    m_openProjectAction->setShortcut(QKeySequence::Open);
    connect(m_openProjectAction, &QAction::triggered, this, &MainWindow::openProject);

    m_newProjectAction = new QAction("新建项目", this);
    m_newProjectAction->setObjectName("新建项目");
    m_newProjectAction->setIcon(QIcon(":/icons/folder-plus.png"));  // 需要添加相应图标
    m_newProjectAction->setStatusTip("新建项目");
    m_newProjectAction->setToolTip("新建项目");
    m_newProjectAction->setShortcut(QKeySequence::New);
    connect(m_newProjectAction, &QAction::triggered, this, &MainWindow::newProject);

    m_saveProjectAction = new QAction("保存项目", this);
    m_saveProjectAction->setObjectName("保存项目");
    m_saveProjectAction->setIcon(QIcon(":/icons/content-save-check.png"));  // 需要添加相应图标
    m_saveProjectAction->setStatusTip("保存项目");
    m_saveProjectAction->setToolTip("保存项目");
    m_saveProjectAction->setShortcut(QKeySequence::Save);
    connect(m_saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);

    m_closeProjectAction = new QAction("关闭项目", this);
    m_closeProjectAction->setObjectName("关闭项目");
    m_closeProjectAction->setIcon(QIcon(":/icons/close-box.png"));  // 需要添加相应图标
    m_closeProjectAction->setStatusTip("关闭项目");
    m_closeProjectAction->setToolTip("关闭项目");
    m_closeProjectAction->setShortcut(QKeySequence("Ctrl+W"));
    connect(m_closeProjectAction, &QAction::triggered, this, &MainWindow::closeProject);

    m_saveFileAction = new QAction("保存文件", this);
    m_saveFileAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(m_saveFileAction, &QAction::triggered, this, &MainWindow::saveCurrentFile);

    m_saveAllAction = new QAction("全部保存", this);
    m_saveAllAction->setShortcut(QKeySequence("Ctrl+A"));
    connect(m_saveAllAction, &QAction::triggered, this, &MainWindow::saveAllFiles);

    m_saveFileAsAction = new QAction("文件另存为", this);
    m_saveFileAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(m_saveFileAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);

    m_exitAction = new QAction("退出", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    // Build menu actions
    m_buildAction = new QAction("编译", this);
    m_buildAction->setShortcut(QKeySequence("F7"));
    connect(m_buildAction, &QAction::triggered, this, &MainWindow::buildProject);

    m_cleanAction = new QAction("清理", this);
    connect(m_cleanAction, &QAction::triggered, this, &MainWindow::cleanProject);

    m_flashAction = new QAction("烧录", this);
    m_flashAction->setShortcut(QKeySequence("F8"));
    connect(m_flashAction, &QAction::triggered, this, &MainWindow::flashProject);

    // Debug menu actions
    m_debugAction = new QAction("开始调试", this);
    m_debugAction->setShortcut(QKeySequence("F5"));
    connect(m_debugAction, &QAction::triggered, this, &MainWindow::startDebug);

    m_stopDebugAction = new QAction("停止调试", this);
    m_stopDebugAction->setShortcut(QKeySequence("Shift+F5"));
    connect(m_stopDebugAction, &QAction::triggered, this, &MainWindow::stopDebug);

    m_continueAction = new QAction("继续", this);
    m_continueAction->setShortcut(QKeySequence("F5"));
    connect(m_continueAction, &QAction::triggered, this, &MainWindow::continueDebug);

    m_stepOverAction = new QAction("单步跳过", this);
    m_stepOverAction->setShortcut(QKeySequence("F10"));
    connect(m_stepOverAction, &QAction::triggered, this, &MainWindow::stepOver);

    m_stepIntoAction = new QAction("单步进入", this);
    m_stepIntoAction->setShortcut(QKeySequence("F11"));
    connect(m_stepIntoAction, &QAction::triggered, this, &MainWindow::stepInto);

    m_stepOutAction = new QAction("单步跳出", this);
    m_stepOutAction->setShortcut(QKeySequence("Shift+F11"));
    connect(m_stepOutAction, &QAction::triggered, this, &MainWindow::stepOut);

    m_breakpointAction = new QAction("设置断点", this);
    m_breakpointAction->setShortcut(QKeySequence("F9"));
    connect(m_breakpointAction, &QAction::triggered, this, &MainWindow::setBreakpoint);

    // Tools menu actions
    m_configureToolchainAction = new QAction("配置工具链", this);
    m_configureToolchainAction->setIcon(QIcon(":/icons/ToosSetting.png"));  // 需要添加相应图标
    m_configureToolchainAction->setStatusTip("配置编译和调试工具链");
    m_configureToolchainAction->setToolTip("配置编译和调试工具链");
    connect(m_configureToolchainAction, &QAction::triggered, this, &MainWindow::configureToolchain);

    // 添加串口调试助手动作
    m_serialToolAction = new QAction("串口调试助手", this);
    m_serialToolAction->setIcon(QIcon(":/icons/serialport.png"));  // 需要添加相应图标
    m_serialToolAction->setStatusTip("打开串口调试助手");
    connect(m_serialToolAction, &QAction::triggered, this, &MainWindow::openSerialTool);

    // 添加网络调试助手动作
    m_networkToolAction = new QAction("网络调试助手", this);
    m_networkToolAction->setIcon(QIcon(":/icons/network_tool.png"));  // 需要添加相应图标
    m_networkToolAction->setStatusTip("打开网络调试助手");
    m_networkToolAction->setToolTip("打开网络调试助手");
    connect(m_networkToolAction, &QAction::triggered, this, &MainWindow::openNetworkTool);

    // View menu actions
    // m_fullScreenAction = new QAction("全屏模式", this);
    // m_fullScreenAction->setShortcut(QKeySequence("F11"));
    // m_fullScreenAction->setCheckable(true);
    // connect(m_fullScreenAction, &QAction::triggered, this, &MainWindow::toggleFullScreen);

    // m_fullScreenAction = new QAction("全屏模式", this);
    // m_fullScreenAction->setShortcut(QKeySequence("F11"));
    // m_fullScreenAction->setShortcutContext(Qt::ApplicationShortcut); // 确保在整个应用程序范围内有效
    // m_fullScreenAction->setCheckable(true);
    // connect(m_fullScreenAction, &QAction::triggered, this, &MainWindow::toggleFullScreen);

    // 修改 createActions() 方法中的 F11 快捷键设置
    m_fullScreenAction = new QAction("全屏模式", this);
    m_fullScreenAction->setShortcut(QKeySequence("Ctrl+F11")); // 修改为 Ctrl+F11 避免冲突
    m_fullScreenAction->setShortcutContext(Qt::ApplicationShortcut);
    m_fullScreenAction->setCheckable(true);
    connect(m_fullScreenAction, &QAction::triggered, this, &MainWindow::toggleFullScreen);

    // Help menu actions
    m_aboutAction = new QAction("关于", this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    // 添加分栏相关的动作
    m_horizontalSplitAction = new QAction("水平分栏", this);
    m_horizontalSplitAction->setIcon(QIcon(":/icons/horizontal_split.png"));
    m_horizontalSplitAction->setStatusTip("创建水平分栏");
    connect(m_horizontalSplitAction, &QAction::triggered, [this]() {
        m_codeEditor->createSplitView(Qt::Horizontal);
    });

    m_verticalSplitAction = new QAction("垂直分栏", this);
    m_verticalSplitAction->setIcon(QIcon(":/icons/vertical_split.png"));
    m_verticalSplitAction->setStatusTip("创建垂直分栏");
    connect(m_verticalSplitAction, &QAction::triggered, [this]() {
        m_codeEditor->createSplitView(Qt::Vertical);
    });

    m_closeSplitAction = new QAction("关闭分栏", this);
    m_closeSplitAction->setIcon(QIcon(":/icons/close_split.png"));
    m_closeSplitAction->setStatusTip("关闭当前分栏");
    connect(m_closeSplitAction, &QAction::triggered, [this]() {
        m_codeEditor->closeSplitView();
    });
}

void MainWindow::loadSettings()
{
    if (!m_settings)
        return;

    m_settings->beginGroup("MainWindow");
    restoreGeometry(m_settings->value("geometry").toByteArray());
    restoreState(m_settings->value("state").toByteArray());
    m_projectPath = m_settings->value("projectPath").toString();
    m_currentTheme = m_settings->value("currentTheme", "dark").toString();
    m_currentDownloader = m_settings->value("currentDownloader", "STLINK").toString();
    m_settings->endGroup();

    m_settings->beginGroup("Toolchain");
    m_gccPath = m_settings->value("gccPath").toString();
    m_openocdPath = m_settings->value("openocdPath").toString();
    m_openocdConfig = m_settings->value("openocdConfig").toString();
    m_settings->endGroup();

    // If we have a saved project path, update the project tree
    if (!m_projectPath.isEmpty()) {
        updateProjectTree(m_projectPath);
        statusBar()->showMessage("已加载项目: " + m_projectPath);
    }

    // Update the downloader combobox to match the saved setting
    int downloaderIndex = m_downloaderComboBox->findText(m_currentDownloader);
    if (downloaderIndex >= 0) {
        m_downloaderComboBox->setCurrentIndex(downloaderIndex);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 检查是否按下了 Ctrl+F11 组合键
    if (event->key() == Qt::Key_F11 && event->modifiers() == Qt::ControlModifier) {
        toggleFullScreen();
        event->accept();
        qDebug("全屏模式切换");
    } else {
        // 其他键交给父类处理
        QMainWindow::keyPressEvent(event);
    }
}


// // 在文件末尾添加以下方法实现

// void MainWindow::setupCodeFolding()
// {
//     // 扫描文档查找可折叠的代码块
//     QTextDocument *doc = m_codeEditor->document();
//     QTextBlock block = doc->begin();

//     while (block.isValid()) {
//         int lineNumber = block.blockNumber() + 1;
//         if (isFoldable(lineNumber)) {
//             // 在行号区域添加折叠标记
//             // 这里需要自定义 QCodeEditor 类来支持折叠标记的显示
//         }
//         block = block.next();
//     }
// }

// void MainWindow::updateCodeFolding(const QRect &rect, int dy)
// {
//     if (dy != 0) {
//         // 文档垂直滚动时更新折叠标记
//         setupCodeFolding();
//     }
// }

// bool MainWindow::isFoldable(int line)
// {
//     // 判断一行是否可折叠
//     // 例如：检查是否有 { 开始的代码块
//     QTextBlock block = m_codeEditor->document()->findBlockByLineNumber(line - 1);
//     if (!block.isValid())
//         return false;

//     QString text = block.text().trimmed();

//     // 检查是否是可折叠的代码块开始
//     // 例如：函数定义、if/for/while 语句等
//     return (text.contains("{") && !text.contains("}")) ||
//            (text.contains("if") && text.contains("(") && text.contains(")") && !text.contains(";")) ||
//            (text.contains("for") && text.contains("(") && text.contains(")") && !text.contains(";")) ||
//            (text.contains("while") && text.contains("(") && text.contains(")") && !text.contains(";"));
// }

// void MainWindow::toggleFold(int line)
// {
//     // 切换指定行的折叠状态
//     if (m_foldedBlocks.contains(line)) {
//         // 取消折叠
//         m_foldedBlocks.remove(line);
//     } else if (isFoldable(line)) {
//         // 折叠代码块
//         m_foldedBlocks[line] = true;
//     }

//     // 更新编辑器显示
//     m_codeEditor->update();
// }
// Add this implementation somewhere in your mainwindow.cpp file
bool MainWindow::event(QEvent *event)
{
    // Call the base class implementation first
    return QMainWindow::event(event);
}


// Add this implementation somewhere in your mainwindow.cpp file
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // Call the base class implementation first
    QMainWindow::mousePressEvent(event);
}

// Add this implementation somewhere in your mainwindow.cpp file
void MainWindow::resizeEvent(QResizeEvent *event)
{
    // Call the base class implementation first
    QMainWindow::resizeEvent(event);
}




// 在createCentralWidget方法中替换原有的代码编辑器部分
void MainWindow::createCentralWidget()
{
    // 创建中央窗口部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建水平布局
    QHBoxLayout *hLayout = new QHBoxLayout(centralWidget);
    hLayout->setContentsMargins(0, 0, 0, 0);

    // 创建项目树视图
    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setReadOnly(false);
    m_fileSystemModel->setNameFilters(QStringList() << "*.c" << "*.h" << "*.cpp" << "*.hpp" << "*.s" << "*.asm" << "Makefile");
    m_fileSystemModel->setNameFilterDisables(false);

    m_projectTreeView = new QTreeView(this);
    m_projectTreeView->setModel(m_fileSystemModel);
    m_projectTreeView->setRootIndex(m_fileSystemModel->index(QDir::homePath()));
    m_projectTreeView->setAnimated(true);
    m_projectTreeView->setIndentation(20);
    m_projectTreeView->setSortingEnabled(true);
    m_projectTreeView->setColumnWidth(0, 250);
    m_projectTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_projectTreeView, &QTreeView::doubleClicked, this, &MainWindow::onFileDoubleClicked);
    connect(m_projectTreeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showContextMenu);

    // 创建垂直分割器
    QSplitter *vSplitter = new QSplitter(Qt::Vertical);

    // 创建QScintilla编辑器
    // m_editor = new QsciScintilla(this);
    // setupEditor();

    // 创建输出和调试控制台
    m_tabWidget = new QTabWidget(this);
    m_outputConsole = new QTextEdit(this);
    m_outputConsole->setReadOnly(true);
    m_debugConsole = new QTextEdit(this);
    m_debugConsole->setReadOnly(true);

    m_tabWidget->addTab(m_outputConsole, "输出");
    m_tabWidget->addTab(m_debugConsole, "调试");

    // 添加命令行输入
    QWidget *debugControlWidget = new QWidget(this);
    QVBoxLayout *debugLayout = new QVBoxLayout(debugControlWidget);
    debugLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *debugButtonLayout = new QHBoxLayout();
    m_stopButton = new QPushButton("停止", this);
    m_continueButton = new QPushButton("继续", this);
    m_stepOverButton = new QPushButton("单步跳过", this);
    m_stepIntoButton = new QPushButton("单步进入", this);
    m_stepOutButton = new QPushButton("单步跳出", this);

    debugButtonLayout->addWidget(m_stopButton);
    debugButtonLayout->addWidget(m_continueButton);
    debugButtonLayout->addWidget(m_stepOverButton);
    debugButtonLayout->addWidget(m_stepIntoButton);
    debugButtonLayout->addWidget(m_stepOutButton);

    m_commandLine = new QLineEdit(this);
    m_commandLine->setPlaceholderText("输入GDB命令...");

    debugLayout->addLayout(debugButtonLayout);
    debugLayout->addWidget(m_commandLine);
    debugLayout->addWidget(m_debugConsole);

    m_tabWidget->addTab(debugControlWidget, "调试控制");

    // 禁用调试相关按钮，直到开始调试
    m_stopButton->setEnabled(false);
    m_continueButton->setEnabled(false);
    m_stepOverButton->setEnabled(false);
    m_stepIntoButton->setEnabled(false);
    m_stepOutButton->setEnabled(false);
    m_commandLine->setEnabled(false);

    // 连接调试按钮信号
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopDebug);
    connect(m_continueButton, &QPushButton::clicked, this, &MainWindow::continueDebug);
    connect(m_stepOverButton, &QPushButton::clicked, this, &MainWindow::stepOver);
    connect(m_stepIntoButton, &QPushButton::clicked, this, &MainWindow::stepInto);
    connect(m_stepOutButton, &QPushButton::clicked, this, &MainWindow::stepOut);
    connect(m_commandLine, &QLineEdit::returnPressed, this, &MainWindow::executeGdbCommand);

    // 添加组件到垂直分割器
    vSplitter->addWidget(m_editor);
    vSplitter->addWidget(m_tabWidget);
    vSplitter->setStretchFactor(0, 7);
    vSplitter->setStretchFactor(1, 3);

    // 创建水平分割器
    QSplitter *hSplitter = new QSplitter(Qt::Horizontal);
    hSplitter->addWidget(m_projectTreeView);
    hSplitter->addWidget(vSplitter);
    hSplitter->setStretchFactor(0, 1);
    hSplitter->setStretchFactor(1, 4);

    // 添加到主布局
    hLayout->addWidget(hSplitter);

    // 创建工具栏
    createToolbars();
}

// 添加设置编辑器的方法
// void MainWindow::setupEditor()
// {
//     // 基本设置
//     m_editor->setUtf8(true);                          // 使用UTF-8编码
//     m_editor->setMarginLineNumbers(1, true);          // 显示行号
//     m_editor->setMarginWidth(1, "9999");              // 设置行号区域宽度
//     m_editor->setAutoIndent(true);                    // 自动缩进
//     m_editor->setIndentationGuides(true);             // 显示缩进指南
//     m_editor->setTabWidth(4);                         // 设置Tab宽度
//     m_editor->setIndentationsUseTabs(false);          // 使用空格而非Tab
//     m_editor->setBraceMatching(QsciScintilla::SloppyBraceMatch); // 括号匹配
//     m_editor->setCaretLineVisible(true);              // 高亮当前行
//     m_editor->setCaretLineBackgroundColor(QColor("#1F1F1F")); // 当前行背景色
//     m_editor->setMarginsBackgroundColor(QColor("#282828")); // 边距背景色
//     m_editor->setMarginsForegroundColor(QColor("#CCCCCC")); // 边距前景色

//     // 设置代码折叠
//     m_editor->setFolding(QsciScintilla::BoxedTreeFoldStyle, 2); // 设置折叠样式和边距
//     m_editor->setFoldMarginColors(QColor("#282828"), QColor("#282828")); // 折叠区域颜色

//     // 设置自动补全
//     setupAutoCompletion();

//     // 设置C++语法高亮
//     m_lexerCPP = new QsciLexerCPP();
//     m_lexerCPP->setDefaultFont(QFont("Consolas", 10));

//     // 设置语法高亮颜色
//     m_lexerCPP->setColor(QColor("#569CD6"), QsciLexerCPP::Keyword);
//     m_lexerCPP->setColor(QColor("#D69D85"), QsciLexerCPP::DoubleQuotedString);
//     m_lexerCPP->setColor(QColor("#D69D85"), QsciLexerCPP::SingleQuotedString);
//     m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::Comment);
//     m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::CommentLine);
//     m_lexerCPP->setColor(QColor("#9CDCFE"), QsciLexerCPP::Identifier);
//     m_lexerCPP->setColor(QColor("#B5CEA8"), QsciLexerCPP::Number);
//     m_lexerCPP->setColor(QColor("#C586C0"), QsciLexerCPP::Operator);
//     m_lexerCPP->setColor(QColor("#4EC9B0"), QsciLexerCPP::GlobalClass);

//     // 设置编辑器使用C++词法分析器
//     m_editor->setLexer(m_lexerCPP);

//     // 创建Makefile词法分析器
//     m_lexerMake = new QsciLexerMakefile();
//     m_lexerMake->setDefaultFont(QFont("Consolas", 10));

//     // 连接信号
//     connect(m_editor, &QsciScintilla::textChanged, [this]() {
//         // 文件已修改，可以在这里添加标记
//         if (!m_editor->isModified()) {
//             m_editor->setModified(true);
//         }
//     });
// }



void MainWindow::setupEditor()
{
    m_editor = new QsciScintilla(this);
    // 设置编辑器字体
    QFont font("Consolas", 10);
    font.setFixedPitch(true);
    m_editor->setFont(font);
    m_editor->setMarginsFont(font);

    // 设置行号
    m_editor->setMarginType(0, QsciScintilla::NumberMargin);
    m_editor->setMarginWidth(0, 35);
    m_editor->setMarginLineNumbers(0, true);

    // 设置自动缩进
    m_editor->setAutoIndent(true);
    m_editor->setIndentationWidth(4);
    m_editor->setTabWidth(4);
    m_editor->setTabIndents(true);
    m_editor->setBackspaceUnindents(true);

    // 设置括号匹配
    m_editor->setBraceMatching(QsciScintilla::SloppyBraceMatch);

    // 设置自动换行
    m_editor->setWrapMode(QsciScintilla::WrapWord);

    // 设置代码折叠
    m_editor->setFolding(QsciScintilla::BoxedTreeFoldStyle);

    // 设置UTF-8编码
    m_editor->setUtf8(true);

    // 设置C++语法高亮
    m_lexerCPP = new QsciLexerCPP();
    m_lexerCPP->setFont(font);
    m_editor->setLexer(m_lexerCPP);

    // 设置Makefile语法高亮器（备用）
    m_lexerMake = new QsciLexerMakefile();
    m_lexerMake->setFont(font);

    // 设置自动补全
    //setupAutoCompletion();

    // 根据当前主题设置颜色
    if (m_currentTheme == "dark" || m_currentTheme.isEmpty()) {
        // 深色主题
        m_editor->setColor(QColor("#DCDCDC")); // 文本颜色
        m_editor->setPaper(QColor("#1E1E1E")); // 背景颜色
        m_editor->setMarginsForegroundColor(QColor("#AAAAAA")); // 行号前景色
        m_editor->setMarginsBackgroundColor(QColor("#2D2D2D")); // 行号背景色
        m_editor->setCaretForegroundColor(QColor("#FFFFFF")); // 光标颜色
        m_editor->setSelectionBackgroundColor(QColor("#264F78")); // 选择背景色

        // 设置语法高亮颜色
        m_lexerCPP->setColor(QColor("#569CD6"), QsciLexerCPP::Keyword); // 关键字
        m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::DoubleQuotedString); // 字符串
        m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::SingleQuotedString); // 字符
        m_lexerCPP->setColor(QColor("#B5CEA8"), QsciLexerCPP::Number); // 数字
        m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::Comment); // 注释
        m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::CommentLine); // 行注释
        m_lexerCPP->setColor(QColor("#C586C0"), QsciLexerCPP::PreProcessor); // 预处理器
        m_lexerCPP->setColor(QColor("#4EC9B0"), QsciLexerCPP::GlobalClass); // 类名
        m_lexerCPP->setPaper(QColor("#1E1E1E")); // 背景色
    } else {
        // 浅色主题
        m_editor->setColor(QColor("#000000")); // 文本颜色
        m_editor->setPaper(QColor("#FFFFFF")); // 背景颜色
        m_editor->setMarginsForegroundColor(QColor("#333333")); // 行号前景色
        m_editor->setMarginsBackgroundColor(QColor("#F0F0F0")); // 行号背景色
        m_editor->setCaretForegroundColor(QColor("#000000")); // 光标颜色
        m_editor->setSelectionBackgroundColor(QColor("#ADD6FF")); // 选择背景色

        // 设置语法高亮颜色
        m_lexerCPP->setColor(QColor("#0000FF"), QsciLexerCPP::Keyword); // 关键字
        m_lexerCPP->setColor(QColor("#A31515"), QsciLexerCPP::DoubleQuotedString); // 字符串
        m_lexerCPP->setColor(QColor("#A31515"), QsciLexerCPP::SingleQuotedString); // 字符
        m_lexerCPP->setColor(QColor("#098658"), QsciLexerCPP::Number); // 数字
        m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::Comment); // 注释
        m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::CommentLine); // 行注释
        m_lexerCPP->setColor(QColor("#800000"), QsciLexerCPP::PreProcessor); // 预处理器
        m_lexerCPP->setColor(QColor("#267F99"), QsciLexerCPP::GlobalClass); // 类名
        m_lexerCPP->setPaper(QColor("#FFFFFF")); // 背景色
    }
}


// void MainWindow::setupAutoCompletion()
// {
//     // 创建C++自动补全API
//     m_apiCPP = new QsciAPIs(m_lexerCPP);

//     // 添加常用的C/C++关键字和函数
//     QStringList keywords = {
//         "int", "char", "void", "float", "double", "long", "short", "unsigned", "signed",
//         "struct", "union", "enum", "typedef", "const", "volatile", "static", "extern",
//         "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue",
//         "return", "goto", "sizeof", "NULL", "true", "false",

//         // STM32特定函数和宏
//         "GPIO_Init", "GPIO_SetBits", "GPIO_ResetBits", "GPIO_ReadInputDataBit",
//         "TIM_TimeBaseInit", "TIM_Cmd", "TIM_ITConfig", "TIM_GetCounter",
//         "USART_Init", "USART_Cmd", "USART_SendData", "USART_ReceiveData",
//         "ADC_Init", "ADC_Cmd", "ADC_StartConversion", "ADC_GetConversionValue",
//         "RCC_APB1PeriphClockCmd", "RCC_APB2PeriphClockCmd", "RCC_AHB1PeriphClockCmd",
//         "NVIC_Init", "NVIC_EnableIRQ", "NVIC_DisableIRQ",
//         "SysTick_Config", "HAL_Delay", "HAL_GPIO_WritePin", "HAL_GPIO_ReadPin"
//     };

//     // 添加关键字到API
//     for (const QString &keyword : keywords) {
//         m_apiCPP->add(keyword);
//     }

//     // 准备API
//     m_apiCPP->prepare();

//     // 设置自动补全
//     m_editor->setAutoCompletionSource(QsciScintilla::AcsAll);  // 所有来源
//     m_editor->setAutoCompletionThreshold(2);                   // 输入2个字符后显示
//     m_editor->setAutoCompletionCaseSensitivity(false);         // 不区分大小写
//     m_editor->setAutoCompletionReplaceWord(true);              // 替换整个单词
//     m_editor->setAutoCompletionUseSingle(QsciScintilla::AcusNever); // 不自动选择唯一项
// }



// // 添加加载文件的方法
// void MainWindow::loadFile(const QString &filePath)
// {
//     QFile file(filePath);
//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         QMessageBox::warning(this, "错误", "无法打开文件: " + filePath);
//         return;
//     }

//     // 读取文件内容
//     QTextStream in(&file);
//     QString content = in.readAll();
//     file.close();

//     // 设置编辑器内容
//     m_editor->setText(content);
//     m_editor->setModified(false);

//     // 根据文件类型设置词法分析器
//     if (filePath.endsWith(".c") || filePath.endsWith(".cpp") ||
//         filePath.endsWith(".h") || filePath.endsWith(".hpp")) {
//         m_editor->setLexer(m_lexerCPP);
//     } else if (filePath.endsWith("Makefile") || filePath.contains("makefile")) {
//         m_editor->setLexer(m_lexerMake);
//     }

//     // 更新状态栏
//     statusBar()->showMessage("已打开文件: " + filePath);
// }


void MainWindow::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法打开文件: %1").arg(filePath));
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // 根据文件扩展名选择合适的语法高亮器
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    if (extension == "c" || extension == "cpp" || extension == "h" || extension == "hpp") {
        m_editor->setLexer(m_lexerCPP);
    } else if (extension == "mk" || fileInfo.fileName().toLower() == "makefile") {
        m_editor->setLexer(m_lexerMake);
    } else {
        // 对于其他类型的文件，清除语法高亮
        m_editor->setLexer(nullptr);
    }

    // 设置文件内容
    m_editor->setText(content);

    // 更新当前文件路径
    m_currentFilePath = filePath;

    // 更新状态栏
    statusBar()->showMessage(tr("已打开: %1").arg(fileInfo.fileName()));
}


// 修改onFileDoubleClicked方法以使用新的编辑器
void MainWindow::onFileDoubleClicked(const QModelIndex &index)
{
    QString filePath = m_fileSystemModel->filePath(index);
    QFileInfo fileInfo(filePath);

    if (fileInfo.isFile()) {
        loadFile(filePath);
    }
}

void MainWindow::saveFile()
{
    // Check if we have a current file path
    if (m_currentFilePath.isEmpty()) {
        // If no current file path, call saveFileAs instead
        saveFileAs();
        return;
    }

    // Save the file using the CodeEditor's saveFile method
    if (m_codeEditor->saveFile(m_currentFilePath)) {
        statusBar()->showMessage("File saved: " + m_currentFilePath, 2000);
        setWindowTitle("STM32IDE - " + QFileInfo(m_currentFilePath).fileName());
    } else {
        QMessageBox::warning(this, "Save Failed", "Failed to save file: " + m_currentFilePath);
    }
}



// 修改saveFileAs方法以使用新的编辑器
void MainWindow::saveFileAs()
{
    QString filePath = QFileDialog::getSaveFileName(this, "保存文件",
                                                    m_projectPath,
                                                    "C/C++源文件 (*.c *.cpp *.h *.hpp);;汇编文件 (*.s *.asm);;所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_editor->text();
            file.close();

            m_editor->setModified(false);
            statusBar()->showMessage("已保存文件: " + filePath);
        } else {
            QMessageBox::warning(this, "错误", "无法保存文件: " + filePath);
        }
    }
}

// 添加执行GDB命令的方法
void MainWindow::executeGdbCommand()
{
    if (!m_isDebugging || !m_process) {
        return;
    }

    QString command = m_commandLine->text();
    if (!command.isEmpty()) {
        m_debugConsole->append("> " + command);
        m_process->write((command + "\n").toUtf8());
        m_commandLine->clear();
    }
}


void MainWindow::showContextMenu(const QPoint &pos)
{
    // Get the index at the position
    QModelIndex index = m_projectTreeView->indexAt(pos);
    
    if (index.isValid()) {
        // Create a context menu
        QMenu contextMenu(this);
        
        // Get the file path
        QString filePath = m_fileSystemModel->filePath(index);
        QFileInfo fileInfo(filePath);
        
        // Add actions based on file type
        if (fileInfo.isFile()) {
            // File actions
            QAction *openAction = contextMenu.addAction("打开");
            connect(openAction, &QAction::triggered, [this, index]() {
                onFileDoubleClicked(index);
            });
            
            contextMenu.addSeparator();
            
            QAction *renameAction = contextMenu.addAction("重命名");
            connect(renameAction, &QAction::triggered, [this, index]() {
                m_projectTreeView->edit(index);
            });
            
            QAction *deleteAction = contextMenu.addAction("删除");
            connect(deleteAction, &QAction::triggered, [this, index]() {
                QString filePath = m_fileSystemModel->filePath(index);
                QFile::remove(filePath);
            });
        } else if (fileInfo.isDir()) {
            // Directory actions
            QAction *newFileAction = contextMenu.addAction("新建文件");
            connect(newFileAction, &QAction::triggered, [this, filePath]() {
                QString fileName = QInputDialog::getText(this, "新建文件", "文件名:");
                if (!fileName.isEmpty()) {
                    QString newFilePath = filePath + "/" + fileName;
                    QFile file(newFilePath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.close();
                    }
                }
            });
            
            QAction *newFolderAction = contextMenu.addAction("新建文件夹");
            connect(newFolderAction, &QAction::triggered, [this, filePath]() {
                QString folderName = QInputDialog::getText(this, "新建文件夹", "文件夹名:");
                if (!folderName.isEmpty()) {
                    QDir dir(filePath);
                    dir.mkdir(folderName);
                }
            });
            
            contextMenu.addSeparator();
            
            QAction *renameAction = contextMenu.addAction("重命名");
            connect(renameAction, &QAction::triggered, [this, index]() {
                m_projectTreeView->edit(index);
            });
            
            QAction *deleteAction = contextMenu.addAction("删除");
            connect(deleteAction, &QAction::triggered, [this, index]() {
                QString dirPath = m_fileSystemModel->filePath(index);
                QDir dir(dirPath);
                dir.removeRecursively();
            });
        }
        
        // Show the context menu
        contextMenu.exec(m_projectTreeView->viewport()->mapToGlobal(pos));
    }
}

// // 添加关闭工程的方法
// void MainWindow::closeProject()
// {
//     // 防止误关闭整个应用程序
//     if (sender() == m_exitAction) {
//         qDebug() << "Exit action triggered, not closing project";
//         return;
//     }
//     qDebug() << "closeProject method called";

//     // // 检查是否有未保存的文件
//     // if (hasUnsavedChanges()) {
//     //     QMessageBox::StandardButton reply = QMessageBox::question(this,
//     //                                                               "关闭工程",
//     //                                                               "有未保存的更改，是否保存？",
//     //                                                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

//     //     if (reply == QMessageBox::Cancel) {
//     //         qDebug() << "User canceled project closing";
//     //         return;
//     //     } else if (reply == QMessageBox::Yes) {
//     //         saveAllFiles();
//     //     }
//     // }

//     // 关闭所有打开的编辑器标签页，但不关闭主编辑器
//     closeAllEditors();

//     // // 清空主编辑器内容
//     // if (m_editor) {
//     //     m_editor->clear();
//     //     m_editor->setModified(false);
//     // }


//     // 安全地清空主编辑器内容
//     if (m_editor) {
//         // 断开信号连接，防止清空时触发不必要的信号
//         m_editor->blockSignals(true);
//         m_editor->clear();
//         m_editor->setModified(false);
//         m_editor->blockSignals(false);
//     }

//     // // 关闭工程
//     // if (m_buildSystem) {
//     //     m_buildSystem->closeProject();
//     // }

//     // 清空项目路径
//     m_projectPath.clear();
//     m_currentFilePath.clear();

//     // 更新UI状态
//     updateWindowTitle();
//     updateStatusInfo();

//     // 清空项目树
//     clearProjectTree();

//     // 禁用工程相关的菜单项和工具栏按钮
//     //updateMenuState();

//     // 清空输出控制台
//     if (m_outputConsole) {
//         m_outputConsole->clear();
//     }

//     // 清空调试控制台
//     if (m_debugConsole) {
//         m_debugConsole->clear();
//     }

//     statusBar()->showMessage("工程已关闭", 3000);
//     qDebug() << "Project closed successfully";
// }


void MainWindow::closeProject()
{
    // 防止误关闭整个应用程序
    if (sender() == m_exitAction) {
        qDebug() << "Exit action triggered, not closing project";
        return;
    }
    qDebug() << "closeProject method called";
    // 检查是否有未保存的文件
    // if (hasUnsavedChanges()) {
    //     QMessageBox::StandardButton reply = QMessageBox::question(this,
    //                                                               "关闭工程",
    //                                                               "有未保存的更改，是否保存？",
    //                                                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    //     if (reply == QMessageBox::Cancel) {
    //         qDebug() << "User canceled project closing";
    //         return;
    //     } else if (reply == QMessageBox::Yes) {
    //         saveAllFiles();
    //     }
    // }
    try {
        //关闭所有打开的编辑器标签页，但不关闭主编辑器
        if (m_tabWidget) {
            m_tabWidget->blockSignals(true);
            while (m_tabWidget->count() > 0) {
                m_tabWidget->removeTab(0);
            }
            m_tabWidget->blockSignals(false);
        }

        // 清空项目路径
        m_projectPath.clear();
        m_currentFilePath.clear();

        // 更新UI状态
        //updateWindowTitle();
        updateStatusInfo();
        updateWindowTitle();

        // 清空项目树
        clearProjectTree();
        //禁用工程相关的菜单项和工具栏按钮
        //updateMenuState();
        // 清空输出控制台
        if (m_outputConsole) {
            m_outputConsole->blockSignals(true);
            m_outputConsole->clear();
            m_outputConsole->blockSignals(false);
        }

        // 清空调试控制台
        if (m_debugConsole) {
            m_debugConsole->blockSignals(true);
            m_debugConsole->clear();
            m_debugConsole->blockSignals(false);
        }

        statusBar()->showMessage("工程已关闭", 3000);
        qDebug() << "Project closed successfully";
    } catch (const std::exception& e) {
        qDebug() << "Exception in closeProject: " << e.what();
    } catch (...) {
        qDebug() << "Unknown exception in closeProject";
    }
}

// 添加处理工程关闭信号的槽
void MainWindow::onProjectClosed()
{
    // 更新UI状态
    updateWindowTitle();
    updateStatusInfo();

    // 禁用工程相关的菜单项和工具栏按钮
    updateMenuState();
}

// 添加检查未保存更改的方法
bool MainWindow::hasUnsavedChanges()
{
    // 遍历所有打开的编辑器，检查是否有未保存的更改
    // 这里需要根据你的编辑器实现来编写具体代码
    // 简单示例：

    // for (auto editor : m_openEditors) {
    //     if (editor->isModified()) {
    //         return true;
    //     }
    // }
    // Check if any editor has unsaved changes
    if (m_editor && m_editor->isModified()) {
        return true;
    }

    // If using a tab widget to manage editors
    if (m_tabWidget) {
        for (int i = 0; i < m_tabWidget->count(); i++) {
            QWidget* widget = m_tabWidget->widget(i);
            QsciScintilla* editor = qobject_cast<QsciScintilla*>(widget);
            if (editor && editor->isModified()) {
                return true;
            }
        }
    }

    return false;
}

// 添加保存所有文件的方法
void MainWindow::saveAllFiles()
{
    // 遍历所有打开的编辑器，保存文件
    // 这里需要根据你的编辑器实现来编写具体代码
    // 简单示例：
    // for (auto editor : m_openEditors) {
    //     if (editor->isModified()) {
    //         editor->save();
    //     }
    // }

    // Save individual editors
    if (m_editor && m_editor->isModified()) {
        // Get the file path for this editor (you need to track this)
        QString filePath = m_currentFilePath; // or however you track the file path
        saveEditorContent(m_editor, filePath);
    }

    // If using a tab widget to manage editors
    if (m_tabWidget) {
        for (int i = 0; i < m_tabWidget->count(); i++) {
            QWidget* widget = m_tabWidget->widget(i);
            QsciScintilla* editor = qobject_cast<QsciScintilla*>(widget);
            if (editor && editor->isModified()) {
                // Get the file path for this editor (you need to track this)
                // This might be stored in tab data or elsewhere
                QString filePath = m_tabWidget->tabToolTip(i); // if you store path in tooltip
                saveEditorContent(editor, filePath);
            }
        }
    }
}

// 添加关闭所有编辑器的方法
void MainWindow::closeAllEditors()
{
    // 关闭所有打开的编辑器
    // 这里需要根据你的编辑器实现来编写具体代码
    // 简单示例：
    // while (!m_openEditors.isEmpty()) {
    //     auto editor = m_openEditors.takeFirst();
    //     delete editor;
    // }

    // 如果使用QTabWidget管理编辑器，可以这样清空：
    if (m_tabWidget) {
        while (m_tabWidget->count() > 0) {
            m_tabWidget->removeTab(0);
        }
    }
}



// 添加更新菜单状态的方法
void MainWindow::updateMenuState()
{
    // 根据是否有打开的工程来启用或禁用菜单项
    bool hasProject = !m_projectPath.isEmpty();

    // 更新文件菜单
    if (m_saveFileAction) m_saveFileAction->setEnabled(hasProject);  // 修正变量名
    if (m_saveAllAction) m_saveAllAction->setEnabled(hasProject);
    if (m_closeProjectAction) m_closeProjectAction->setEnabled(hasProject);

    // 更新构建菜单
    if (m_buildAction) m_buildAction->setEnabled(hasProject);
    if (m_cleanAction) m_cleanAction->setEnabled(hasProject);
    if (m_flashAction) m_flashAction->setEnabled(hasProject);

    // 更新工具栏按钮
    if (m_buildButton) m_buildButton->setEnabled(hasProject);
    if (m_cleanButton) m_cleanButton->setEnabled(hasProject);
    if (m_flashButton) m_flashButton->setEnabled(hasProject);
}


void MainWindow::updateWindowTitle()
{
    if (m_projectPath.isEmpty()) {
        setWindowTitle("STM32 IDE");
    } else {
        QFileInfo fileInfo(m_projectPath);
        QString projectName = fileInfo.fileName();
        setWindowTitle(QString("STM32 IDE - %1").arg(projectName));
    }
}


// Add this helper method to save a QsciScintilla editor's content to a file
bool MainWindow::saveEditorContent(QsciScintilla* editor, const QString& filePath)
{
    if (!editor || filePath.isEmpty()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "保存失败", "无法保存文件: " + filePath);
        return false;
    }

    QTextStream out(&file);
    out << editor->text();
    file.close();

    editor->setModified(false);
    statusBar()->showMessage("已保存文件: " + filePath, 2000);
    return true;
}

// Modify the clearProjectTree method
void MainWindow::clearProjectTree()
{
    // Since you're using QFileSystemModel, not QStandardItemModel,
    // we need to handle clearing differently
    // if (m_projectTreeView) {
    //     // Set the root path to the home directory or some default location
    //     m_projectTreeView->setRootIndex(m_fileSystemModel->index(QDir::homePath()));

    //     // Or if you want to completely reset the model:
    //     // m_fileSystemModel->setRootPath("");
    //     // m_projectTreeView->setRootIndex(m_fileSystemModel->index(""));
    // }

    if (m_projectTreeView && m_fileSystemModel) {
        //m_projectTreeView->setRootIndex(m_fileSystemModel->index(QDir::homePath()));
        m_fileSystemModel->setRootPath("");
        m_projectTreeView->setRootIndex(m_fileSystemModel->index(""));
    }
}


// // 添加清空项目树的方法
// void MainWindow::clearProjectTree()
// {
//     // 清空项目树视图
//     // 这里需要根据你的项目树实现来编写具体代码
//     // 简单示例：
//     if (m_projectTreeView) {
//         QStandardItemModel* model = qobject_cast<QStandardItemModel*>(m_projectTreeView->model());
//         if (model) {
//             model->clear();
//         }
//     }
// }



// 添加在MainWindow类的实现部分末尾

// 打开串口调试助手
void MainWindow::openSerialTool()
{
    // 创建串口调试助手窗口
    QDialog *serialToolDialog = new QDialog(this);
    serialToolDialog->setWindowTitle("串口调试助手");
    serialToolDialog->setMinimumSize(600, 400);

    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(serialToolDialog);

    // 创建串口设置区域
    QGroupBox *settingsGroup = new QGroupBox("串口设置");
    QGridLayout *settingsLayout = new QGridLayout(settingsGroup);

    // 添加串口选择
    QLabel *portLabel = new QLabel("串口:");
    QComboBox *portComboBox = new QComboBox();

    // 获取可用串口列表
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        portComboBox->addItem(info.portName());
    }

    // 添加波特率选择
    QLabel *baudLabel = new QLabel("波特率:");
    QComboBox *baudComboBox = new QComboBox();
    QList<qint32> baudRates = QSerialPortInfo::standardBaudRates();
    foreach(qint32 rate, baudRates) {
        baudComboBox->addItem(QString::number(rate));
    }
    baudComboBox->setCurrentText("115200");

    // 添加数据位选择
    QLabel *dataBitsLabel = new QLabel("数据位:");
    QComboBox *dataBitsComboBox = new QComboBox();
    dataBitsComboBox->addItem("5");
    dataBitsComboBox->addItem("6");
    dataBitsComboBox->addItem("7");
    dataBitsComboBox->addItem("8");
    dataBitsComboBox->setCurrentText("8");

    // 添加停止位选择
    QLabel *stopBitsLabel = new QLabel("停止位:");
    QComboBox *stopBitsComboBox = new QComboBox();
    stopBitsComboBox->addItem("1");
    stopBitsComboBox->addItem("1.5");
    stopBitsComboBox->addItem("2");
    stopBitsComboBox->setCurrentText("1");

    // 添加校验位选择
    QLabel *parityLabel = new QLabel("校验位:");
    QComboBox *parityComboBox = new QComboBox();
    parityComboBox->addItem("无");
    parityComboBox->addItem("奇校验");
    parityComboBox->addItem("偶校验");
    parityComboBox->addItem("空校验");
    parityComboBox->addItem("标记校验");

    // 添加打开/关闭按钮
    QPushButton *openButton = new QPushButton("打开串口");

    // 将控件添加到设置布局
    settingsLayout->addWidget(portLabel, 0, 0);
    settingsLayout->addWidget(portComboBox, 0, 1);
    settingsLayout->addWidget(baudLabel, 1, 0);
    settingsLayout->addWidget(baudComboBox, 1, 1);
    settingsLayout->addWidget(dataBitsLabel, 2, 0);
    settingsLayout->addWidget(dataBitsComboBox, 2, 1);
    settingsLayout->addWidget(stopBitsLabel, 3, 0);
    settingsLayout->addWidget(stopBitsComboBox, 3, 1);
    settingsLayout->addWidget(parityLabel, 4, 0);
    settingsLayout->addWidget(parityComboBox, 4, 1);
    settingsLayout->addWidget(openButton, 5, 0, 1, 2);

    // 创建数据显示区域
    QGroupBox *dataGroup = new QGroupBox("数据显示");
    QVBoxLayout *dataLayout = new QVBoxLayout(dataGroup);

    QTextEdit *receiveTextEdit = new QTextEdit();
    receiveTextEdit->setReadOnly(true);

    // 创建发送区域
    QGroupBox *sendGroup = new QGroupBox("数据发送");
    QVBoxLayout *sendLayout = new QVBoxLayout(sendGroup);

    QTextEdit *sendTextEdit = new QTextEdit();
    QPushButton *sendButton = new QPushButton("发送");

    QHBoxLayout *sendOptionsLayout = new QHBoxLayout();
    QCheckBox *hexDisplayCheckBox = new QCheckBox("HEX显示");
    QCheckBox *hexSendCheckBox = new QCheckBox("HEX发送");
    QCheckBox *autoSendCheckBox = new QCheckBox("自动发送");
    QLabel *intervalLabel = new QLabel("间隔(ms):");
    QSpinBox *intervalSpinBox = new QSpinBox();
    intervalSpinBox->setRange(100, 10000);
    intervalSpinBox->setValue(1000);
    intervalSpinBox->setSingleStep(100);

    sendOptionsLayout->addWidget(hexDisplayCheckBox);
    sendOptionsLayout->addWidget(hexSendCheckBox);
    sendOptionsLayout->addWidget(autoSendCheckBox);
    sendOptionsLayout->addWidget(intervalLabel);
    sendOptionsLayout->addWidget(intervalSpinBox);
    sendOptionsLayout->addStretch();

    sendLayout->addWidget(sendTextEdit);
    sendLayout->addLayout(sendOptionsLayout);
    sendLayout->addWidget(sendButton);

    dataLayout->addWidget(receiveTextEdit);

    // 将所有组添加到主布局
    mainLayout->addWidget(settingsGroup);
    mainLayout->addWidget(dataGroup);
    mainLayout->addWidget(sendGroup);

    // 显示对话框
    serialToolDialog->setAttribute(Qt::WA_DeleteOnClose);
    serialToolDialog->show();
}

// 打开网络调试助手
void MainWindow::openNetworkTool()
{
    // 创建网络调试助手窗口
    QDialog *networkToolDialog = new QDialog(this);
    networkToolDialog->setWindowTitle("网络调试助手");
    networkToolDialog->setMinimumSize(600, 400);

    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(networkToolDialog);

    // 创建网络设置区域
    QGroupBox *settingsGroup = new QGroupBox("网络设置");
    QGridLayout *settingsLayout = new QGridLayout(settingsGroup);

    // 添加协议选择
    QLabel *protocolLabel = new QLabel("协议类型:");
    QComboBox *protocolComboBox = new QComboBox();
    protocolComboBox->addItem("TCP客户端");
    protocolComboBox->addItem("TCP服务器");
    protocolComboBox->addItem("UDP");

    // 添加IP地址和端口
    QLabel *ipLabel = new QLabel("IP地址:");
    QLineEdit *ipLineEdit = new QLineEdit("127.0.0.1");

    QLabel *portLabel = new QLabel("端口:");
    QSpinBox *portSpinBox = new QSpinBox();
    portSpinBox->setRange(1, 65535);
    portSpinBox->setValue(8080);

    // 添加连接/断开按钮
    QPushButton *connectButton = new QPushButton("连接");

    // 将控件添加到设置布局
    settingsLayout->addWidget(protocolLabel, 0, 0);
    settingsLayout->addWidget(protocolComboBox, 0, 1);
    settingsLayout->addWidget(ipLabel, 1, 0);
    settingsLayout->addWidget(ipLineEdit, 1, 1);
    settingsLayout->addWidget(portLabel, 2, 0);
    settingsLayout->addWidget(portSpinBox, 2, 1);
    settingsLayout->addWidget(connectButton, 3, 0, 1, 2);

    // 创建数据显示区域
    QGroupBox *dataGroup = new QGroupBox("数据显示");
    QVBoxLayout *dataLayout = new QVBoxLayout(dataGroup);

    QTextEdit *receiveTextEdit = new QTextEdit();
    receiveTextEdit->setReadOnly(true);

    // 创建发送区域
    QGroupBox *sendGroup = new QGroupBox("数据发送");
    QVBoxLayout *sendLayout = new QVBoxLayout(sendGroup);

    QTextEdit *sendTextEdit = new QTextEdit();
    QPushButton *sendButton = new QPushButton("发送");

    QHBoxLayout *sendOptionsLayout = new QHBoxLayout();
    QCheckBox *hexDisplayCheckBox = new QCheckBox("HEX显示");
    QCheckBox *hexSendCheckBox = new QCheckBox("HEX发送");
    QCheckBox *autoSendCheckBox = new QCheckBox("自动发送");
    QLabel *intervalLabel = new QLabel("间隔(ms):");
    QSpinBox *intervalSpinBox = new QSpinBox();
    intervalSpinBox->setRange(100, 10000);
    intervalSpinBox->setValue(1000);
    intervalSpinBox->setSingleStep(100);

    sendOptionsLayout->addWidget(hexDisplayCheckBox);
    sendOptionsLayout->addWidget(hexSendCheckBox);
    sendOptionsLayout->addWidget(autoSendCheckBox);
    sendOptionsLayout->addWidget(intervalLabel);
    sendOptionsLayout->addWidget(intervalSpinBox);
    sendOptionsLayout->addStretch();

    sendLayout->addWidget(sendTextEdit);
    sendLayout->addLayout(sendOptionsLayout);
    sendLayout->addWidget(sendButton);

    dataLayout->addWidget(receiveTextEdit);

    // 将所有组添加到主布局
    mainLayout->addWidget(settingsGroup);
    mainLayout->addWidget(dataGroup);
    mainLayout->addWidget(sendGroup);

    // 显示对话框
    networkToolDialog->setAttribute(Qt::WA_DeleteOnClose);
    networkToolDialog->show();
}
