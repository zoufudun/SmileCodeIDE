#ifndef DBCPARSERENGINE_H
#define DBCPARSERENGINE_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include "../Adapter/icanadapter.h"

// 信号字节序
enum class DbcByteOrder {
  Intel,    // 小端 Little-Endian
  Motorola  // 大端 Big-Endian
};

// 信号定义
struct DbcSignal {
  QString name;
  int startBit = 0;
  int length = 0;
  DbcByteOrder byteOrder = DbcByteOrder::Intel;
  bool isSigned = false;
  double factor = 1.0;
  double offset = 0.0;
  double min = 0.0;
  double max = 0.0;
  QString unit;
  QString comment;
  QMap<int, QString> valueDescriptions; // 枚举描述
};

// 报文定义
struct DbcMessage {
  quint32 id = 0;
  bool isExtended = false;
  QString name;
  int dlc = 8;
  QString sender;
  QList<DbcSignal> signalList;
};

// 解码后的物理值信号
struct DecodedSignal {
  QString name;
  double rawValue = 0.0;
  double physicalValue = 0.0;
  QString unit;
  QString valueString; // 若有枚举定义显示枚举，否则显示物理值+单位
};

class DbcParserEngine : public QObject {
  Q_OBJECT
public:
  explicit DbcParserEngine(QObject *parent = nullptr);
  ~DbcParserEngine() override;

  // 加载并解析 DBC 文件
  bool loadDbcFile(const QString &filePath);
  bool loadDbcContent(const QString &content);
  void clear();

  bool hasMessage(quint32 id) const;
  DbcMessage message(quint32 id) const;
  QList<DbcMessage> allMessages() const { return m_messages.values(); }

  // 对一帧原始报文进行解码
  QList<DecodedSignal> decodeFrame(const CanRawFrame &frame) const;

  // 辅助函数：根据信号提取原始数值
  static quint64 extractRawBits(const QByteArray &data, int startBit, int length, DbcByteOrder order);

signals:
  void dbcLoaded(int messageCount, int signalCount);
  void errorOccurred(const QString &msg);

private:
  QMap<quint32, DbcMessage> m_messages; // ID -> Message
};

#endif // DBCPARSERENGINE_H
