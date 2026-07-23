#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QAction>
#include <Qsci/qsciscintilla.h>

class EditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorWidget(QWidget *parent = nullptr);
    ~EditorWidget();

    QsciScintilla* editor() const { return m_editor; }
    void setupEditor(QsciLexer *lexer = nullptr);
    bool isLastEditor() const;

signals:
    void splitHorizontally(EditorWidget *editor);
    void splitVertically(EditorWidget *editor);
    void closeEditor(EditorWidget *editor);

private slots:
    void onSplitHorizontally();
    void onSplitVertically();
    void onCloseEditor();
    void updateActions();

private:
    QVBoxLayout *m_layout;
    QToolBar *m_toolbar;
    QsciScintilla *m_editor;
    
    QAction *m_splitHAction;
    QAction *m_splitVAction;
    QAction *m_closeAction;
};

#endif // EDITORWIDGET_H
