/*
 * @Description:
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-05 21:44:22
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-04-13 10:34:04
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
#include <QStack>
#include <QPair>     // 添加QPair头文件
#include <QRegularExpression> // Add this line to include
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
//             editor->setFoldMarginColors(QColor("#1E1E1E"), QColor("#1E1E1E"));

//             // 设置选中文本的颜色
//             editor->setSelectionBackgroundColor(QColor("#264F78"));
//             editor->setSelectionForegroundColor(QColor("#FFFFFF"));
//         }

//         // 设置语法高亮颜色
//         m_lexerCPP->setColor(QColor("#569CD6"), QsciLexerCPP::Keyword); // 关键字
//         m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::DoubleQuotedString); // 字符串
//         m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::SingleQuotedString); // 字符
//         m_lexerCPP->setColor(QColor("#B5CEA8"), QsciLexerCPP::Number); // 数字
//         m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::Comment); // 注释
//         m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::CommentLine); // 行注释
//         m_lexerCPP->setColor(QColor("#C586C0"), QsciLexerCPP::PreProcessor); // 预处理器
//         m_lexerCPP->setColor(QColor("#4EC9B0"), QsciLexerCPP::GlobalClass); // 类名

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
//             editor->setFoldMarginColors(QColor("#F0F0F0"), QColor("#F0F0F0"));

//             // 设置选中文本的颜色
//             editor->setSelectionBackgroundColor(QColor("#ADD6FF"));
//             editor->setSelectionForegroundColor(QColor("#000000"));
//         }

//         // 设置语法高亮颜色
//         m_lexerCPP->setColor(QColor("#0000FF"), QsciLexerCPP::Keyword); // 关键字
//         m_lexerCPP->setColor(QColor("#A31515"), QsciLexerCPP::DoubleQuotedString); // 字符串
//         m_lexerCPP->setColor(QColor("#A31515"), QsciLexerCPP::SingleQuotedString); // 字符
//         m_lexerCPP->setColor(QColor("#098658"), QsciLexerCPP::Number); // 数字
//         m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::Comment); // 注释
//         m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::CommentLine); // 行注释
//         m_lexerCPP->setColor(QColor("#800000"), QsciLexerCPP::PreProcessor); // 预处理器
//         m_lexerCPP->setColor(QColor("#267F99"), QsciLexerCPP::GlobalClass); // 类名

//         // 设置背景色
//         m_lexerCPP->setPaper(QColor("#FFFFFF"));

//         // 设置默认字体
//         QFont font("Consolas", 10);
//         m_lexerCPP->setFont(font);
//     }
//     // 可以添加更多主题...
// }


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

        // 添加函数名颜色设置 - 橙色
        //m_lexerCPP->setColor(QColor("#CE9178"), QsciLexerCPP::Identifier); // 标识符(包括函数名)
        // 或者使用更专门的函数名样式
        // m_lexerCPP->setColor(QColor("#DCDCAA"), QsciLexerCPP::FunctionMethodName); // 函数方法名
        // 恢复标识符的默认颜色
        m_lexerCPP->setColor(QColor("#DCDCDC"), QsciLexerCPP::Identifier); // 标识符恢复默认颜色

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

        // 添加函数名颜色设置 - 橙色
        m_lexerCPP->setColor(QColor("#000000"), QsciLexerCPP::Identifier); // 标识符(包括函数名)

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


// // 添加新方法：设置函数名高亮
// void CodeEditor::setupFunctionNameHighlighting(bool isDarkTheme)
// {
//     // 为每个编辑器设置函数名高亮
//     for (QsciScintilla* editor : m_editors) {
//         // 使用自定义指示器来高亮函数名
//         const int FUNCTION_INDICATOR = 20;

//         // 设置指示器样式为文本前景色
//         editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, FUNCTION_INDICATOR, QsciScintilla::INDIC_TEXTFORE);

//         // 根据主题设置颜色
//         if (isDarkTheme) {
//             // 深色主题 - 橙黄色
//             editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, FUNCTION_INDICATOR, 0xAACDDC); // 注意：颜色格式为BGR
//         } else {
//             // 浅色主题 - 暗金色
//             editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, FUNCTION_INDICATOR, 0x0B8686); // 注意：颜色格式为BGR
//         }

//         // 设置指示器透明度
//         editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, FUNCTION_INDICATOR, 255);

//         // 连接文本变化信号，以便在文本变化时更新函数名高亮
//         disconnect(editor, &QsciScintilla::textChanged, nullptr, nullptr); // 断开之前的连接
//         connect(editor, &QsciScintilla::textChanged, [this, editor]() {
//             highlightFunctionNames(editor, FUNCTION_INDICATOR);
//         });

//         // 初始化时高亮函数名
//         highlightFunctionNames(editor, FUNCTION_INDICATOR);
//     }
// }

// // 添加新方法：高亮函数名
// void CodeEditor::highlightFunctionNames(QsciScintilla* editor, int indicatorId)
// {
//     // 清除现有的函数名高亮
//     editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(), indicatorId);

//     // 获取编辑器文本
//     QString text = editor->text();

//     // 使用正则表达式匹配函数名
//     // 匹配模式：返回类型 + 函数名 + 参数列表
//     QRegularExpression functionRegex(R"((\w+(?:\s+\w+)*\s+)(\w+)\s*\()");
//     QRegularExpressionMatchIterator matches = functionRegex.globalMatch(text);

//     while (matches.hasNext()) {
//         QRegularExpressionMatch match = matches.next();

//         // 获取函数名的捕获组
//         QString functionName = match.captured(2);

//         // 获取函数名在文本中的位置
//         int startPos = match.capturedStart(2);
//         int endPos = startPos + functionName.length();

//         // 将位置转换为行和列
//         int startLine = 0, startCol = 0, endLine = 0, endCol = 0;
//         editor->lineIndexFromPosition(startPos, &startLine, &startCol);
//         editor->lineIndexFromPosition(endPos, &endLine, &endCol);

//         // 高亮函数名
//         editor->fillIndicatorRange(startLine, startCol, endLine, endCol, indicatorId);
//     }
// }

// 添加新方法：设置函数名高亮
void CodeEditor::setupFunctionNameHighlighting(bool isDarkTheme)
{
    // 为每个编辑器设置函数名高亮
    for (QsciScintilla* editor : m_editors) {
        // 使用自定义指示器来高亮函数名
        const int FUNCTION_INDICATOR = 20;

        // 设置指示器样式为文本前景色
        editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, FUNCTION_INDICATOR, QsciScintilla::INDIC_TEXTFORE);

        // 根据主题设置颜色 - 修改为更明显的橙色
        if (isDarkTheme) {
            // 深色主题 - 明亮的橙色 (注意：颜色格式为BGR)
            editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, FUNCTION_INDICATOR, 0xAA78DC); // 橙色 #DCDCAA
        } else {
            // 浅色主题 - 暗金色
            editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, FUNCTION_INDICATOR, 0x0B6DB8); // 暗金色 #B86D0B
        }

        // 设置指示器透明度和优先级
        editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, FUNCTION_INDICATOR, 255);
        editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, FUNCTION_INDICATOR, false); // 不在文本下方显示
        editor->SendScintilla(QsciScintilla::SCI_INDICSETOUTLINEALPHA, FUNCTION_INDICATOR, 255); // 设置轮廓透明度

        // 断开之前的连接并重新连接
        disconnect(editor, &QsciScintilla::textChanged, nullptr, nullptr);
        connect(editor, &QsciScintilla::textChanged, [this, editor]() {
            // 确保在文本变化后立即高亮函数名
            QTimer::singleShot(0, [this, editor]() {
                highlightFunctionNames(editor, 20);
            });
        });

        // 初始化时高亮函数名
        highlightFunctionNames(editor, FUNCTION_INDICATOR);
    }
}

// 修改高亮函数名的方法
void CodeEditor::highlightFunctionNames(QsciScintilla* editor, int indicatorId)
{
    // 清除现有的函数名高亮
    editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(), indicatorId);

    // 获取编辑器文本
    QString text = editor->text();

    // 使用更精确的正则表达式匹配函数名
    // 匹配模式：返回类型 + 函数名 + 参数列表
    QRegularExpression functionRegex(R"((\b\w+(?:\s+\w+)*\s+)(\w+)\s*\()");
    QRegularExpressionMatchIterator matches = functionRegex.globalMatch(text);

    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();

        // 获取函数名的捕获组
        QString functionName = match.captured(2);

        // 跳过关键字
        if (functionName == "if" || functionName == "for" || functionName == "while" ||
            functionName == "switch" || functionName == "return" || functionName == "else") {
            continue;
        }

        // 获取函数名在文本中的位置
        int startPos = match.capturedStart(2);
        int endPos = startPos + functionName.length();

        // 将位置转换为行和列
        int startLine = 0, startCol = 0, endLine = 0, endCol = 0;
        editor->lineIndexFromPosition(startPos, &startLine, &startCol);
        editor->lineIndexFromPosition(endPos, &endLine, &endCol);

        // 高亮函数名
        editor->fillIndicatorRange(startLine, startCol, endLine, endCol, indicatorId);
    }
}
void CodeEditor::createSplitView(Qt::Orientation orientation)
{
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
    // QsciScintilla* newEditor = new QsciScintilla();
    // m_editors.append(newEditor);

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
    // connect(newEditor, &QsciScintilla::textChanged, this, &CodeEditor::updateVariableList);
    if (!m_currentEditor) {
        return;
    }

    // 获取当前编辑器的父分割器
    QSplitter* parentSplitter = qobject_cast<QSplitter*>(m_currentEditor->parent());
    if (!parentSplitter) {
        parentSplitter = m_mainSplitter;
    }

    // 创建新的分割器，替换当前编辑器
    QSplitter* splitter = new QSplitter(orientation);

    // 获取当前编辑器在父分割器中的索引
    int index = parentSplitter->indexOf(m_currentEditor);

    // 从父分割器中移除当前编辑器
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

    // 将分割器添加到父分割器
    parentSplitter->insertWidget(index, splitter);

    // 设置分割器比例
    splitter->setSizes(QList<int>() << 1 << 1);

    // 将焦点设置到新编辑器
    newEditor->setFocus();
    m_currentEditor = newEditor;

    // 连接信号和槽
    connect(newEditor, &QsciScintilla::textChanged, this, &CodeEditor::updateVariableList);

}



// void CodeEditor::closeSplitView()
// {
//     if (!m_currentEditor || m_editors.size() <= 1) {
//         return;
//     }

//     // 获取当前编辑器的父分割器
//     QSplitter* parentSplitter = qobject_cast<QSplitter*>(m_currentEditor->parent());
//     if (!parentSplitter || parentSplitter == m_mainSplitter) {
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
//     } else if (QSplitter* otherSplitter = qobject_cast<QSplitter*>(otherWidget)) {
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
//     QSplitter* parentSplitter = qobject_cast<QSplitter*>(m_currentEditor->parent());
//     if (!parentSplitter || parentSplitter == m_mainSplitter) {
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
//     } else if (QSplitter* otherSplitter = qobject_cast<QSplitter*>(otherWidget)) {
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

    // 从编辑器列表中移除当前编辑器
    m_editors.removeOne(m_currentEditor);

    // 保存当前编辑器的引用，以便后面删除
    QsciScintilla* editorToDelete = m_currentEditor;

    // 获取父分割器的父部件
    QWidget* grandParent = qobject_cast<QWidget*>(parentSplitter->parent());
    QSplitter* grandParentSplitter = nullptr;

    // 先更新当前编辑器引用，避免使用已删除的指针
    if (qobject_cast<QsciScintilla*>(otherWidget)) {
        m_currentEditor = qobject_cast<QsciScintilla*>(otherWidget);
    } else if (QSplitter* otherSplitter = qobject_cast<QSplitter*>(otherWidget)) {
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
    }
    else {
        // 处理嵌套分割器的情况
        grandParentSplitter = qobject_cast<QSplitter*>(grandParent);
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
QsciScintilla* CodeEditor::findFirstEditor(QSplitter* splitter)
{
    if (!splitter) {
        return nullptr;
    }

    for (int i = 0; i < splitter->count(); ++i) {
        QWidget* widget = splitter->widget(i);

        // 如果是编辑器，直接返回
        QsciScintilla* editor = qobject_cast<QsciScintilla*>(widget);
        if (editor) {
            return editor;
        }

        // 如果是分割器，递归查找
        QSplitter* childSplitter = qobject_cast<QSplitter*>(widget);
        if (childSplitter) {
            QsciScintilla* foundEditor = findFirstEditor(childSplitter);
            if (foundEditor) {
                return foundEditor;
            }
        }
    }

    return nullptr;
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
    
    // 设置括号匹配的样式和颜色
    // 注意：QScintilla默认只支持一种括号匹配颜色，但我们可以通过设置不同的样式来区分
    editor->setMatchedBraceBackgroundColor(QColor("#3B514D")); // 匹配的括号背景色
    editor->setMatchedBraceForegroundColor(QColor("#FFD700")); // 匹配的括号前景色 - 金色
    editor->setUnmatchedBraceBackgroundColor(QColor("#4B1515")); // 不匹配的括号背景色
    editor->setUnmatchedBraceForegroundColor(QColor("#FF0000")); // 不匹配的括号前景色 - 红色

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
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator, ROUND_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#4EC9B0"), ROUND_BRACE_INDICATOR); // 青绿色
//     editor->setIndicatorOutlineColor(QColor("#4EC9B0"), ROUND_BRACE_INDICATOR);
    
//     // 设置方括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator, SQUARE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#CE9178"), SQUARE_BRACE_INDICATOR); // 橙色
//     editor->setIndicatorOutlineColor(QColor("#CE9178"), SQUARE_BRACE_INDICATOR);
    
//     // 设置花括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator, CURLY_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#569CD6"), CURLY_BRACE_INDICATOR); // 蓝色
//     editor->setIndicatorOutlineColor(QColor("#569CD6"), CURLY_BRACE_INDICATOR);
    
//     // 设置尖括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator, ANGLE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#C586C0"), ANGLE_BRACE_INDICATOR); // 紫色
//     editor->setIndicatorOutlineColor(QColor("#C586C0"), ANGLE_BRACE_INDICATOR);
    
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
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, ROUND_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, ROUND_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, ROUND_BRACE_INDICATOR, 0x4EC9B0);

//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, SQUARE_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, SQUARE_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, SQUARE_BRACE_INDICATOR, 0xCE9178);

//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, CURLY_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, CURLY_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, CURLY_BRACE_INDICATOR, 0x569CD6);

//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, ANGLE_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, ANGLE_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, ANGLE_BRACE_INDICATOR, 0xC586C0);



//     // 设置圆括号指示器样式 - 使用更明显的样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator, ROUND_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#4EC9B0"), ROUND_BRACE_INDICATOR); // 青绿色

//     // 设置方括号指示器样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator, SQUARE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#CE9178"), SQUARE_BRACE_INDICATOR); // 橙色

//     // 设置花括号指示器样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator, CURLY_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#569CD6"), CURLY_BRACE_INDICATOR); // 蓝色

//     // 设置尖括号指示器样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator, ANGLE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#C586C0"), ANGLE_BRACE_INDICATOR); // 紫色

//     // 连接文本变化信号，以便在文本变化时更新括号颜色
//     connect(editor, &QsciScintilla::textChanged, [this, editor]() {
//         highlightBraces(editor);
//     });

//     // 初始化时高亮括号
//     highlightBraces(editor);
// }

void CodeEditor::setupBraceColors(QsciScintilla* editor)
{
    // 定义不同类型的括号指示器
    const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
    const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
    const int CURLY_BRACE_INDICATOR = 10; // 花括号 {}
    const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

    // 删除重复的indicatorDefine调用，只保留SendScintilla方法

    // 设置圆括号指示器样式 - 使用Scintilla原生API
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, ROUND_BRACE_INDICATOR, 255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, ROUND_BRACE_INDICATOR, true);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, ROUND_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, ROUND_BRACE_INDICATOR, 0x4EC9B0); // 青绿色

    // 设置方括号指示器样式
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, SQUARE_BRACE_INDICATOR, 255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, SQUARE_BRACE_INDICATOR, true);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, SQUARE_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, SQUARE_BRACE_INDICATOR, 0xCE9178); // 橙色

    // 设置花括号指示器样式 - 特别关注花括号的颜色区分
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, CURLY_BRACE_INDICATOR, 255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, CURLY_BRACE_INDICATOR, true);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, CURLY_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, CURLY_BRACE_INDICATOR, 0x569CD6); // 蓝色

    // 设置尖括号指示器样式
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, ANGLE_BRACE_INDICATOR, 255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, ANGLE_BRACE_INDICATOR, true);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, ANGLE_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, ANGLE_BRACE_INDICATOR, 0xC586C0); // 紫色

    // 连接文本变化信号，以便在文本变化时更新括号颜色
    connect(editor, &QsciScintilla::textChanged, [this, editor]() {
        highlightBraces(editor);
    });

    // 初始化时高亮括号
    highlightBraces(editor);
}

// // 添加新方法：高亮不同类型的括号
// void CodeEditor::highlightBraces(QsciScintilla* editor)
// {
//     // 获取编辑器文本
//     QString text = editor->text();
    
//     // 定义不同类型的括号指示器
//     const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
//     const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
//     const int CURLY_BRACE_INDICATOR = 10; // 花括号 {}
//     const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>
    
//     // 清除所有括号指示器
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, ROUND_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, SQUARE_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, CURLY_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, ANGLE_BRACE_INDICATOR);
    
//     // 使用栈来匹配括号对
//     QStack<QPair<int, int>> roundBraceStack;  // 圆括号栈 (行, 列)
//     QStack<QPair<int, int>> squareBraceStack; // 方括号栈
//     QStack<QPair<int, int>> curlyBraceStack;  // 花括号栈
//     QStack<QPair<int, int>> angleBraceStack;  // 尖括号栈
    
//     // 遍历文本中的每个字符
//     for (int line = 0; line < editor->lines(); line++) {
//         QString lineText = editor->text(line);
        
//         for (int col = 0; col < lineText.length(); col++) {
//             QChar ch = lineText.at(col);
            
//             // 处理圆括号
//             if (ch == '(') {
//                 roundBraceStack.push(qMakePair(line, col));
//             } else if (ch == ')') {
//                 if (!roundBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = roundBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ROUND_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, ROUND_BRACE_INDICATOR);
//                 }
//             }
            
//             // 处理方括号
//             if (ch == '[') {
//                 squareBraceStack.push(qMakePair(line, col));
//             } else if (ch == ']') {
//                 if (!squareBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = squareBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, SQUARE_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, SQUARE_BRACE_INDICATOR);
//                 }
//             }
            
//             // 处理花括号
//             if (ch == '{') {
//                 curlyBraceStack.push(qMakePair(line, col));
//             } else if (ch == '}') {
//                 if (!curlyBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = curlyBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, CURLY_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, CURLY_BRACE_INDICATOR);
//                 }
//             }
            
//             // 处理尖括号 (注意：这里可能会与小于/大于符号混淆，实际应用中可能需要更复杂的逻辑)
//             if (ch == '<') {
//                 angleBraceStack.push(qMakePair(line, col));
//             } else if (ch == '>') {
//                 if (!angleBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = angleBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ANGLE_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, ANGLE_BRACE_INDICATOR);
//                 }
//             }
//         }
//     }
// }


// void CodeEditor::highlightBraces(QsciScintilla* editor)
// {
//     // 获取编辑器文本
//     QString text = editor->text();

//     // 定义不同类型的括号指示器
//     const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
//     const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
//     const int CURLY_BRACE_INDICATOR = 10; // 花括号 {}
//     const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

//     // 清除所有括号指示器
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, ROUND_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, SQUARE_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, CURLY_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, ANGLE_BRACE_INDICATOR);

//     // 使用栈来匹配括号对
//     QStack<QPair<int, int>> roundBraceStack;  // 圆括号栈 (行, 列)
//     QStack<QPair<int, int>> squareBraceStack; // 方括号栈
//     QStack<QPair<int, int>> curlyBraceStack;  // 花括号栈
//     QStack<QPair<int, int>> angleBraceStack;  // 尖括号栈

//     // 为嵌套的花括号准备不同的颜色
//     QList<QColor> curlyBraceColors;
//     curlyBraceColors << QColor("#569CD6") // 蓝色
//                      << QColor("#4EC9B0") // 青绿色
//                      << QColor("#CE9178") // 橙色
//                      << QColor("#C586C0") // 紫色
//                      << QColor("#DCDCAA") // 黄色
//                      << QColor("#9CDCFE"); // 浅蓝色

//     // 记录每对花括号的嵌套级别
//     QMap<QPair<int, int>, int> curlyBraceLevel;
//     int currentCurlyLevel = 0;

//     // 遍历文本中的每个字符
//     for (int line = 0; line < editor->lines(); line++) {
//         QString lineText = editor->text(line);

//         for (int col = 0; col < lineText.length(); col++) {
//             QChar ch = lineText.at(col);

//             // 处理圆括号
//             if (ch == '(') {
//                 roundBraceStack.push(qMakePair(line, col));
//             } else if (ch == ')') {
//                 if (!roundBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = roundBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ROUND_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, ROUND_BRACE_INDICATOR);
//                 }
//             }

//             // 处理方括号
//             if (ch == '[') {
//                 squareBraceStack.push(qMakePair(line, col));
//             } else if (ch == ']') {
//                 if (!squareBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = squareBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, SQUARE_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, SQUARE_BRACE_INDICATOR);
//                 }
//             }

//             // 处理花括号 - 特别处理嵌套级别
//             if (ch == '{') {
//                 QPair<int, int> bracePair = qMakePair(line, col);
//                 curlyBraceStack.push(bracePair);
//                 // 记录当前花括号的嵌套级别
//                 curlyBraceLevel[bracePair] = currentCurlyLevel;
//                 currentCurlyLevel++;
//             } else if (ch == '}') {
//                 if (!curlyBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = curlyBraceStack.pop();
//                     // 获取这对花括号的嵌套级别
//                     int level = curlyBraceLevel[openBrace];
//                     currentCurlyLevel--;

//                     // 根据嵌套级别选择颜色
//                     QColor braceColor = curlyBraceColors[level % curlyBraceColors.size()];

//                     // 设置当前指示器的颜色
//                     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, CURLY_BRACE_INDICATOR);
//                     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, CURLY_BRACE_INDICATOR,
//                                           (braceColor.red() << 16) | (braceColor.green() << 8) | braceColor.blue());

//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, CURLY_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, CURLY_BRACE_INDICATOR);
//                 }
//             }

//             // 处理尖括号
//             if (ch == '<') {
//                 angleBraceStack.push(qMakePair(line, col));
//             } else if (ch == '>') {
//                 if (!angleBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = angleBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ANGLE_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, ANGLE_BRACE_INDICATOR);
//                 }
//             }
//         }
//     }
// }


// void CodeEditor::highlightBraces(QsciScintilla* editor)
// {
//     // 获取编辑器文本
//     QString text = editor->text();

//     // 定义不同类型的括号指示器
//     const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
//     const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
//     const int CURLY_BRACE_INDICATOR = 10; // 花括号 {}
//     const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

//     // 清除所有括号指示器
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, ROUND_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, SQUARE_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, CURLY_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, ANGLE_BRACE_INDICATOR);

//     // 使用栈来匹配括号对
//     QStack<QPair<int, int>> roundBraceStack;  // 圆括号栈 (行, 列)
//     QStack<QPair<int, int>> squareBraceStack; // 方括号栈
//     QStack<QPair<int, int>> curlyBraceStack;  // 花括号栈
//     QStack<QPair<int, int>> angleBraceStack;  // 尖括号栈

//     // 为嵌套的花括号准备不同的颜色 - 使用更鲜明的颜色
//     QList<QColor> curlyBraceColors;
//     curlyBraceColors << QColor("#569CD6") // 蓝色
//                      << QColor("#4EC9B0") // 青绿色
//                      << QColor("#CE9178") // 橙色
//                      << QColor("#C586C0") // 紫色
//                      << QColor("#DCDCAA") // 黄色
//                      << QColor("#9CDCFE"); // 浅蓝色

//     // 记录每对花括号的嵌套级别和对应的颜色
//     QMap<QPair<int, int>, int> curlyBraceLevel;
//     QMap<QPair<int, int>, QColor> curlyBraceColor;
//     int currentCurlyLevel = 0;

//     // 遍历文本中的每个字符
//     for (int line = 0; line < editor->lines(); line++) {
//         QString lineText = editor->text(line);

//         for (int col = 0; col < lineText.length(); col++) {
//             QChar ch = lineText.at(col);

//             // 处理圆括号
//             if (ch == '(') {
//                 roundBraceStack.push(qMakePair(line, col));
//             } else if (ch == ')') {
//                 if (!roundBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = roundBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ROUND_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, ROUND_BRACE_INDICATOR);
//                 }
//             }

//             // 处理方括号
//             if (ch == '[') {
//                 squareBraceStack.push(qMakePair(line, col));
//             } else if (ch == ']') {
//                 if (!squareBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = squareBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, SQUARE_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, SQUARE_BRACE_INDICATOR);
//                 }
//             }

//             // 处理花括号 - 特别处理嵌套级别
//             if (ch == '{') {
//                 QPair<int, int> bracePair = qMakePair(line, col);
//                 curlyBraceStack.push(bracePair);

//                 // 记录当前花括号的嵌套级别
//                 curlyBraceLevel[bracePair] = currentCurlyLevel;

//                 // 根据嵌套级别选择颜色
//                 QColor braceColor = curlyBraceColors[currentCurlyLevel % curlyBraceColors.size()];
//                 curlyBraceColor[bracePair] = braceColor;

//                 // 为当前花括号设置颜色
//                 int colorValue = (braceColor.red() << 16) | (braceColor.green() << 8) | braceColor.blue();

//                 // 创建一个临时指示器用于这对花括号
//                 int tempIndicator = CURLY_BRACE_INDICATOR + currentCurlyLevel + 1;
//                 if (tempIndicator > 30) tempIndicator = 30; // 避免超出指示器范围

//                 // 设置指示器样式
//                 editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, tempIndicator, QsciScintilla::INDIC_TEXTFORE);
//                 editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, tempIndicator, colorValue);

//                 // 高亮开括号
//                 editor->fillIndicatorRange(line, col, line, col + 1, tempIndicator);

//                 // 增加嵌套级别
//                 currentCurlyLevel++;
//             } else if (ch == '}') {
//                 if (!curlyBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = curlyBraceStack.pop();

//                     // 获取这对花括号的嵌套级别
//                     int level = curlyBraceLevel[openBrace];

//                     // 获取这对花括号的颜色
//                     QColor braceColor = curlyBraceColor[openBrace];
//                     int colorValue = (braceColor.red() << 16) | (braceColor.green() << 8) | braceColor.blue();

//                     // 创建一个临时指示器用于这对花括号
//                     int tempIndicator = CURLY_BRACE_INDICATOR + level + 1;
//                     if (tempIndicator > 30) tempIndicator = 30; // 避免超出指示器范围

//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, tempIndicator);

//                     // 减少嵌套级别
//                     currentCurlyLevel--;
//                 }
//             }

//             // 处理尖括号
//             if (ch == '<') {
//                 angleBraceStack.push(qMakePair(line, col));
//             } else if (ch == '>') {
//                 if (!angleBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = angleBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ANGLE_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, ANGLE_BRACE_INDICATOR);
//                 }
//             }
//         }
//     }
// }


// void CodeEditor::highlightBraces(QsciScintilla* editor)
// {
//     // 获取编辑器文本
//     QString text = editor->text();

//     // 定义不同类型的括号指示器基础值
//     const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
//     const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
//     const int CURLY_BRACE_BASE_INDICATOR = 12; // 花括号 {} 基础指示器值
//     const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

//     // 清除所有括号指示器
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, ROUND_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, SQUARE_BRACE_INDICATOR);
//     editor->clearIndicatorRange(0, 0, editor->lines(), 0, ANGLE_BRACE_INDICATOR);

//     // 清除所有花括号指示器 (12-20)
//     for (int i = 0; i < 8; i++) {
//         editor->clearIndicatorRange(0, 0, editor->lines(), 0, CURLY_BRACE_BASE_INDICATOR + i);
//     }

//     // 使用栈来匹配括号对
//     QStack<QPair<int, int>> roundBraceStack;  // 圆括号栈 (行, 列)
//     QStack<QPair<int, int>> squareBraceStack; // 方括号栈
//     QStack<QPair<int, int>> curlyBraceStack;  // 花括号栈
//     QStack<QPair<int, int>> angleBraceStack;  // 尖括号栈

//     // 为嵌套的花括号准备不同的颜色 - 使用更鲜明的颜色
//     QList<QColor> curlyBraceColors;
//     curlyBraceColors << QColor("#569CD6") // 蓝色
//                      << QColor("#4EC9B0") // 青绿色
//                      << QColor("#CE9178") // 橙色
//                      << QColor("#C586C0") // 紫色
//                      << QColor("#DCDCAA") // 黄色
//                      << QColor("#9CDCFE") // 浅蓝色
//                      << QColor("#D16969") // 红色
//                      << QColor("#6A9955"); // 绿色

//     // 为每个嵌套级别预先设置指示器样式和颜色
//     for (int i = 0; i < curlyBraceColors.size(); i++) {
//         int indicatorId = CURLY_BRACE_BASE_INDICATOR + i;
//         QColor color = curlyBraceColors[i];

//         // 设置指示器样式为文本前景色
//         editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, indicatorId, QsciScintilla::INDIC_TEXTFORE);
//         editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, indicatorId,
//                               (color.red() << 16) | (color.green() << 8) | color.blue());
//         editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, indicatorId, 255);
//     }

//     // 记录每对花括号的嵌套级别
//     int currentCurlyLevel = 0;

//     // 遍历文本中的每个字符
//     for (int line = 0; line < editor->lines(); line++) {
//         QString lineText = editor->text(line);

//         for (int col = 0; col < lineText.length(); col++) {
//             QChar ch = lineText.at(col);

//             // 处理圆括号
//             if (ch == '(') {
//                 roundBraceStack.push(qMakePair(line, col));
//             } else if (ch == ')') {
//                 if (!roundBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = roundBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ROUND_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, ROUND_BRACE_INDICATOR);
//                 }
//             }

//             // 处理方括号
//             if (ch == '[') {
//                 squareBraceStack.push(qMakePair(line, col));
//             } else if (ch == ']') {
//                 if (!squareBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = squareBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, SQUARE_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, SQUARE_BRACE_INDICATOR);
//                 }
//             }

//             // 处理花括号 - 特别处理嵌套级别
//             if (ch == '{') {
//                 // 保存当前位置和嵌套级别
//                 QPair<int, int> bracePair = qMakePair(line, col);
//                 int level = currentCurlyLevel % curlyBraceColors.size();

//                 // 将当前位置和嵌套级别压入栈
//                 curlyBraceStack.push(bracePair);

//                 // 使用对应嵌套级别的指示器
//                 int indicatorId = CURLY_BRACE_BASE_INDICATOR + level;

//                 // 高亮开括号
//                 editor->fillIndicatorRange(line, col, line, col + 1, indicatorId);

//                 // 增加嵌套级别
//                 currentCurlyLevel++;
//             } else if (ch == '}') {
//                 if (!curlyBraceStack.isEmpty()) {
//                     // 减少嵌套级别
//                     currentCurlyLevel--;

//                     // 获取对应的开括号位置
//                     QPair<int, int> openBrace = curlyBraceStack.pop();

//                     // 计算这对花括号的嵌套级别
//                     int level = currentCurlyLevel % curlyBraceColors.size();

//                     // 使用对应嵌套级别的指示器
//                     int indicatorId = CURLY_BRACE_BASE_INDICATOR + level;

//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, indicatorId);
//                 }
//             }

//             // 处理尖括号
//             if (ch == '<') {
//                 angleBraceStack.push(qMakePair(line, col));
//             } else if (ch == '>') {
//                 if (!angleBraceStack.isEmpty()) {
//                     QPair<int, int> openBrace = angleBraceStack.pop();
//                     // 高亮开括号
//                     editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ANGLE_BRACE_INDICATOR);
//                     // 高亮闭括号
//                     editor->fillIndicatorRange(line, col, line, col + 1, ANGLE_BRACE_INDICATOR);
//                 }
//             }
//         }
//     }
// }


void CodeEditor::highlightBraces(QsciScintilla* editor)
{
    // 获取编辑器文本
    QString text = editor->text();

    // 定义不同类型的括号指示器基础值
    const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
    const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
    const int CURLY_BRACE_BASE_INDICATOR = 12; // 花括号 {} 基础指示器值
    const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

    // 清除所有括号指示器
    editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(), ROUND_BRACE_INDICATOR);
    editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(), SQUARE_BRACE_INDICATOR);
    editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(), ANGLE_BRACE_INDICATOR);

    // 清除所有花括号指示器 (12-20) - 确保完全清除
    for (int i = 0; i < 8; i++) {
        int indicatorId = CURLY_BRACE_BASE_INDICATOR + i;
        editor->clearIndicatorRange(0, 0, editor->lines(), editor->text().length(), indicatorId);
    }

    // 使用栈来匹配括号对
    QStack<QPair<int, int>> roundBraceStack;  // 圆括号栈 (行, 列)
    QStack<QPair<int, int>> squareBraceStack; // 方括号栈
    QStack<QPair<int, int>> curlyBraceStack;  // 花括号栈
    QStack<QPair<int, int>> angleBraceStack;  // 尖括号栈

    // 为嵌套的花括号准备不同的颜色 - 使用更鲜明的颜色
    QList<QColor> curlyBraceColors;
    curlyBraceColors << QColor("#569CD6") // 蓝色
                     << QColor("#4EC9B0") // 青绿色
                     << QColor("#CE9178") // 橙色
                     << QColor("#C586C0") // 紫色
                     << QColor("#DCDCAA") // 黄色
                     << QColor("#9CDCFE") // 浅蓝色
                     << QColor("#D16969") // 红色
                     << QColor("#6A9955"); // 绿色

    // 为每个嵌套级别预先设置指示器样式和颜色 - 确保每个指示器都有独特的样式
    for (int i = 0; i < curlyBraceColors.size(); i++) {
        int indicatorId = CURLY_BRACE_BASE_INDICATOR + i;
        QColor color = curlyBraceColors[i];

        // 设置指示器样式为文本前景色 - 使用更明显的样式
        editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, indicatorId, QsciScintilla::INDIC_TEXTFORE);
        editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, indicatorId,
                              (color.red() << 16) | (color.green() << 8) | color.blue());
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
                    editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ROUND_BRACE_INDICATOR);
                    // 高亮闭括号
                    editor->fillIndicatorRange(line, col, line, col + 1, ROUND_BRACE_INDICATOR);
                }
            }

            // 处理方括号
            if (ch == '[') {
                squareBraceStack.push(qMakePair(line, col));
            } else if (ch == ']') {
                if (!squareBraceStack.isEmpty()) {
                    QPair<int, int> openBrace = squareBraceStack.pop();
                    // 高亮开括号
                    editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, SQUARE_BRACE_INDICATOR);
                    // 高亮闭括号
                    editor->fillIndicatorRange(line, col, line, col + 1, SQUARE_BRACE_INDICATOR);
                }
            }

            // 处理花括号 - 特别处理嵌套级别
            if (ch == '{') {
                // 保存当前位置
                QPair<int, int> bracePair = qMakePair(line, col);

                // 计算当前嵌套级别对应的指示器ID
                int level = currentCurlyLevel % curlyBraceColors.size();
                int indicatorId = CURLY_BRACE_BASE_INDICATOR + level;

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
                    editor->fillIndicatorRange(openBrace.first, openBrace.second, openBrace.first, openBrace.second + 1, ANGLE_BRACE_INDICATOR);
                    // 高亮闭括号
                    editor->fillIndicatorRange(line, col, line, col + 1, ANGLE_BRACE_INDICATOR);
                }
            }
        }
    }
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
