#include "dbcparserengine.h"
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

DbcParserEngine::DbcParserEngine(QObject *parent) : QObject(parent) {}

DbcParserEngine::~DbcParserEngine() {
  clear();
}

void DbcParserEngine::clear() {
  m_messages.clear();
}

bool DbcParserEngine::hasMessage(quint32 id) const {
  return m_messages.contains(id);
}

DbcMessage DbcParserEngine::message(quint32 id) const {
  return m_messages.value(id);
}

bool DbcParserEngine::loadDbcFile(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    emit errorOccurred(QString("无法打开 DBC 文件: %1").arg(filePath));
    return false;
  }
  QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  in.setCodec("UTF-8");
#endif
  QString content = in.readAll();
  file.close();
  return loadDbcContent(content);
}

bool DbcParserEngine::loadDbcContent(const QString &content) {
  clear();

  // 正则表达式
  // BO_ 256 EngineData: 8 Vector__XXX
  QRegularExpression reBo(R"dbc(^BO_\s+(\d+)\s+([a-zA-Z0-9_]+)\s*:\s*(\d+)\s+([a-zA-Z0-9_]+))dbc");
  // SG_ RPM : 0|16@1+ (0.25,0) [0|16000] "rpm" Vector__XXX
  QRegularExpression reSg(R"dbc(^\s*SG_\s+([a-zA-Z0-9_]+)\s*(?:[Mm\d]*)\s*:\s*(\d+)\|(\d+)@([01])([+-])\s*\(([-+0-9.eE]+),([-+0-9.eE]+)\)\s*\[([-+0-9.eE]+)\|([-+0-9.eE]+)\]\s*"([^"]*)"\s*([a-zA-Z0-9_]*))dbc");
  // VAL_ 256 SignalName 0 "Desc0" 1 "Desc1" ;
  QRegularExpression reVal(R"dbc(^VAL_\s+(\d+)\s+([a-zA-Z0-9_]+)\s+(.+);)dbc");

  QStringList lines = content.split(QRegularExpression("[\r\n]+"));
  DbcMessage *currentMsg = nullptr;
  int totalSignals = 0;

  for (const QString &line : lines) {
    QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) continue;

    // 匹配 BO_
    QRegularExpressionMatch matchBo = reBo.match(trimmed);
    if (matchBo.hasMatch()) {
      quint32 rawId = matchBo.captured(1).toUInt();
      bool ext = (rawId & 0x80000000) != 0;
      quint32 canId = ext ? (rawId & 0x1FFFFFFF) : rawId;

      DbcMessage msg;
      msg.id = canId;
      msg.isExtended = ext;
      msg.name = matchBo.captured(2);
      msg.dlc = matchBo.captured(3).toInt();
      msg.sender = matchBo.captured(4);

      m_messages[canId] = msg;
      currentMsg = &m_messages[canId];
      continue;
    }

    // 匹配 SG_
    if (currentMsg) {
      QRegularExpressionMatch matchSg = reSg.match(line);
      if (matchSg.hasMatch()) {
        DbcSignal sig;
        sig.name = matchSg.captured(1);
        sig.startBit = matchSg.captured(2).toInt();
        sig.length = matchSg.captured(3).toInt();
        sig.byteOrder = (matchSg.captured(4) == "1") ? DbcByteOrder::Intel : DbcByteOrder::Motorola;
        sig.isSigned = (matchSg.captured(5) == "-");
        sig.factor = matchSg.captured(6).toDouble();
        sig.offset = matchSg.captured(7).toDouble();
        sig.min = matchSg.captured(8).toDouble();
        sig.max = matchSg.captured(9).toDouble();
        sig.unit = matchSg.captured(10);

        currentMsg->signalList.append(sig);
        totalSignals++;
        continue;
      }
    }

    // 匹配 VAL_
    QRegularExpressionMatch matchVal = reVal.match(trimmed);
    if (matchVal.hasMatch()) {
      quint32 rawId = matchVal.captured(1).toUInt();
      quint32 canId = (rawId & 0x80000000) ? (rawId & 0x1FFFFFFF) : rawId;
      QString sigName = matchVal.captured(2);
      QString rest = matchVal.captured(3);

      if (m_messages.contains(canId)) {
        for (DbcSignal &s : m_messages[canId].signalList) {
          if (s.name == sigName) {
            QRegularExpression rePair(R"val((\d+)\s+"([^"]+)")val");
            auto iter = rePair.globalMatch(rest);
            while (iter.hasNext()) {
              auto m = iter.next();
              s.valueDescriptions[m.captured(1).toInt()] = m.captured(2);
            }
            break;
          }
        }
      }
    }
  }

  emit dbcLoaded(m_messages.size(), totalSignals);
  return true;
}

quint64 DbcParserEngine::extractRawBits(const QByteArray &data, int startBit, int length, DbcByteOrder order) {
  if (data.isEmpty() || length <= 0 || length > 64) return 0;

  quint64 rawValue = 0;

  if (order == DbcByteOrder::Intel) {
    // Intel Little-Endian
    for (int i = 0; i < length; ++i) {
      int bitPos = startBit + i;
      int byteIndex = bitPos / 8;
      int bitIndex = bitPos % 8;

      if (byteIndex < data.size()) {
        quint8 b = static_cast<quint8>(data.at(byteIndex));
        if ((b >> bitIndex) & 0x01) {
          rawValue |= (static_cast<quint64>(1) << i);
        }
      }
    }
  } else {
    // Motorola Big-Endian (MSB first)
    for (int i = 0; i < length; ++i) {
      int byteIndex = startBit / 8;
      int bitIndex = startBit % 8;

      if (byteIndex < data.size()) {
        quint8 b = static_cast<quint8>(data.at(byteIndex));
        if ((b >> bitIndex) & 0x01) {
          rawValue |= (static_cast<quint64>(1) << (length - 1 - i));
        }
      }

      if (bitIndex == 0) {
        startBit += 15;
      } else {
        startBit -= 1;
      }
    }
  }

  return rawValue;
}

QList<DecodedSignal> DbcParserEngine::decodeFrame(const CanRawFrame &frame) const {
  QList<DecodedSignal> result;
  if (!m_messages.contains(frame.id)) return result;

  const DbcMessage &msg = m_messages.value(frame.id);

  for (const DbcSignal &sig : msg.signalList) {
    quint64 rawBits = extractRawBits(frame.data, sig.startBit, sig.length, sig.byteOrder);

    double rawNum = 0.0;
    if (sig.isSigned) {
      // 符号位扩展
      if (sig.length < 64 && (rawBits & (static_cast<quint64>(1) << (sig.length - 1)))) {
        quint64 mask = (static_cast<quint64>(~0)) << sig.length;
        rawNum = static_cast<qint64>(rawBits | mask);
      } else {
        rawNum = static_cast<qint64>(rawBits);
      }
    } else {
      rawNum = static_cast<double>(rawBits);
    }

    double phys = rawNum * sig.factor + sig.offset;

    DecodedSignal ds;
    ds.name = sig.name;
    ds.rawValue = rawNum;
    ds.physicalValue = phys;
    ds.unit = sig.unit;

    int intVal = static_cast<int>(rawNum);
    if (sig.valueDescriptions.contains(intVal)) {
      ds.valueString = sig.valueDescriptions.value(intVal);
    } else {
      ds.valueString = QString::number(phys, 'f', 2);
      if (!sig.unit.isEmpty()) {
        ds.valueString += " " + sig.unit;
      }
    }

    result.append(ds);
  }

  return result;
}
