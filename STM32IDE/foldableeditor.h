#ifndef FOLDABLEEDITOR_H
#define FOLDABLEEDITOR_H

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QMap>

class LineNumberArea;

class FoldableEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit FoldableEditor(QWidget *parent = nullptr);
    
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    
    // 代码折叠相关方法
    void setFoldingEnabled(bool enabled);
    bool isFoldingEnabled() const;
    bool isFoldable(int line) const;
    void toggleFold(int line);
    bool isFolded(int line) const;
    
    // 设置语法高亮
    void setSyntaxHighlighter(QSyntaxHighlighter *highlighter);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override; // Add this line to declare the event method

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
    QSyntaxHighlighter *m_highlighter;
    bool m_foldingEnabled;
    QMap<int, bool> m_foldedBlocks; // 存储已折叠的代码块 <行号, 是否折叠>
    
    // 查找代码块的结束位置
    int findBlockEnd(int startLine) const;
    
    friend class LineNumberArea;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(FoldableEditor *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }
    
    void mousePressEvent(QMouseEvent *event) override;

private:
    FoldableEditor *codeEditor;
};

#endif // FOLDABLEEDITOR_H
