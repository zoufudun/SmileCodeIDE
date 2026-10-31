#include "codeeditor.h"
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSettings>

namespace {
QString functionTemplate() { return "/**\n * @brief ${brief}\n${param}${return} */\n"; }
QString fileTemplate() { return "/**\n * @file ${file}\n * @brief ${brief}\n * @author ${author}\n * @date ${date}\n */\n"; }
}

QList<QAction *> CodeEditor::editorActions() const { return m_editorActions; }
QAction *CodeEditor::editorAction(const QString &name) const {
    for (auto action : m_editorActions) if (action->objectName() == name) return action;
    return nullptr;
}

void CodeEditor::initializeEditorCommands() {
    auto command = [this](const char *name, const QString &label, const QString &shortcut,
                           const QString &group, auto handler) {
        auto action = new QAction(label, this);
        action->setObjectName(QString::fromLatin1(name));
        action->setShortcut(QKeySequence(shortcut));
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        action->setProperty("commandGroup", group);
        connect(action, &QAction::triggered, this, handler);
        addAction(action);
        m_editorActions.append(action);
        return action;
    };
    const QString navigation = tr("导航"), edit = tr("编辑"), search = tr("搜索"), view = tr("视图");
    command("navigateBack", tr("后退"), "Alt+Left", navigation, [this] { replayNavigation(-1); });
    command("navigateForward", tr("前进"), "Alt+Right", navigation, [this] { replayNavigation(1); });
    command("goToDefinition", tr("转到定义"), "F12", navigation, &CodeEditor::gotoDefinitionAtCursor);
    command("findReferences", tr("查找引用（源码扫描）"), "Shift+F12", navigation, &CodeEditor::findReferencesAtCursor);
    command("switchSourceHeader", tr("切换头文件 / 源文件"), "Alt+O", navigation, [this] {
        const QFileInfo current(m_currentFilePath);
        QStringList extensions = {"h", "hpp", "hh", "c", "cpp", "cc", "cxx"};
        const bool header = QStringList{"h", "hpp", "hh"}.contains(current.suffix().toLower());
        QStringList candidates = getAllProjectSourceFiles();
        for (const auto &extension : extensions) candidates.prepend(current.dir().filePath(current.completeBaseName() + "." + extension));
        for (const auto &path : candidates) {
            QFileInfo f(path);
            if (f.exists() && f.completeBaseName() == current.completeBaseName() &&
                QStringList{"h", "hpp", "hh"}.contains(f.suffix().toLower()) != header) {
                openFileAndGoTo(path, 1, 1); return;
            }
        }
        QMessageBox::information(this, tr("切换文件"), tr("未找到同名头文件或源文件。"));
    });
    command("undo", tr("撤销"), "Ctrl+Z", edit, [this] { if (m_currentEditor) m_currentEditor->undo(); });
    command("redo", tr("重做"), "Ctrl+Y", edit, [this] { if (m_currentEditor) m_currentEditor->redo(); });
    command("cut", tr("剪切"), "Ctrl+X", edit, [this] { if (m_currentEditor) m_currentEditor->cut(); });
    command("copy", tr("复制"), "Ctrl+C", edit, [this] { if (m_currentEditor) m_currentEditor->copy(); });
    command("paste", tr("粘贴"), "Ctrl+V", edit, [this] { if (m_currentEditor) m_currentEditor->paste(); });
    command("selectAll", tr("全选"), "Ctrl+A", edit, [this] { if (m_currentEditor) m_currentEditor->selectAll(); });
    command("renameSymbol", tr("重命名符号"), "F2", edit, &CodeEditor::renameSymbolAtCursor);
    command("indentSelection", tr("增加缩进"), "Ctrl+]", edit, [this] { transformSelectedLines(false); });
    command("unindentSelection", tr("减少缩进"), "Ctrl+[", edit, [this] { transformSelectedLines(true); });
    command("toggleLineComment", tr("切换行注释"), "Ctrl+/", edit, [this] { transformSelectedLines(false, true); });
    command("toggleBlockComment", tr("切换块注释"), "Ctrl+Shift+/", edit, &CodeEditor::toggleBlockComment);
    command("insertDoxygenComment", tr("插入 Doxygen 函数注释"), "", edit, [this] { insertDoxygen(false); });
    command("insertDoxygenFileComment", tr("插入 Doxygen 文件注释"), "", edit, [this] { insertDoxygen(true); });
    command("editDoxygenTemplates", tr("编辑 Doxygen 模板…"), "", edit, &CodeEditor::editDoxygenTemplates);
    command("find", tr("查找"), "Ctrl+F", search, [this] { showFindReplaceBar(false); });
    command("replace", tr("替换"), "Ctrl+H", search, [this] { showFindReplaceBar(true); });
    command("findInProject", tr("在项目中查找"), "Ctrl+Shift+F", search, [this] { emit projectSearchRequested(); });
    command("toggleOutline", tr("显示 / 隐藏符号大纲"), "", view, &CodeEditor::toggleFunctionOutline);
    command("splitHorizontal", tr("左右分栏"), "", view, [this] { createSplitView(Qt::Horizontal); });
    command("splitVertical", tr("上下分栏"), "", view, [this] { createSplitView(Qt::Vertical); });
    command("closeSplit", tr("关闭当前分栏"), "", view, &CodeEditor::closeSplitView);
    updateNavigationActions();
}

void CodeEditor::createEditorMenus(QMenuBar *bar) {
    if (!bar) return;
    QMap<QString, QMenu *> menus;
    auto findExistingMenu = [bar](const QString &title) -> QMenu * {
        for (auto action : bar->actions()) {
            if (auto m = action->menu()) {
                if (m->title().remove("&").trimmed() == title) return m;
            }
        }
        return nullptr;
    };
    for (auto action : m_editorActions) {
        const QString group = action->property("commandGroup").toString();
        if (!menus.contains(group)) {
            QMenu *target = findExistingMenu(group);
            if (!target) target = bar->addMenu(group);
            menus[group] = target;
        }
        menus[group]->addAction(action);
    }
}

void CodeEditor::transformSelectedLines(bool unindent, bool comment) {
    auto editor = m_currentEditor;
    if (!editor || editor->isReadOnly()) return;
    int first, start, last, end;
    bool selection = editor->hasSelectedText();
    if (selection) editor->getSelection(&first, &start, &last, &end);
    else { editor->getCursorPosition(&first, &start); last = first; end = start; }
    if (last > first && end == 0) --last;
    bool uncomment = comment;
    if (comment) for (int line = first; line <= last; ++line) {
        QString text = editor->text(line).trimmed();
        if (!text.isEmpty() && !text.startsWith("//")) uncomment = false;
    }
    editor->beginUndoAction();
    for (int line = last; line >= first; --line) {
        QString text = editor->text(line);
        int whitespace = 0;
        while (whitespace < text.size() && (text[whitespace] == ' ' || text[whitespace] == '\t')) ++whitespace;
        if (comment) {
            if (text.trimmed().isEmpty()) continue;
            editor->setSelection(line, whitespace, line, whitespace + (uncomment ? 2 : 0));
            editor->replaceSelectedText(uncomment ? "" : "//");
        } else if (unindent) {
            int remove = text.startsWith('\t') ? 1 : qMin(whitespace, qMax(1, editor->indentationWidth()));
            editor->setSelection(line, 0, line, remove);
            editor->replaceSelectedText("");
        } else {
            editor->setSelection(line, 0, line, 0);
            editor->replaceSelectedText(QString(qMax(1, editor->indentationWidth()), ' '));
        }
    }
    editor->endUndoAction();
    if (selection) {
        if (last + 1 < editor->lines()) editor->setSelection(first, 0, last + 1, 0);
        else editor->setSelection(first, 0, last, editor->text(last).trimmed().size());
    } else editor->setCursorPosition(first, start);
}

void CodeEditor::toggleBlockComment() {
    auto editor = m_currentEditor;
    if (!editor || editor->isReadOnly()) return;
    if (!editor->hasSelectedText()) {
        int line, column; editor->getCursorPosition(&line, &column);
        QString text = editor->text(line); while (text.endsWith('\n') || text.endsWith('\r')) text.chop(1);
        editor->setSelection(line, 0, line, text.size());
    }
    int first, start, last, end; editor->getSelection(&first, &start, &last, &end);
    QString selected = editor->selectedText();
    if (selected.startsWith("/*") && selected.endsWith("*/")) selected = selected.mid(2, selected.size() - 4);
    else {
        if (selected.contains("*/")) { QMessageBox::information(this, tr("块注释"), tr("选择内容包含块注释结束符，无法安全包裹。")); return; }
        selected = "/*" + selected + "*/";
    }
    editor->beginUndoAction(); editor->replaceSelectedText(selected); editor->endUndoAction();
    QStringList lines = selected.split('\n');
    editor->setSelection(first, start, first + lines.size() - 1, lines.size() == 1 ? start + selected.size() : lines.last().size());
}

bool CodeEditor::confirmDiscardChanges() {
    if (!m_currentEditor || !m_currentEditor->isModified()) return true;
    const auto answer = QMessageBox::question(this, tr("保存更改"), tr("当前文件有未保存的更改。是否保存后继续？"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer == QMessageBox::Discard) return true;
    if (answer != QMessageBox::Save) return false;
    QString path = m_currentFilePath;
    if (path.isEmpty()) path = QFileDialog::getSaveFileName(this, tr("保存文件"));
    if (path.isEmpty()) return false;
    if (saveFile(path)) return true;
    QMessageBox::warning(this, tr("保存失败"), tr("无法保存文件：%1").arg(path));
    return false;
}

CodeEditor::NavigationPosition CodeEditor::navigationPosition() const {
    NavigationPosition position; position.path = m_currentFilePath;
    if (m_currentEditor) m_currentEditor->getCursorPosition(&position.line, &position.column);
    return position;
}
void CodeEditor::recordNavigation(const NavigationPosition &origin) {
    const auto destination = navigationPosition();
    if (origin.path == destination.path && origin.line == destination.line && origin.column == destination.column) return;
    if (m_navigationIndex >= 0) {
        m_navigationHistory[m_navigationIndex] = origin;
        while (m_navigationHistory.size() > m_navigationIndex + 1) m_navigationHistory.removeLast();
    } else if (!origin.path.isEmpty()) m_navigationHistory.append(origin);
    m_navigationHistory.append(destination);
    m_navigationIndex = m_navigationHistory.size() - 1;
    updateNavigationActions();
}
void CodeEditor::updateNavigationActions() {
    if (auto a = editorAction("navigateBack")) a->setEnabled(m_navigationIndex > 0);
    if (auto a = editorAction("navigateForward")) a->setEnabled(m_navigationIndex >= 0 && m_navigationIndex + 1 < m_navigationHistory.size());
}
void CodeEditor::replayNavigation(int offset) {
    const int next = m_navigationIndex + offset;
    if (next < 0 || next >= m_navigationHistory.size()) return;
    const auto origin = navigationPosition();
    const auto target = m_navigationHistory[next];
    m_replayingHistory = true;
    if (openFile(target.path)) {
        m_navigationHistory[m_navigationIndex] = origin;
        m_navigationIndex = next;
        m_currentEditor->setCursorPosition(target.line, target.column);
        m_currentEditor->ensureCursorVisible();
        updateBreadcrumb(target.line);
    }
    m_replayingHistory = false;
    updateNavigationActions();
}

void CodeEditor::insertDoxygen(bool fileComment) {
    if (!m_currentEditor || m_currentEditor->isReadOnly()) return;
    QSettings settings("PhudonTools", "Settings");
    int line, column; m_currentEditor->getCursorPosition(&line, &column);
    QString parameters, returns;
    if (!fileComment) {
        QString declaration;
        for (int n = line; n < qMin(m_currentEditor->lines(), line + 30); ++n) {
            declaration += m_currentEditor->text(n);
            if (declaration.contains(')') || declaration.contains('{') || declaration.contains(';')) break;
        }
        auto match = QRegularExpression(R"(^\s*([\w\s:*&<>~]+?)\s+([\w:~]+)\s*\(([^()]*)\)\s*(?:const\s*)?(?:noexcept\s*)?[;{]?)").match(declaration);
        if (!match.hasMatch()) {
            QMessageBox::information(this, tr("Doxygen 注释"), tr("无法识别当前函数声明。请将光标放在声明首行；复杂函数指针或宏声明请手动编写注释。")); return;
        }
        const QString args = match.captured(3).trimmed();
        if (!args.isEmpty() && args != "void") for (QString argument : args.split(',')) {
            argument = argument.section('=', 0, 0).trimmed();
            if (argument.isEmpty() || argument == "void" || argument.endsWith('*') || argument.endsWith('&')) continue;
            QString clean = argument;
            static const QRegularExpression prefixRe(R"(^(?:(?:const|volatile|struct|class|enum)\s+)+)");
            clean.remove(prefixRe);
            clean = clean.trimmed();
            auto name = QRegularExpression(R"((?:^|\s|[*&])([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?$)").match(clean);
            if (name.hasMatch()) {
                QString paramName = name.captured(1);
                QString typePart = clean.left(name.capturedStart(1)).trimmed();
                static const QSet<QString> typeKeywords = {
                    "void", "int", "char", "long", "short", "double", "float",
                    "unsigned", "signed", "bool", "const", "auto", "uint8_t",
                    "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t",
                    "int32_t", "int64_t", "size_t"
                };
                if (!typeKeywords.contains(paramName) && !typePart.isEmpty()) {
                    parameters += " * @param " + paramName + " \n";
                }
            }
        }
        if (match.captured(1).trimmed() != "void") returns = " * @return \n";
    }
    QString comment = settings.value(fileComment ? "Doxygen/fileTemplate" : "Doxygen/functionTemplate", fileComment ? fileTemplate() : functionTemplate()).toString();
    comment.replace("${brief}", "").replace("${param}", parameters).replace("${return}", returns)
           .replace("${file}", QFileInfo(m_currentFilePath).fileName()).replace("${author}", settings.value("Doxygen/author", "").toString())
           .replace("${date}", QDate::currentDate().toString(Qt::ISODate));
    if (!comment.endsWith('\n')) comment += '\n';
    if (fileComment) line = 0;
    QString indentation = QRegularExpression("^[ \\t]*").match(m_currentEditor->text(line)).captured();
    QStringList lines = comment.split('\n');
    for (int n = 0; n + 1 < lines.size(); ++n) lines[n].prepend(indentation);
    m_currentEditor->beginUndoAction(); m_currentEditor->insertAt(lines.join('\n'), line, 0); m_currentEditor->endUndoAction();
}

void CodeEditor::editDoxygenTemplates() {
    QSettings settings("PhudonTools", "Settings");
    QDialog dialog(this); dialog.setWindowTitle(tr("Doxygen 模板")); dialog.resize(620, 530);
    auto layout = new QFormLayout(&dialog);
    auto function = new QPlainTextEdit(settings.value("Doxygen/functionTemplate", functionTemplate()).toString(), &dialog);
    auto file = new QPlainTextEdit(settings.value("Doxygen/fileTemplate", fileTemplate()).toString(), &dialog);
    auto author = new QPlainTextEdit(settings.value("Doxygen/author").toString(), &dialog); author->setMaximumHeight(50);
    layout->addRow(tr("占位符：${brief}、${param}、${return}、${file}、${author}、${date}\n${param} 和 ${return} 展开为完整注释行。"), new QLabel(&dialog));
    layout->addRow(tr("函数模板"), function); layout->addRow(tr("文件模板"), file); layout->addRow(tr("作者"), author);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog); layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        settings.setValue("Doxygen/functionTemplate", function->toPlainText()); settings.setValue("Doxygen/fileTemplate", file->toPlainText()); settings.setValue("Doxygen/author", author->toPlainText());
    }
}
