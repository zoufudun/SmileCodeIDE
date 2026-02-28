#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QLineEdit>
#include <QProcess>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class TerminalWidget : public QWidget {
  Q_OBJECT
public:
  explicit TerminalWidget(const QString &workingDir = "",
                          QWidget *parent = nullptr);
  ~TerminalWidget();

private slots:
  void executeCommand();
  void processOutput();
  void processError();

private:
  void startShell();
  QString ansiToHtml(const QString &text);

  QProcess *m_process;
  QTextEdit *m_outputView;
  QLineEdit *m_inputLine;
  QString m_workingDir;
  QByteArray m_buffer; // Buffer for partial UTF-8 sequences
};

#endif // TERMINALWIDGET_H
