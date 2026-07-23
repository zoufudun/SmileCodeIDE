/*
 * @Description: 
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-13 22:09:24
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-04-24 19:30:31
 */
#include "editorwidget.h"
#include <QApplication>
#include <QStyle>
#include <QSplitter>

EditorWidget::EditorWidget(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // 创建工具栏
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(16, 16));
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    
    // 创建动作
    m_splitHAction = m_toolbar->addAction(QIcon(":/icons/split_h.png"), "水平分栏");
    m_splitVAction = m_toolbar->addAction(QIcon(":/icons/split_v.png"), "垂直分栏");
    m_closeAction = m_toolbar->addAction(QIcon(":/icons/close_split.png"), "关闭分栏");
    
    // 如果没有图标资源，使用系统图标
    if (m_splitHAction->icon().isNull()) {
        m_splitHAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton));
    }
    if (m_splitVAction->icon().isNull()) {
        m_splitVAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ToolBarVerticalExtensionButton));
    }
    if (m_closeAction->icon().isNull()) {
        m_closeAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton));
    }
    
    // 连接信号
    connect(m_splitHAction, &QAction::triggered, this, &EditorWidget::onSplitHorizontally);
    connect(m_splitVAction, &QAction::triggered, this, &EditorWidget::onSplitVertically);
    connect(m_closeAction, &QAction::triggered, this, &EditorWidget::onCloseEditor);
    
    // 创建编辑器
    m_editor = new QsciScintilla(this);
    
    // 添加到布局
    m_layout->addWidget(m_toolbar);
    m_layout->addWidget(m_editor);
    
    // 更新动作状态
    updateActions();
}

EditorWidget::~EditorWidget()
{
}

void EditorWidget::setupEditor(QsciLexer *lexer)
{
    if (!m_editor)
        return;
        
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
    
    // 设置词法分析器
    if (lexer) {
        m_editor->setLexer(lexer);
    }
}

bool EditorWidget::isLastEditor() const
{
    // 检查父部件是否是QSplitter，并且只有一个子部件
    QWidget *parent = parentWidget();
    if (QSplitter *splitter = qobject_cast<QSplitter*>(parent)) {
        return splitter->count() == 1;
    }
    return true;
}

void EditorWidget::onSplitHorizontally()
{
    emit splitHorizontally(this);
}

void EditorWidget::onSplitVertically()
{
    emit splitVertically(this);
}

void EditorWidget::onCloseEditor()
{
    emit closeEditor(this);
}

void EditorWidget::updateActions()
{
    // 如果是最后一个编辑器，禁用关闭按钮
    m_closeAction->setVisible(!isLastEditor());
}
