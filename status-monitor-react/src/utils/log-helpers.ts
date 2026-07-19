import type { LogType } from '../types/layout';

const TYPE_NAMES: Record<string, string> = {
  detector: '烟温探测器',
  valve: '控制分配阀',
  manual_alarm: '手动报警按钮',
  gas_cylinder: '1301气体钢瓶',
  water_pump: '水泵',
  pressure_switch: '压力开关',
  mobile_spray_gun: '移动喷枪',
};

const STATUS_NAMES: Record<string, Record<string, string>> = {
  detector:          { '0': '正常', '1': '报警' },
  valve:             { '0': '关',   '1': '开' },
  manual_alarm:      { '0': '正常', '1': '按下' },
  gas_cylinder:      { '0': '正常', '1': '泄漏' },
  water_pump:        { '0': '停止', '1': '运转' },
  pressure_switch:   { '0': '关闭', '1': '开启' },
  mobile_spray_gun:  { '0': '停止', '1': '喷射' },
};

function getTypeName(deviceType: string): string {
  return TYPE_NAMES[deviceType] || '未知设备';
}

function getStatusName(deviceType: string, status: boolean): string {
  return STATUS_NAMES[deviceType]?.[status ? '1' : '0'] || (status ? '报警' : '正常');
}

export function translateRawMessage(rawData: string): string {
  try {
    const msg = JSON.parse(rawData);
    if (msg.type === 'config') {
      return `⚙️ 网关配置已同步：成功加载了 ${msg.mappings ? msg.mappings.length : 0} 个设备。`;
    }
    if (msg.type === 'update') {
      const label = msg.label || `ID: #${msg.deviceId}`;
      return `📡 设备变化：${getTypeName(msg.deviceType)} [${label}] → ${getStatusName(msg.deviceType, msg.status)}`;
    }
    return `📩 报文数据: ${rawData}`;
  } catch {
    return `📩 原始数据: ${rawData}`;
  }
}

export function getUpdateLogType(_deviceType: string, status: boolean): LogType {
  return status ? 'alarm' : 'recovery';
}

export function formatUpdateLog(label: string, deviceType: string, status: boolean): string {
  const icon = status ? '🚨' : '✅';
  return `${icon} 【${getTypeName(deviceType)}】[${label}] → ${getStatusName(deviceType, status)}`;
}
