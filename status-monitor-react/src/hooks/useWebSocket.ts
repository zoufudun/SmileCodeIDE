import { useEffect, useRef, useCallback } from 'react';
import { WS_URL, RECONNECT_INTERVAL } from '../constants/defaults';
import { useWebSocketStore } from '../store/useWebSocketStore';
import { useDeviceStore } from '../store/useDeviceStore';
import { useLogStore } from '../store/useLogStore';
import { translateRawMessage, getUpdateLogType, formatUpdateLog } from '../utils/log-helpers';
import type { ConfigMessage, UpdateMessage, WSMessage } from '../types/device';

/**
 * WebSocket 连接管理 hook
 * - 自动连接 ws://localhost:12345
 * - 断线后 3 秒自动重连
 * - 解析 config/update 消息并分发到对应 store
 */
export function useWebSocket() {
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimerRef = useRef<ReturnType<typeof setInterval> | null>(null);

  const setConnectionStatus = useWebSocketStore((s) => s.setConnectionStatus);
  const setMappings = useWebSocketStore((s) => s.setMappings);
  const updateDeviceStatus = useWebSocketStore((s) => s.updateDeviceStatus);
  const initFromMappings = useDeviceStore((s) => s.initFromMappings);
  const updateDevice = useDeviceStore((s) => s.updateDevice);
  const appendLog = useLogStore((s) => s.appendLog);

  const connect = useCallback(() => {
    if (wsRef.current) {
      wsRef.current.close();
    }

    appendLog('正在连接本地 Qt CAN 消息服务...', 'system');

    try {
      wsRef.current = new WebSocket(WS_URL);
    } catch {
      // WebSocket 构造失败
      return;
    }

    wsRef.current.onopen = () => {
      setConnectionStatus('connected');
      appendLog('已成功连接到 Qt CAN 消息服务器！开始接收实时设备状态数据。', 'system');
      if (reconnectTimerRef.current) {
        clearInterval(reconnectTimerRef.current);
        reconnectTimerRef.current = null;
      }
    };

    wsRef.current.onclose = () => {
      setConnectionStatus('disconnected');
      appendLog('与服务器断开连接，准备进行自动重连...', 'warning');

      if (!reconnectTimerRef.current) {
        reconnectTimerRef.current = setInterval(connect, RECONNECT_INTERVAL);
      }
    };

    wsRef.current.onerror = () => {
      // 错误由 onclose 处理
    };

    wsRef.current.onmessage = (event: MessageEvent) => {
      // 信息日志：翻译原始报文
      const translatedMsg = translateRawMessage(event.data as string);
      appendLog(translatedMsg, 'raw');

      try {
        const data = JSON.parse(event.data as string) as WSMessage;

        if (data.type === 'config') {
          const configMsg = data as ConfigMessage;
          setMappings(configMsg.mappings);
          initFromMappings(configMsg.mappings);
        } else if (data.type === 'update') {
          const updateMsg = data as UpdateMessage;
          updateDeviceStatus(updateMsg.deviceId, updateMsg.status);
          updateDevice(updateMsg);

          // 报警日志
          const logType = getUpdateLogType(updateMsg.deviceType, updateMsg.status);
          const logContent = formatUpdateLog(updateMsg.label, updateMsg.deviceType, updateMsg.status);
          appendLog(logContent, logType);
        }
      } catch {
        // JSON 解析失败，已在信息日志中显示原始数据
      }
    };
  }, [setConnectionStatus, setMappings, updateDeviceStatus, initFromMappings, updateDevice, appendLog]);

  useEffect(() => {
    connect();

    return () => {
      if (reconnectTimerRef.current) {
        clearInterval(reconnectTimerRef.current);
      }
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [connect]);
}
