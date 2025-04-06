/*
 * @Description: 
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-03-29 21:55:10
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-03-29 21:55:19
 */
#include "foldableeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QMouseEvent>

FoldableEditor::FoldableEditor(QWidget *parent)
    : QPlainTextEdit(parent), m_highlighter(nullptr), m_foldingEnabled(true)
{
    lineNumberArea = new LineNumberArea(this);
    
    connect(this, &FoldableEditor::blockCountChanged, this, &FoldableEditor::updateLineNumberAreaWidth);
    connect(this, &FoldableEditor::updateRequest, this, &FoldableEditor::updateLineNumberArea);
    connect(this, &FoldableEditor::cursorPositionChanged, this, &FoldableEditor::highlightCurrentLine);
    
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int FoldableEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    
    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    
    // 如果启用了代码折叠，增加额外的空间用于折叠标记
    if (m_foldingEnabled) {
        space += 15; // 折叠标记的宽度
    }
    
    return space;
}

void FoldableEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void FoldableEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void FoldableEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void FoldableEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        
        QColor lineColor = QColor(Qt::yellow).lighter(160);
        
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    
    setExtraSelections(extraSelections);
}

void FoldableEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);
    
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            
            // 绘制行号
            painter.drawText(0, top, lineNumberArea->width() - 15, fontMetrics().height(),
                             Qt::AlignRight, number);
            
            // 如果启用了代码折叠，绘制折叠标记
            if (m_foldingEnabled) {
                int line = blockNumber + 1;
                if (isFoldable(line)) {
                    // 绘制折叠/展开图标
                    QRect foldRect(lineNumberArea->width() - 15, top, 15, fontMetrics().height());
                    
                    if (isFolded(line)) {
                        // 绘制展开图标 [+]
                        painter.drawRect(foldRect.adjusted(2, 2, -2, -2));
                        painter.drawLine(foldRect.left() + 4, foldRect.center().y(), 
                                         foldRect.right() - 4, foldRect.center().y());
                        painter.drawLine(foldRect.center().x(), foldRect.top() + 4,
                                         foldRect.center().x(), foldRect.bottom() - 4);
                    } else {
                        // 绘制折叠图标 [-]
                        painter.drawRect(foldRect.adjusted(2, 2, -2, -2));
                        painter.drawLine(foldRect.left() + 4, foldRect.center().y(), 
                                         foldRect.right() - 4, foldRect.center().y());
                    }
                }
            }
        }
        
        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void FoldableEditor::setFoldingEnabled(bool enabled)
{
    if (m_foldingEnabled != enabled) {
        m_foldingEnabled = enabled;
        updateLineNumberAreaWidth(0);
        lineNumberArea->update();
    }
}

bool FoldableEditor::isFoldingEnabled() const
{
    return m_foldingEnabled;
}

bool FoldableEditor::isFoldable(int line) const
{
    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    if (!block.isValid())
        return false;
    
    QString text = block.text().trimmed();
    
    // 检查是否是可折叠的代码块开始
    return (text.contains("{") && !text.contains("}")) ||
           (text.contains("if") && text.contains("(") && text.contains(")") && !text.contains(";")) ||
           (text.contains("for") && text.contains("(") && text.contains(")") && !text.contains(";")) ||
           (text.contains("while") && text.contains("(") && text.contains(")") && !text.contains(";"));
}

void FoldableEditor::toggleFold(int line)
{
    if (!m_foldingEnabled)
        return;

    if (isFoldable(line)) {
        if (isFolded(line)) {
            // 取消折叠
            m_foldedBlocks.remove(line);

            // 重新计算文档布局，显示所有文本块
            // document()->documentLayout()->documentChanged(0, 0, 0, document()->characterCount());
            // 使用正确的方法通知文档布局变化
            document()->markContentsDirty(0, document()->characterCount());
        } else {
            // 折叠代码块
            m_foldedBlocks[line] = true;

            // 重新计算文档布局，隐藏折叠的文本块
            // document()->documentLayout()->documentChanged(0, 0, 0, document()->characterCount());
            // 使用正确的方法通知文档布局变化
            document()->markContentsDirty(0, document()->characterCount());
        }

        // 更新视图
        viewport()->update();
        lineNumberArea->update();
    }
}

// 添加一个新方法来处理文档布局
bool FoldableEditor::event(QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest) {
        // 在布局请求时处理折叠的代码块
        QTextBlock block = document()->begin();
        while (block.isValid()) {
            int blockLine = block.blockNumber() + 1;

            // 检查此块是否应该被隐藏（在折叠区域内）
            bool shouldBeHidden = false;
            for (auto foldedLine : m_foldedBlocks.keys()) {
                int endLine = findBlockEnd(foldedLine);
                if (endLine > 0 && blockLine > foldedLine && blockLine <= endLine) {
                    shouldBeHidden = true;
                    break;
                }
            }

            // 使用 QTextBlockFormat 来隐藏/显示文本块
            QTextCursor cursor(block);
            QTextBlockFormat format = block.blockFormat();

            if (shouldBeHidden) {
                // 隐藏块 - 设置高度为0
                format.setLineHeight(0, QTextBlockFormat::FixedHeight);
                cursor.setBlockFormat(format);
            } else {
                // 显示块 - 恢复正常高度
                format.setLineHeight(0, QTextBlockFormat::SingleHeight);
                cursor.setBlockFormat(format);
            }

            block = block.next();
        }
    }

    return QPlainTextEdit::event(event);
}

bool FoldableEditor::isFolded(int line) const
{
    return m_foldedBlocks.contains(line);
}

int FoldableEditor::findBlockEnd(int startLine) const
{
    QTextBlock startBlock = document()->findBlockByLineNumber(startLine - 1);
    if (!startBlock.isValid())
        return -1;
    
    // 查找匹配的大括号
    int braceLevel = 0;
    bool foundOpenBrace = false;
    
    // 检查第一行是否包含左大括号
    QString text = startBlock.text();
    int openPos = text.indexOf('{');
    if (openPos >= 0) {
        foundOpenBrace = true;
        braceLevel = 1;
    }
    
    QTextBlock block = startBlock.next();
    int currentLine = startLine;
    
    while (block.isValid()) {
        text = block.text();
        
        // 如果第一行没有找到左大括号，在后续行中查找
        if (!foundOpenBrace) {
            openPos = text.indexOf('{');
            if (openPos >= 0) {
                foundOpenBrace = true;
                braceLevel = 1;
            }
        } else {
            // 计算大括号的嵌套级别
            for (int i = 0; i < text.length(); ++i) {
                if (text[i] == '{')
                    ++braceLevel;
                else if (text[i] == '}')
                    --braceLevel;
                
                // 找到匹配的右大括号
                if (braceLevel == 0)
                    return currentLine;
            }
        }
        
        block = block.next();
        ++currentLine;
    }
    
    return -1; // 没有找到匹配的右大括号
}

void FoldableEditor::setSyntaxHighlighter(QSyntaxHighlighter *highlighter)
{
    if (m_highlighter)
        delete m_highlighter;
    
    m_highlighter = highlighter;
    if (m_highlighter)
        m_highlighter->setDocument(document());
}

void FoldableEditor::mousePressEvent(QMouseEvent *event)
{
    QPlainTextEdit::mousePressEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    if (codeEditor->m_foldingEnabled) {
        int lineNumber = event->pos().y() / codeEditor->fontMetrics().height() + 1;
        QTextBlock block = codeEditor->document()->findBlockByLineNumber(lineNumber - 1);
        
        if (block.isValid()) {
            // 检查点击位置是否在折叠图标区域
            if (event->pos().x() >= width() - 15) {
                if (codeEditor->isFoldable(lineNumber)) {
                    codeEditor->toggleFold(lineNumber);
                    return;
                }
            }
        }
    }
    
    QWidget::mousePressEvent(event);
}
