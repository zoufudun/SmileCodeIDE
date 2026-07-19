import { create } from 'zustand';
import type { LogEntry, LogType } from '../types/layout';
import { MAX_LOG_ENTRIES } from '../constants/defaults';

let logIdCounter = 0;
const generateId = () => `log_${Date.now()}_${++logIdCounter}`;

interface LogState {
  alarmLogs: LogEntry[];
  infoLogs: LogEntry[];

  appendLog: (content: string, type: LogType) => void;
  clearAlarmLogs: () => void;
  clearInfoLogs: () => void;
}

function truncateLogs(logs: LogEntry[], max: number): LogEntry[] {
  if (logs.length > max) {
    return logs.slice(logs.length - max);
  }
  return logs;
}

export const useLogStore = create<LogState>((set) => ({
  alarmLogs: [
    {
      id: generateId(),
      timestamp: new Date().toLocaleTimeString(),
      content: '等待连接到本地 Qt CANTool 消息服务器...',
      type: 'system',
    },
  ],
  infoLogs: [
    {
      id: generateId(),
      timestamp: new Date().toLocaleTimeString(),
      content: '等待接收实时设备翻译报文...',
      type: 'system',
    },
  ],

  appendLog: (content, type) => {
    const entry: LogEntry = {
      id: generateId(),
      timestamp: new Date().toLocaleTimeString(),
      content,
      type,
    };

    set((state) => {
      if (type === 'raw') {
        return { infoLogs: truncateLogs([...state.infoLogs, entry], MAX_LOG_ENTRIES) };
      }
      return { alarmLogs: truncateLogs([...state.alarmLogs, entry], MAX_LOG_ENTRIES) };
    });
  },

  clearAlarmLogs: () => set({ alarmLogs: [] }),
  clearInfoLogs: () => set({ infoLogs: [] }),
}));
