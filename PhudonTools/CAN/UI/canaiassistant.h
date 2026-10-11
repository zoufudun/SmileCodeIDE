#ifndef CANAIASSISTANT_H
#define CANAIASSISTANT_H

#include <QWidget>
#include <QLabel>
#include <QTextBrowser>
#include <QLineEdit>
#include <QPushButton>
#include "../CAN/Engine/knowledgeengine.h"

class CanAiAssistant : public QWidget {
  Q_OBJECT
public:
  explicit CanAiAssistant(KnowledgeEngine *knowledge, QWidget *parent = nullptr);
  ~CanAiAssistant() override = default;

  void applyThemeStyle(const QString &qss);

signals:
  void closeRequested(QWidget *panel);

public slots:
  void analyzeDtcList(const QStringList &dtcCodes);
  void analyzeNegativeResponse(quint8 sid, quint8 nrc);

private slots:
  void onSendQuestion();
  void onQuickActionClicked(int type);

private:
  void setupUi();
  void appendAssistantMessage(const QString &markdown);
  void appendUserMessage(const QString &text);

  KnowledgeEngine *m_knowledge = nullptr;
  QWidget *m_headerWidget = nullptr;
  QLabel *m_titleLabel = nullptr;
  QPushButton *m_closeBtn = nullptr;

  QTextBrowser *m_chatBrowser = nullptr;
  QLineEdit *m_inputEdit = nullptr;
  QPushButton *m_btnSend = nullptr;
};

#endif // CANAIASSISTANT_H
