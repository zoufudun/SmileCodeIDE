#ifndef KNOWLEDGEENGINE_H
#define KNOWLEDGEENGINE_H

#include <QMap>
#include <QObject>
#include <QString>

struct DtcKnowledgeItem {
  QString code;        // 如 "P0100", "U0100"
  QString category;    // 动力系统 (P), 底盘 (C), 车身 (B), 网络通信 (U)
  QString title;       // 故障简述
  QString description; // 详细含义
  QString probableCause; // 可能诱因
  QString suggestion;    // 专家排查建议
};

struct UdsServiceDoc {
  quint8 sid;
  QString name;
  QString subFunctions;
  QString usage;
};

class KnowledgeEngine : public QObject {
  Q_OBJECT
public:
  explicit KnowledgeEngine(QObject *parent = nullptr);
  ~KnowledgeEngine() override = default;

  // 查询 DTC 知识库
  bool hasDtc(const QString &dtcCode) const;
  DtcKnowledgeItem dtcDetails(const QString &dtcCode) const;

  // 将 3 字节原始 DTC (如 0x01, 0x00, 0x2F) 转换为标准字符串 (如 "P0100-2F")
  static QString formatDtcCode(quint8 b1, quint8 b2, quint8 b3 = 0);

  // 获取 UDS 服务说明
  UdsServiceDoc udsService(quint8 sid) const;

  // AI 智能诊断：针对故障码列表生成排障分析报告
  QString generateTroubleshootingReport(const QStringList &dtcList) const;

private:
  void initDatabase();

  QMap<QString, DtcKnowledgeItem> m_dtcDb;
  QMap<quint8, UdsServiceDoc> m_udsDb;
};

#endif // KNOWLEDGEENGINE_H
