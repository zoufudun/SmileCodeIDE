import { useState, useEffect } from 'react';
import { formatDateTime } from '../utils/time-helpers';

/**
 * 系统时钟 hook，每秒更新并返回格式化时间字符串
 */
export function useSystemClock(): string {
  const [time, setTime] = useState(() => formatDateTime(new Date()));

  useEffect(() => {
    const timer = setInterval(() => {
      setTime(formatDateTime(new Date()));
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  return time;
}
