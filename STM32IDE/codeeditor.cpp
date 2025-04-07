/*
 * @Description:
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-05 21:44:22
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-04-07 21:53:00
 */
#include "codeeditor.h"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegExp>
#include <QToolBar>  // 添加工具栏头文件
#include <QAction>   // 添加动作头文件
#include <QIcon>     // 添加图标头文件
#include <QApplication> // 添加此行以使用QStyle
#include <QStyle>        // Add this for QStyle class

CodeEditor::CodeEditor(QWidget *parent) : QWidget(parent), m_currentEditor(nullptr), m_apiCPP(nullptr)
{
    // 创建主布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 创建工具栏
    createToolBar();
    layout->addWidget(m_toolBar);

    // 创建主分割器
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    layout->addWidget(m_mainSplitter);

    // 创建C++词法分析器
    m_lexerCPP = new QsciLexerCPP();

    // 创建第一个编辑器
    QsciScintilla* editor = new QsciScintilla(this);
    m_editors.append(editor);
    m_currentEditor = editor;
    m_mainSplitter->addWidget(editor);

    // 设置编辑器属性
    setupEditor(editor);

    // 设置自动补全
    setupAutoCompletion(editor);

    // 设置示例代码
    editor->setText("// STM32 代码编辑器\n#include <stdint.h>\n\nint main(void) {\n    // 初始化代码\n    while(1) {\n        // 主循环\n    }\n    return 0;\n}");

    // 连接信号和槽
    connect(editor, &QsciScintilla::textChanged, this, &CodeEditor::updateVariableList);
}

CodeEditor::~CodeEditor()
{
    if (m_apiCPP) {
        delete m_apiCPP;
        m_apiCPP = nullptr;
    }

    if (m_lexerCPP) {
        delete m_lexerCPP;
        m_lexerCPP = nullptr;
    }
}

QsciScintilla* CodeEditor::currentEditor() const
{
    return m_currentEditor;
}

QList<QsciScintilla*> CodeEditor::allEditors() const
{
    return m_editors;
}

void CodeEditor::setText(const QString &text)
{
    if (m_currentEditor) {
        m_currentEditor->setText(text);
    }
}

QString CodeEditor::text() const
{
    if (m_currentEditor) {
        return m_currentEditor->text();
    }
    return QString();
}

bool CodeEditor::openFile(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        // 在当前编辑器中显示文件内容
        if (m_currentEditor) {
            m_currentEditor->setText(content);
        }

        // 更新当前文件路径
        m_currentFilePath = filePath;

        // 解析文件中的变量并更新自动补全
        updateVariableList();

        return true;
    }
    return false;
}

bool CodeEditor::saveFile(const QString &filePath)
{
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

void CodeEditor::applyTheme(const QString &themeName)
{
    if (themeName == "dark") {
        // 深色主题
        for (QsciScintilla* editor : m_editors) {
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
        }

        // 设置语法高亮颜色
        m_lexerCPP->setColor(QColor("#569CD6"), QsciLexerCPP::Keyword); // 关键字
        m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::DoubleQuotedString); // 字符串
        m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::SingleQuotedString); // 字符
        m_lexerCPP->setColor(QColor("#B5CEA8"), QsciLexerCPP::Number); // 数字
        m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::Comment); // 注释
        m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::CommentLine); // 行注释
        m_lexerCPP->setColor(QColor("#C586C0"), QsciLexerCPP::PreProcessor); // 预处理器
        m_lexerCPP->setColor(QColor("#4EC9B0"), QsciLexerCPP::GlobalClass); // 类名

        // 设置背景色
        m_lexerCPP->setPaper(QColor("#1E1E1E"));

        // 设置默认字体
        QFont font("Consolas", 10);
        m_lexerCPP->setFont(font);
    } else if (themeName == "light") {
        // 浅色主题
        for (QsciScintilla* editor : m_editors) {
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
        }

        // 设置语法高亮颜色
        m_lexerCPP->setColor(QColor("#0000FF"), QsciLexerCPP::Keyword); // 关键字
        m_lexerCPP->setColor(QColor("#A31515"), QsciLexerCPP::DoubleQuotedString); // 字符串
        m_lexerCPP->setColor(QColor("#A31515"), QsciLexerCPP::SingleQuotedString); // 字符
        m_lexerCPP->setColor(QColor("#098658"), QsciLexerCPP::Number); // 数字
        m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::Comment); // 注释
        m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::CommentLine); // 行注释
        m_lexerCPP->setColor(QColor("#800000"), QsciLexerCPP::PreProcessor); // 预处理器
        m_lexerCPP->setColor(QColor("#267F99"), QsciLexerCPP::GlobalClass); // 类名

        // 设置背景色
        m_lexerCPP->setPaper(QColor("#FFFFFF"));

        // 设置默认字体
        QFont font("Consolas", 10);
        m_lexerCPP->setFont(font);
    }
    // 可以添加更多主题...
}

void CodeEditor::createSplitView(Qt::Orientation orientation)
{
    if (!m_currentEditor) {
        return;
    }

    // 创建新的分割器，替换当前编辑器
    QSplitter* splitter = new QSplitter(orientation);

    // 获取当前编辑器在主分割器中的索引
    int index = m_mainSplitter->indexOf(m_currentEditor);

    // 从主分割器中移除当前编辑器
    m_currentEditor->setParent(nullptr);

    // 将当前编辑器添加到新分割器
    splitter->addWidget(m_currentEditor);

    // 创建新的编辑器
    QsciScintilla* newEditor = new QsciScintilla();
    m_editors.append(newEditor);

    // 设置编辑器属性
    setupEditor(newEditor);

    // 设置自动补全
    setupAutoCompletion(newEditor);

    // 复制当前编辑器的内容到新编辑器
    newEditor->setText(m_currentEditor->text());

    // 将新编辑器添加到分割器
    splitter->addWidget(newEditor);

    // 将分割器添加到主分割器
    m_mainSplitter->insertWidget(index, splitter);

    // 设置分割器比例
    splitter->setSizes(QList<int>() << 1 << 1);

    // 将焦点设置到新编辑器
    newEditor->setFocus();
    m_currentEditor = newEditor;

    // 连接信号和槽
    connect(newEditor, &QsciScintilla::textChanged, this, &CodeEditor::updateVariableList);
}

void CodeEditor::closeSplitView()
{
    if (!m_currentEditor || m_editors.size() <= 1) {
        return;
    }

    // 获取当前编辑器的父分割器
    QSplitter* parentSplitter = qobject_cast<QSplitter*>(m_currentEditor->parent());
    if (!parentSplitter || parentSplitter == m_mainSplitter) {
        return;
    }

    // 获取分割器中的另一个部件
    QWidget* otherWidget = nullptr;
    for (int i = 0; i < parentSplitter->count(); ++i) {
        QWidget* widget = parentSplitter->widget(i);
        if (widget != m_currentEditor) {
            otherWidget = widget;
            break;
        }
    }

    if (!otherWidget) {
        return;
    }

    // 获取父分割器在主分割器中的索引
    int index = m_mainSplitter->indexOf(parentSplitter);

    // 从父分割器中移除另一个部件
    otherWidget->setParent(nullptr);

    // 将另一个部件添加到主分割器
    m_mainSplitter->insertWidget(index, otherWidget);

    // 从编辑器列表中移除当前编辑器
    m_editors.removeOne(m_currentEditor);

    // 删除当前编辑器
    delete m_currentEditor;

    // 删除父分割器
    delete parentSplitter;

    // 更新当前编辑器
    if (qobject_cast<QsciScintilla*>(otherWidget)) {
        m_currentEditor = qobject_cast<QsciScintilla*>(otherWidget);
    } else {
        m_currentEditor = m_editors.first();
    }

    // 将焦点设置到当前编辑器
    m_currentEditor->setFocus();
}

void CodeEditor::setupEditor(QsciScintilla* editor)
{
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

    // 设置自动换行
    editor->setWrapMode(QsciScintilla::WrapNone);

    // 设置光标宽度
    editor->setCaretWidth(2);

    // 设置行尾可见
    editor->setEolVisibility(false);

    // 设置缩进指南
    editor->setIndentationGuides(true);
}

void CodeEditor::setupAutoCompletion(QsciScintilla* editor)
{
    // 如果还没有创建API对象，则创建
    if (!m_apiCPP) {
        m_apiCPP = new QsciAPIs(m_lexerCPP);

        // 添加C/C++关键字和STM32相关API
        QStringList keywords;
        // C/C++关键字
        keywords << "auto" << "break" << "case" << "char" << "const" << "continue" << "default"
                 << "do" << "double" << "else" << "enum" << "extern" << "float" << "for"
                 << "goto" << "if" << "int" << "long" << "register" << "return" << "short"
                 << "signed" << "sizeof" << "static" << "struct" << "switch" << "typedef"
                 << "union" << "unsigned" << "void" << "volatile" << "while";

        // STM32数据类型
        keywords << "uint8_t" << "uint16_t" << "uint32_t" << "int8_t" << "int16_t" << "int32_t"
                 << "GPIO_InitTypeDef" << "USART_InitTypeDef" << "TIM_TimeBaseInitTypeDef";

        // STM32特定函数和宏
        keywords <<"GPIO_Init"<< "GPIO_SetBits"<< "GPIO_ResetBits"<< "GPIO_ReadInputDataBit"
                 <<"TIM_TimeBaseInit"<< "TIM_Cmd"<<"TIM_ITConfig"<< "TIM_GetCounter"
                 <<"USART_Init"<<"USART_Cmd"<< "USART_SendData"<< "USART_ReceiveData"
                 <<"ADC_Init"<< "ADC_Cmd"<< "ADC_StartConversion"<< "ADC_GetConversionValue"
                 <<"RCC_APB1PeriphClockCmd"<< "RCC_APB2PeriphClockCmd"<< "RCC_AHB1PeriphClockCmd"
                 <<"NVIC_Init"<<"NVIC_EnableIRQ"<< "NVIC_DisableIRQ"
                 <<"SysTick_Config"<< "HAL_Delay"<< "HAL_GPIO_WritePin"<< "HAL_GPIO_ReadPin";

        // STM32 HAL库函数
        keywords << "HAL_Init()" << "HAL_GPIO_Init()" << "HAL_GPIO_WritePin()" << "HAL_GPIO_ReadPin()"
                 << "HAL_Delay()" << "HAL_UART_Init()" << "HAL_UART_Transmit()" << "HAL_UART_Receive()"
                 << "HAL_TIM_Base_Init()" << "HAL_TIM_Base_Start()" << "HAL_TIM_Base_Stop()";

        // 添加到API
        for (const QString &keyword : keywords) {
            m_apiCPP->add(keyword);
        }

        // 准备API
        m_apiCPP->prepare();
    }

    // 设置自动补全
    editor->setAutoCompletionThreshold(1); // 输入1个字符后显示补全
    editor->setAutoCompletionSource(QsciScintilla::AcsAll); // 使用所有可用的补全源
    editor->setAutoCompletionCaseSensitivity(false); // 不区分大小写
    editor->setAutoCompletionReplaceWord(true); // 替换当前单词
    editor->setAutoCompletionUseSingle(QsciScintilla::AcusNever); // 不自动选择唯一匹配项

    // 启用自动补全弹出
    editor->setAutoCompletionFillupsEnabled(true);
}

void CodeEditor::updateVariableList()
{
    if (!m_currentEditor) {
        return;
    }

    // 获取当前文本
    QString text = m_currentEditor->text();

    // 创建一个新的API对象
    QsciAPIs* newApi = new QsciAPIs(m_lexerCPP);

    // 添加C/C++关键字和STM32相关API (重新添加所有标准关键字)
    QStringList keywords;
    // C/C++关键字
    keywords << "auto" << "break" << "case" << "char" << "const" << "continue" << "default"
             << "do" << "double" << "else" << "enum" << "extern" << "float" << "for"
             << "goto" << "if" << "int" << "long" << "register" << "return" << "short"
             << "signed" << "sizeof" << "static" << "struct" << "switch" << "typedef"
             << "union" << "unsigned" << "void" << "volatile" << "while";

    // STM32数据类型
    keywords << "uint8_t" << "uint16_t" << "uint32_t" << "int8_t" << "int16_t" << "int32_t"
             << "GPIO_InitTypeDef" << "USART_InitTypeDef" << "TIM_TimeBaseInitTypeDef";

    // STM32特定函数和宏
    keywords <<"GPIO_Init"<< "GPIO_SetBits"<< "GPIO_ResetBits"<< "GPIO_ReadInputDataBit"
             <<"TIM_TimeBaseInit"<< "TIM_Cmd"<<"TIM_ITConfig"<< "TIM_GetCounter"
             <<"USART_Init"<<"USART_Cmd"<< "USART_SendData"<< "USART_ReceiveData"
             <<"ADC_Init"<< "ADC_Cmd"<< "ADC_StartConversion"<< "ADC_GetConversionValue"
             <<"RCC_APB1PeriphClockCmd"<< "RCC_APB2PeriphClockCmd"<< "RCC_AHB1PeriphClockCmd"
             <<"NVIC_Init"<<"NVIC_EnableIRQ"<< "NVIC_DisableIRQ"
             <<"SysTick_Config"<< "HAL_Delay"<< "HAL_GPIO_WritePin"<< "HAL_GPIO_ReadPin";

    // STM32 HAL库函数
    keywords << "HAL_Init()" << "HAL_GPIO_Init()" << "HAL_GPIO_WritePin()" << "HAL_GPIO_ReadPin()"
             << "HAL_Delay()" << "HAL_UART_Init()" << "HAL_UART_Transmit()" << "HAL_UART_Receive()"
             << "HAL_TIM_Base_Init()" << "HAL_TIM_Base_Start()" << "HAL_TIM_Base_Stop()";

    // 添加关键字到新API
    for (const QString &keyword : keywords) {
        newApi->add(keyword);
    }

    // 使用正则表达式查找变量定义 - 改进的正则表达式
    QRegExp varRegex("\\b(int|char|float|double|uint8_t|uint16_t|uint32_t|int8_t|int16_t|int32_t|bool|void|unsigned|long|short|signed|struct|enum|union)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*[;\\[=,)]");

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
            QRegExp paramRegex("\\b(\\w+)\\s+([\\*&]*)\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\b");
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
    QRegExp forRegex("for\\s*\\(\\s*(?:int|char|float|double|uint8_t|uint16_t|uint32_t)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=");
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
    for (QsciScintilla* editor : m_editors) {
        editor->setAutoCompletionSource(QsciScintilla::AcsAll); // 使用所有可用的补全源
    }
}

void CodeEditor::onEditorChanged(QsciScintilla* editor)
{
    if (m_editors.contains(editor)) {
        m_currentEditor = editor;
    }
}

// 添加创建工具栏的方法
void CodeEditor::createToolBar()
{
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

    QAction* newFileAction = new QAction("新建文件", this);
    newFileAction->setIcon(style->standardIcon(QStyle::SP_FileIcon));
    newFileAction->setToolTip("创建新文件");
    connect(newFileAction, &QAction::triggered, this, &CodeEditor::createNewFile);
    m_toolBar->addAction(newFileAction);

    QAction* openFileAction = new QAction("打开文件", this);
    openFileAction->setIcon(style->standardIcon(QStyle::SP_DialogOpenButton));
    openFileAction->setToolTip("打开文件");
    connect(openFileAction, &QAction::triggered, [this]() {
        // 实现打开文件功能
        emit openFileRequested();
    });
    m_toolBar->addAction(openFileAction);

    QAction* saveFileAction = new QAction("保存文件", this);
    saveFileAction->setIcon(style->standardIcon(QStyle::SP_DialogSaveButton));
    saveFileAction->setToolTip("保存文件");
    connect(saveFileAction, &QAction::triggered, [this]() {
        // 实现保存文件功能
        emit saveFileRequested();
    });
    m_toolBar->addAction(saveFileAction);

    m_toolBar->addSeparator();

    // 水平分栏动作
    QAction* horizontalSplitAction = new QAction(QIcon(":/icons/horizontal_split.png"), "水平分栏", this);
    horizontalSplitAction->setToolTip("创建水平分栏");
    connect(horizontalSplitAction, &QAction::triggered, [this]() {
        createSplitView(Qt::Horizontal);
    });
    m_toolBar->addAction(horizontalSplitAction);

    // 垂直分栏动作
    QAction* verticalSplitAction = new QAction(QIcon(":/icons/vertical_split.png"), "垂直分栏", this);
    verticalSplitAction->setToolTip("创建垂直分栏");
    connect(verticalSplitAction, &QAction::triggered, [this]() {
        createSplitView(Qt::Vertical);
    });
    m_toolBar->addAction(verticalSplitAction);

    // 关闭分栏动作
    QAction* closeSplitAction = new QAction(QIcon(":/icons/close_split.png"), "关闭分栏", this);
    closeSplitAction->setToolTip("关闭当前分栏");
    connect(closeSplitAction, &QAction::triggered, [this]() {
        closeSplitView();
    });
    m_toolBar->addAction(closeSplitAction);

    m_toolBar->addSeparator();

    // 撤销动作
    QAction* undoAction = new QAction(QIcon(":/icons/undo.png"), "撤销", this);
    undoAction->setToolTip("撤销上一步操作");
    connect(undoAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            m_currentEditor->undo();
        }
    });
    m_toolBar->addAction(undoAction);

    // 重做动作
    QAction* redoAction = new QAction(QIcon(":/icons/redo.png"), "重做", this);
    redoAction->setToolTip("重做上一步操作");
    connect(redoAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            m_currentEditor->redo();
        }
    });
    m_toolBar->addAction(redoAction);

    m_toolBar->addSeparator();

    // 剪切动作
    QAction* cutAction = new QAction(QIcon(":/icons/cut.png"), "剪切", this);
    cutAction->setToolTip("剪切选中的文本");
    connect(cutAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            m_currentEditor->cut();
        }
    });
    m_toolBar->addAction(cutAction);

    // 复制动作
    QAction* copyAction = new QAction(QIcon(":/icons/copy.png"), "复制", this);
    copyAction->setToolTip("复制选中的文本");
    connect(copyAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            m_currentEditor->copy();
        }
    });
    m_toolBar->addAction(copyAction);

    // 粘贴动作
    QAction* pasteAction = new QAction(QIcon(":/icons/paste.png"), "粘贴", this);
    pasteAction->setToolTip("粘贴文本");
    connect(pasteAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            m_currentEditor->paste();
        }
    });
    m_toolBar->addAction(pasteAction);

    m_toolBar->addSeparator();

    // 查找动作
    QAction* findAction = new QAction(QIcon(":/icons/find.png"), "查找", this);
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
    QAction* replaceAction = new QAction(QIcon(":/icons/replace.png"), "替换", this);
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
    QAction* indentAction = new QAction(QIcon(":/icons/indent.png"), "增加缩进", this);
    indentAction->setToolTip("增加选中文本的缩进");
    connect(indentAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            // QsciScintilla doesn't have an indent() method
            // Instead, we need to manually insert spaces or tabs at the beginning of selected lines
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
    QAction* unindentAction = new QAction(QIcon(":/icons/unindent.png"), "减少缩进", this);
    unindentAction->setToolTip("减少选中文本的缩进");
    connect(unindentAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            // QsciScintilla doesn't have an unindent() method
            // Instead, we need to manually remove spaces or tabs from the beginning of selected lines
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
    QAction* commentAction = new QAction(QIcon(":/icons/comment.png"), "注释", this);
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
    QAction* uncommentAction = new QAction(QIcon(":/icons/uncomment.png"), "取消注释", this);
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
    QAction* buildAction = new QAction("构建", this);
    buildAction->setIcon(style->standardIcon(QStyle::SP_ComputerIcon));
    buildAction->setToolTip("构建项目");
    connect(buildAction, &QAction::triggered, [this]() {
        // 实现构建功能
        emit buildRequested();
    });
    m_toolBar->addAction(buildAction);

    QAction* cleanAction = new QAction("清理", this);
    cleanAction->setIcon(style->standardIcon(QStyle::SP_TrashIcon));
    cleanAction->setToolTip("清理构建文件");
    connect(cleanAction, &QAction::triggered, [this]() {
        // 实现清理功能
        emit cleanRequested();
    });
    m_toolBar->addAction(cleanAction);

    m_toolBar->addSeparator();

    // 调试操作
    QAction* debugAction = new QAction("调试", this);
    debugAction->setIcon(style->standardIcon(QStyle::SP_BrowserReload));
    debugAction->setToolTip("开始调试");
    connect(debugAction, &QAction::triggered, [this]() {
        // 实现调试功能
        emit debugRequested();
    });
    m_toolBar->addAction(debugAction);

    QAction* runAction = new QAction("运行", this);
    runAction->setIcon(style->standardIcon(QStyle::SP_MediaPlay));
    runAction->setToolTip("运行程序");
    connect(runAction, &QAction::triggered, [this]() {
        // 实现运行功能
        emit runRequested();
    });
    m_toolBar->addAction(runAction);

    QAction* stopAction = new QAction("停止", this);
    stopAction->setIcon(style->standardIcon(QStyle::SP_MediaStop));
    stopAction->setToolTip("停止运行");
    connect(stopAction, &QAction::triggered, [this]() {
        // 实现停止功能
        emit stopRequested();
    });
    m_toolBar->addAction(stopAction);

    m_toolBar->addSeparator();

    // 工具操作
    QAction* serialMonitorAction = new QAction("串口监视器", this);
    serialMonitorAction->setIcon(style->standardIcon(QStyle::SP_ComputerIcon));
    serialMonitorAction->setToolTip("打开串口监视器");
    connect(serialMonitorAction, &QAction::triggered, [this]() {
        // 实现串口监视器功能
        emit serialMonitorRequested();
    });
    m_toolBar->addAction(serialMonitorAction);

    QAction* settingsAction = new QAction("设置", this);
    settingsAction->setIcon(style->standardIcon(QStyle::SP_FileDialogDetailedView));
    settingsAction->setToolTip("打开设置");
    connect(settingsAction, &QAction::triggered, [this]() {
        // 实现设置功能
        emit settingsRequested();
    });
    m_toolBar->addAction(settingsAction);

    m_toolBar->addSeparator();

    // 视图操作
    QAction* zoomInAction = new QAction("放大", this);
    zoomInAction->setIcon(style->standardIcon(QStyle::SP_TitleBarMaxButton));
    zoomInAction->setToolTip("放大视图");
    connect(zoomInAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            m_currentEditor->zoomIn();
        }
    });
    m_toolBar->addAction(zoomInAction);

    QAction* zoomOutAction = new QAction("缩小", this);
    zoomOutAction->setIcon(style->standardIcon(QStyle::SP_TitleBarMinButton));
    zoomOutAction->setToolTip("缩小视图");
    connect(zoomOutAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            m_currentEditor->zoomOut();
        }
    });
    m_toolBar->addAction(zoomOutAction);

    QAction* resetZoomAction = new QAction("重置缩放", this);
    resetZoomAction->setIcon(style->standardIcon(QStyle::SP_TitleBarNormalButton));
    resetZoomAction->setToolTip("重置缩放级别");
    connect(resetZoomAction, &QAction::triggered, [this]() {
        if (m_currentEditor) {
            m_currentEditor->zoomTo(0);
        }
    });
    m_toolBar->addAction(resetZoomAction);

    m_toolBar->addSeparator();

    // 帮助操作
    QAction* helpAction = new QAction("帮助", this);
    helpAction->setIcon(style->standardIcon(QStyle::SP_MessageBoxQuestion));
    helpAction->setToolTip("查看帮助");
    connect(helpAction, &QAction::triggered, [this]() {
        // 实现帮助功能
        emit helpRequested();
    });
    m_toolBar->addAction(helpAction);

    QAction* aboutAction = new QAction("关于", this);
    aboutAction->setIcon(style->standardIcon(QStyle::SP_MessageBoxInformation));
    aboutAction->setToolTip("关于STM32IDE");
    connect(aboutAction, &QAction::triggered, [this]() {
        // 实现关于功能
        emit aboutRequested();
    });
    m_toolBar->addAction(aboutAction);
}

void CodeEditor::createNewFile()
{
    // 清空当前编辑器内容
    if (m_currentEditor) {
        m_currentEditor->clear();
    }

    // 重置当前文件路径
    m_currentFilePath = "";

    // 发出信号通知主窗口
    emit newFileRequested();
}
