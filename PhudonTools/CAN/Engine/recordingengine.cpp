#include "recordingengine.h"
#include <QDateTime>
#include <QRegularExpression>
#include <QFileInfo>

RecordingEngine::RecordingEngine(QObject *parent) : QObject(parent) {
  m_playbackTimer = new QTimer(this);
  connect(m_playbackTimer, &QTimer::timeout, this, &RecordingEngine::onPlaybackTick);
}

RecordingEngine::~RecordingEngine() {
  stopRecording();
  stopPlayback();
}

bool RecordingEngine::startRecording(const QString &filePath, RecordFormat format) {
  stopRecording();

  m_recordFile.setFileName(filePath);
  if (!m_recordFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    emit errorOccurred(QString("无法创建录制文件: %1").arg(filePath));
    return false;
  }

  m_format = format;
  m_recordStream.setDevice(&m_recordFile);
  m_recordStartTime = QDateTime::currentMSecsSinceEpoch();

  if (m_format == RecordFormat::ASC) {
    m_recordStream << "date " << QDateTime::currentDateTime().toString("ddd MMM dd hh:mm:ss.zzz yyyy") << "\n";
    m_recordStream << "base hex  timestamps absolute\n";
    m_recordStream << "internal events logged\n";
    m_recordStream << "// CANPilot Recorded Trace\n";
  } else {
    m_recordStream << "Timestamp(ms),Channel,ID,Type,Tx/Rx,DLC,Data\n";
  }

  m_isRecording = true;
  return true;
}

void RecordingEngine::stopRecording() {
  if (m_isRecording) {
    m_recordStream.flush();
    m_recordFile.close();
    m_isRecording = false;
  }
}

void RecordingEngine::recordFrame(const CanRawFrame &frame, bool isTx) {
  if (!m_isRecording) return;

  double relTimeSec = (frame.timestamp > 0 ? (frame.timestamp - m_recordStartTime) : (QDateTime::currentMSecsSinceEpoch() - m_recordStartTime)) / 1000.0;
  if (relTimeSec < 0) relTimeSec = 0;

  if (m_format == RecordFormat::ASC) {
    // 例如:   0.001230 1  100             Rx   d 8 11 22 33 44 55 66 77 88
    QString dir = isTx ? "Tx" : "Rx";
    QString idStr = QString("%1").arg(frame.id, frame.extended ? 8 : 3, 16, QLatin1Char(' ')).toUpper();
    if (frame.extended) idStr += "x";

    m_recordStream << QString("%1").arg(relTimeSec, 10, 'f', 6) << " "
                   << (frame.channel + 1) << "  "
                   << idStr << "             "
                   << dir << "   d " << frame.dlc() << " ";
    for (int i = 0; i < frame.data.size(); ++i) {
      m_recordStream << QString("%1 ").arg(static_cast<quint8>(frame.data.at(i)), 2, 16, QLatin1Char('0')).toUpper();
    }
    m_recordStream << "\n";
  } else {
    // CSV 格式
    m_recordStream << (frame.timestamp > 0 ? frame.timestamp : QDateTime::currentMSecsSinceEpoch()) << ","
                   << frame.channel << ","
                   << "0x" + QString::number(frame.id, 16).toUpper() << ","
                   << (frame.extended ? "EXT" : "STD") << ","
                   << (isTx ? "Tx" : "Rx") << ","
                   << frame.dlc() << ","
                   << frame.toHexString("") << "\n";
  }
}

bool RecordingEngine::loadForPlayback(const QString &filePath) {
  stopPlayback();
  m_playbackFrames.clear();
  m_playbackOffsets.clear();

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    emit errorOccurred(QString("无法打开回放文件: %1").arg(filePath));
    return false;
  }

  QTextStream in(&file);
  qint64 firstTs = -1;

  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith("//") || line.startsWith("date") || line.startsWith("base") || line.startsWith("internal") || line.startsWith("Timestamp")) {
      continue;
    }

    if (filePath.endsWith(".csv", Qt::CaseInsensitive)) {
      // CSV 解析
      QStringList parts = line.split(',');
      if (parts.size() >= 7) {
        qint64 ts = parts[0].toLongLong();
        if (firstTs < 0) firstTs = ts;

        CanRawFrame f;
        f.timestamp = ts;
        f.channel = parts[1].toInt();
        f.id = parts[2].toUInt(nullptr, 16);
        f.extended = (parts[3] == "EXT");
        f.data = QByteArray::fromHex(parts[6].toLatin1());

        m_playbackFrames.append(f);
        m_playbackOffsets.append(ts - firstTs);
      }
    } else {
      // ASC 解析简易版
      QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
      if (tokens.size() >= 6) {
        double sec = tokens[0].toDouble();
        qint64 offsetMs = static_cast<qint64>(sec * 1000);

        CanRawFrame f;
        f.channel = qMax(0, tokens[1].toInt() - 1);
        QString idStr = tokens[2];
        if (idStr.endsWith("x", Qt::CaseInsensitive)) {
          f.extended = true;
          idStr.chop(1);
        }
        f.id = idStr.toUInt(nullptr, 16);

        int dlcIndex = tokens.indexOf("d");
        if (dlcIndex != -1 && dlcIndex + 1 < tokens.size()) {
          int dlc = tokens[dlcIndex + 1].toInt();
          QByteArray bytes;
          for (int i = 0; i < dlc && (dlcIndex + 2 + i) < tokens.size(); ++i) {
            bytes.append(static_cast<char>(tokens[dlcIndex + 2 + i].toInt(nullptr, 16)));
          }
          f.data = bytes;
        }

        m_playbackFrames.append(f);
        m_playbackOffsets.append(offsetMs);
      }
    }
  }

  file.close();
  m_currentPlaybackIndex = 0;
  return !m_playbackFrames.isEmpty();
}

void RecordingEngine::startPlayback(double speedRatio) {
  if (m_playbackFrames.isEmpty()) return;
  m_speedRatio = (speedRatio > 0.05) ? speedRatio : 1.0;
  m_isPlaying = true;
  m_playbackStartRealTime = QDateTime::currentMSecsSinceEpoch();
  m_playbackTimer->start(10); // 10ms 高频检查
}

void RecordingEngine::pausePlayback() {
  m_isPlaying = false;
  m_playbackTimer->stop();
}

void RecordingEngine::stopPlayback() {
  m_isPlaying = false;
  m_playbackTimer->stop();
  m_currentPlaybackIndex = 0;
}

void RecordingEngine::onPlaybackTick() {
  if (!m_isPlaying || m_currentPlaybackIndex >= m_playbackFrames.size()) {
    stopPlayback();
    emit playbackFinished();
    return;
  }

  qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_playbackStartRealTime;
  qint64 scaledElapsed = static_cast<qint64>(elapsed * m_speedRatio);

  while (m_currentPlaybackIndex < m_playbackFrames.size()) {
    qint64 frameOffset = m_playbackOffsets[m_currentPlaybackIndex];
    if (frameOffset <= scaledElapsed) {
      emit playbackFrameReady(m_playbackFrames[m_currentPlaybackIndex]);
      m_currentPlaybackIndex++;
      emit playbackProgress(m_currentPlaybackIndex, m_playbackFrames.size());
    } else {
      break;
    }
  }
}
