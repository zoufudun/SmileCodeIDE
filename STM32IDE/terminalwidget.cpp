#include "terminalwidget.h"
#include <QDebug>
#include <QDir>
#include <QStringList>

TerminalWidget::TerminalWidget(const QString &workingDir, QWidget *parent)
    : QWidget(parent), m_workingDir(workingDir) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_outputView = new QTextEdit(this);
  m_outputView->setReadOnly(true);
  // Set a monospaced font
  QFont font("Cascadia Code", 10);
  font.setStyleHint(QFont::Monospace);
  // Fallback to Consolas or Courier New if Cascadia Code is missing
  if (!QFontInfo(font).exactMatch()) {
    font.setFamily("Consolas");
  }
  m_outputView->setFont(font);
  // Dark background for terminal feel
  m_outputView->setStyleSheet("background-color: #1E1E1E; color: #D4D4D4;");

  m_inputLine = new QLineEdit(this);
  m_inputLine->setFont(font);
  m_inputLine->setPlaceholderText("Enter command...");
  m_inputLine->setStyleSheet(
      "background-color: #252526; color: #D4D4D4; border: 1px solid #3E3E42;");

  layout->addWidget(m_outputView);
  layout->addWidget(m_inputLine);

  m_process = new QProcess(this);
  m_process->setProcessChannelMode(
      QProcess::MergedChannels); // Merge stdout and stderr

  if (!m_workingDir.isEmpty()) {
    m_process->setWorkingDirectory(m_workingDir);
  }

  connect(m_inputLine, &QLineEdit::returnPressed, this,
          &TerminalWidget::executeCommand);
  connect(m_process, &QProcess::readyReadStandardOutput, this,
          &TerminalWidget::processOutput);
  connect(m_process, &QProcess::readyReadStandardError, this,
          &TerminalWidget::processError);

  // Start system shell
  startShell();
}

TerminalWidget::~TerminalWidget() {
  if (m_process->state() == QProcess::Running) {
    m_process->kill();
    m_process->waitForFinished(100);
  }
}

void TerminalWidget::startShell() {
  // Priority: pwsh.exe > powershell.exe > cmd.exe
  QStringList shells = {"pwsh.exe", "powershell.exe", "cmd.exe"};

  for (const QString &shell : shells) {
    m_process->start(shell);
    // Force UTF-8 encoding for the shell
    m_process->write("chcp 65001\n");
    m_process->write("cls\n"); // Clear the initial chcp output behavior

    if (!m_workingDir.isEmpty()) {
      m_outputView->append("Terminal started in: " + m_workingDir + " (" +
                           shell + ")");
    } else {
      m_outputView->append("Terminal started (" + shell + ")");
    }
    return;
  }
}

void TerminalWidget::executeCommand() {
  QString command = m_inputLine->text();
  if (command.isEmpty())
    return;

  // Send to process (UTF-8)
  m_process->write((command + "\n").toUtf8());

  m_inputLine->clear();
}

// Basic ANSI to HTML converter
QString TerminalWidget::ansiToHtml(const QString &text) {
  QString html = text.toHtmlEscaped();

  // ANSI color codes
  // Foreground Colors
  html.replace(QRegExp("\033\\[30m"), "<span style='color:#000000;'>"); // Black
  html.replace(QRegExp("\033\\[31m"), "<span style='color:#cd3131;'>"); // Red
  html.replace(QRegExp("\033\\[32m"), "<span style='color:#0dbc79;'>"); // Green
  html.replace(QRegExp("\033\\[33m"),
               "<span style='color:#e5e510;'>"); // Yellow
  html.replace(QRegExp("\033\\[34m"), "<span style='color:#2472c8;'>"); // Blue
  html.replace(QRegExp("\033\\[35m"),
               "<span style='color:#bc3fbc;'>"); // Magenta
  html.replace(QRegExp("\033\\[36m"), "<span style='color:#11a8cd;'>"); // Cyan
  html.replace(QRegExp("\033\\[37m"), "<span style='color:#e5e5e5;'>"); // White

  // Bright/Bold Foreground
  html.replace(QRegExp("\033\\[90m"), "<span style='color:#666666;'>");
  html.replace(QRegExp("\033\\[91m"), "<span style='color:#f14c4c;'>");
  html.replace(QRegExp("\033\\[92m"), "<span style='color:#23d18b;'>");
  html.replace(QRegExp("\033\\[93m"), "<span style='color:#f5f543;'>");
  html.replace(QRegExp("\033\\[94m"), "<span style='color:#3b8eea;'>");
  html.replace(QRegExp("\033\\[95m"), "<span style='color:#d670d6;'>");
  html.replace(QRegExp("\033\\[96m"), "<span style='color:#29b8db;'>");
  html.replace(QRegExp("\033\\[97m"), "<span style='color:#ffffff;'>");

  // 0;3x fallback
  html.replace(QRegExp("\033\\[0;30m"), "<span style='color:#000000;'>");
  html.replace(QRegExp("\033\\[0;31m"), "<span style='color:#cd3131;'>");
  html.replace(QRegExp("\033\\[0;32m"), "<span style='color:#0dbc79;'>");
  html.replace(QRegExp("\033\\[0;33m"), "<span style='color:#e5e510;'>");
  html.replace(QRegExp("\033\\[0;34m"), "<span style='color:#2472c8;'>");
  html.replace(QRegExp("\033\\[0;35m"), "<span style='color:#bc3fbc;'>");
  html.replace(QRegExp("\033\\[0;36m"), "<span style='color:#11a8cd;'>");
  html.replace(QRegExp("\033\\[0;37m"), "<span style='color:#e5e5e5;'>");

  // Reset
  html.replace(QRegExp("\033\\[0m"), "</span>");

  // Replace newlines with <br>
  html.replace("\n", "<br>");

  return html;
}

// Helper to decode UTF-8 incrementally
QString decodeUtf8(QByteArray &buffer) {
  if (buffer.isEmpty())
    return "";
  int len = buffer.size();
  int cutoff = 0;

  // Check last few bytes for partial UTF-8 sequences
  int checkBytes = qMin(len, 4);
  for (int i = 0; i < checkBytes; ++i) {
    unsigned char byte = (unsigned char)buffer.at(len - 1 - i);
    if ((byte & 0x80) == 0)
      break;                     // ASCII, safe
    if ((byte & 0xC0) == 0xC0) { // Lead byte
      int needed = 0;
      if ((byte & 0xE0) == 0xC0)
        needed = 2;
      else if ((byte & 0xF0) == 0xE0)
        needed = 3;
      else if ((byte & 0xF8) == 0xF0)
        needed = 4;

      if (i < needed - 1)
        cutoff = i + 1; // Incomplete
      break;
    }
  }

  QByteArray toDecode = buffer.left(len - cutoff);
  buffer = buffer.right(cutoff); // Keep remainder

  return QString::fromUtf8(toDecode);
}

void TerminalWidget::processOutput() {
  m_buffer.append(m_process->readAllStandardOutput());
  QString text = decodeUtf8(m_buffer);

  if (!text.isEmpty()) {
    m_outputView->moveCursor(QTextCursor::End);
    m_outputView->insertHtml(ansiToHtml(text));
    m_outputView->moveCursor(QTextCursor::End);
  }
}

void TerminalWidget::processError() {
  m_buffer.append(m_process->readAllStandardError());
  QString text = decodeUtf8(m_buffer);

  if (!text.isEmpty()) {
    m_outputView->moveCursor(QTextCursor::End);
    m_outputView->insertHtml(ansiToHtml(text));
    m_outputView->moveCursor(QTextCursor::End);
  }
}
