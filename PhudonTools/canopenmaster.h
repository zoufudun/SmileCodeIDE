#ifndef CANOPENMASTER_H
#define CANOPENMASTER_H

#include <QByteArray>
#include <QObject>
#include <QTimer>
#include <QtGlobal>

#include "caninterface.h"

// CANopen 网络管理(NMT)命令
enum class NmtCommand {
  StartNode = 0x01,
  StopNode = 0x02,
  EnterPreOperational = 0x80,
  ResetNode = 0x81,
  ResetCommunication = 0x82
};

// CANopen 节点状态（心跳/启动报文）
enum class NmtState {
  BootUp = 0x00,
  Stopped = 0x04,
  Operational = 0x05,
  PreOperational = 0x7F,
  Unknown = 0xFF
};

// CANopen 主站测试逻辑：构建在 CanInterface 之上，
// 提供 NMT、SDO 快速读写、PDO/心跳/紧急报文监控。
class CanOpenMaster : public QObject {
  Q_OBJECT
public:
  explicit CanOpenMaster(CanInterface *iface, QObject *parent = nullptr);

  void setNodeId(quint8 nodeId) { m_nodeId = nodeId; }
  quint8 nodeId() const { return m_nodeId; }

  // 发送 NMT 命令。targetNode 为 0 表示广播到所有节点。
  bool sendNmt(NmtCommand command, quint8 targetNode);

  // 发送 SYNC 报文
  bool sendSync();

  // SDO 快速下载（写）。size 为 1~4 字节。
  bool sdoWrite(quint16 index, quint8 subIndex, quint32 value, int size);

  // SDO 快速上传（读）
  bool sdoRead(quint16 index, quint8 subIndex);

  static QString stateText(NmtState state);
  static QString abortText(quint32 abortCode);

public slots:
  void processFrame(const CanFrame &frame);

signals:
  void heartbeatReceived(quint8 nodeId, NmtState state);
  void emcyReceived(quint8 nodeId, quint16 errorCode, quint8 errorRegister,
                    const QByteArray &manufacturer);
  void pdoReceived(quint8 nodeId, int pdoNumber, bool isTpdo,
                   const QByteArray &data);
  void sdoReadFinished(bool success, quint16 index, quint8 subIndex,
                       quint32 value, quint32 abortCode);
  void sdoWriteFinished(bool success, quint16 index, quint8 subIndex,
                        quint32 abortCode);
  void logMessage(const QString &message);

private slots:
  void onSdoTimeout();

private:
  bool sendSdoRequest(const QByteArray &payload);
  void handleSdoResponse(const CanFrame &frame);

  CanInterface *m_iface;
  quint8 m_nodeId = 1;

  QTimer m_sdoTimer;
  bool m_sdoBusy = false;
  bool m_sdoIsRead = false;
  quint16 m_sdoIndex = 0;
  quint8 m_sdoSubIndex = 0;
};

#endif // CANOPENMASTER_H
