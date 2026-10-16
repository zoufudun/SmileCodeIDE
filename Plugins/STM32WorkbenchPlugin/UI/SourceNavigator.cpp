#include "SourceNavigator.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFontDatabase>
#include <QLabel>
#include <QMessageBox>
#include <QMetaMethod>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCursor>
#include <QVBoxLayout>

bool SourceNavigator::open(QWidget *parent, QWidget *host, const QString &filePath,
                           int line, int column) {
    if (host) {
        const int index = host->metaObject()->indexOfMethod("openSourceLocation(QString,int,int)");
        if (index >= 0) {
            const QMetaMethod method = host->metaObject()->method(index);
            if (method.returnMetaType().id() == QMetaType::Bool) {
                bool opened = false;
                if (method.invoke(host, Qt::DirectConnection, Q_RETURN_ARG(bool, opened),
                                  Q_ARG(QString, filePath), Q_ARG(int, line), Q_ARG(int, column))) {
                    return opened;
                }
            }
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto *error = new QMessageBox(QMessageBox::Warning,
                                     QObject::tr("无法打开源文件"),
                                     QObject::tr("无法读取 %1\n%2").arg(filePath, file.errorString()),
                                     QMessageBox::Close, parent);
        error->setObjectName("sourceNavigationError");
        error->setAttribute(Qt::WA_DeleteOnClose);
        error->setWindowModality(Qt::NonModal);
        error->show();
        return false;
    }

    QDialog *dialog = parent ? parent->findChild<QDialog *>(
                                  "sourcePreviewDialog", Qt::FindDirectChildrenOnly) : nullptr;
    if (!parent) {
        for (QWidget *window : QApplication::topLevelWidgets()) {
            if (!window->parentWidget() && window->objectName() == "sourcePreviewDialog") {
                dialog = qobject_cast<QDialog *>(window);
                break;
            }
        }
    }
    if (!dialog) {
        dialog = new QDialog(parent);
        dialog->setObjectName("sourcePreviewDialog");
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->resize(900, 650);
        auto *layout = new QVBoxLayout(dialog);
        auto *heading = new QLabel(dialog);
        heading->setObjectName("sourcePreviewHeading");
        heading->setTextFormat(Qt::PlainText);
        heading->setTextInteractionFlags(Qt::TextSelectableByMouse);
        heading->setWordWrap(true);
        layout->addWidget(heading);
        auto *editor = new QPlainTextEdit(dialog);
        editor->setObjectName("sourcePreviewEditor");
        editor->setReadOnly(true);
        editor->setLineWrapMode(QPlainTextEdit::NoWrap);
        editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        layout->addWidget(editor);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
        QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
        layout->addWidget(buttons);
    }
    dialog->setWindowTitle(QObject::tr("源文件预览（只读）— %1").arg(filePath));
    auto *heading = dialog->findChild<QLabel *>("sourcePreviewHeading");
    heading->setText(QObject::tr("%1  —  行 %2，列 %3")
                         .arg(filePath).arg(line).arg(column > 0 ? column : 1));
    auto *editor = dialog->findChild<QPlainTextEdit *>("sourcePreviewEditor");
    editor->setPlainText(QString::fromUtf8(file.readAll()));
    const int row = qBound(0, line - 1, editor->document()->blockCount() - 1);
    const QTextBlock block = editor->document()->findBlockByNumber(row);
    QTextCursor cursor(block);
    cursor.setPosition(block.position() + qBound(0, column - 1, block.length() - 1));
    editor->setTextCursor(cursor);
    QTextEdit::ExtraSelection selectedLine;
    selectedLine.cursor = QTextCursor(block);
    selectedLine.format.setBackground(editor->palette().highlight().color());
    selectedLine.format.setForeground(editor->palette().highlightedText().color());
    selectedLine.format.setProperty(QTextFormat::FullWidthSelection, true);
    editor->setExtraSelections({selectedLine});
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    editor->centerCursor();
    editor->setFocus();
    return true;
}
