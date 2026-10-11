#ifndef RECORDINGENGINE_H
#define RECORDINGENGINE_H

#include <QFile>
#include <QList>
#include <QObject>
#include <QTextStream>
#include <QTimer>
#include "../Adapter/icanadapter.h"

enum class RecordFormat {
  ASC, // Vector CANoe 标准 ASC 格式
  CSV  // 通用逗号分隔格式
};

class RecordingEngine : public QObject {
  Q_OBJECT
public:
  explicit RecordingEngine(QObject *parent = nullptr);
  ~RecordingEngine() override;

  // 录制控制
  bool startRecording(const QString &filePath, RecordFormat format = RecordFormat::ASC);
  void stopRecording();
  bool isRecording() const { return m_isRecording; }

  // 接收一帧进行写入
  void recordFrame(const CanRawFrame &frame, bool isTx);

  // 回放控制
  bool loadForPlayback(const QString &filePath);
  void startPlayback(double speedRatio = 1.0);
  void pausePlayback();
  void stopPlayback();
  bool isPlaying() const { return m_isPlaying; }
  int totalPlaybackFrames() const { return m_playbackFrames.size(); }
  int currentPlaybackIndex() const { return m_currentPlaybackIndex; }

signals:
  void playbackFrameReady(const CanRawFrame &frame);
  void playbackProgress(int currentIndex, int totalFrames);
  void playbackFinished();
  void errorOccurred(const QString &msg);

private slots:
  void onPlaybackTick();

private:
  // 录制
  bool m_isRecording = false;
  QFile m_recordFile;
  QTextStream m_recordStream;
  RecordFormat m_format = RecordFormat::ASC;
  qint64 m_recordStartTime = 0;

  // 回放
  bool m_isPlaying = false;
  QList<CanRawFrame> m_playbackFrames;
  QList<qint64> m_playbackOffsets;
  int m_currentPlaybackIndex = 0;
  QTimer *m_playbackTimer = nullptr;
  double m_speedRatio = 1.0;
  qint64 m_playbackStartRealTime = 0;
};

#endif // RECORDINGENGINE_H
