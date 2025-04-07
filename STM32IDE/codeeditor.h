/*
 * @Description: 
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-05 21:43:38
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-04-07 21:55:44
 */
#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QWidget>
#include <QSplitter>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qsciapis.h>
#include <QSet>
#include <QDebug>
#include <QToolBar>  // 添加工具栏头文件

class CodeEditor : public QWidget
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor();

    // 获取当前编辑器
    QsciScintilla* currentEditor() const;
    
    // 获取所有编辑器
    QList<QsciScintilla*> allEditors() const;
    
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

public slots:
    // 更新变量列表用于自动补全
    void updateVariableList();

private slots:
    // 当前编辑器变更
    void onEditorChanged(QsciScintilla* editor);

private:
    // 设置编辑器基本属性
    void setupEditor(QsciScintilla* editor);
    
    // 设置自动补全
    void setupAutoCompletion(QsciScintilla* editor);
    
    // 创建工具栏
    void createToolBar();
    
    // 主分割器
    QSplitter* m_mainSplitter;
    
    // 当前活动的编辑器
    QsciScintilla* m_currentEditor;
    
    // 所有编辑器列表
    QList<QsciScintilla*> m_editors;
    
    // C++词法分析器
    QsciLexerCPP* m_lexerCPP;
    
    // API对象用于自动补全
    QsciAPIs* m_apiCPP;
    
    // 当前文件路径
    QString m_currentFilePath;
    
    // 工具栏
    QToolBar* m_toolBar;

    // 在signals部分添加以下信号
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

