/**
 * 烟温探测器 SVG 图标 (48x48)
 * 精致雷达式传感器，含双雷达脉冲波纹 + LED 辉光
 */
export function DetectorIcon() {
  return (
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      {/* 雷达脉冲波纹 */}
      <circle className="svg-radar-pulse" cx="24" cy="24" r="18" stroke="#ef4444" strokeWidth="2" fill="none" opacity="0" />
      <circle className="svg-radar-pulse2" cx="24" cy="24" r="18" stroke="#ef4444" strokeWidth="1.5" fill="none" opacity="0" />
      {/* 底座 */}
      <rect x="20" y="38" width="8" height="4" rx="2" fill="#334155" />
      <rect x="18" y="40" width="12" height="2" rx="1" fill="#1e293b" />
      {/* 外壳 */}
      <circle cx="24" cy="22" r="15" fill="#0f172a" stroke="#334155" strokeWidth="2" />
      <circle cx="24" cy="22" r="12" fill="#1e293b" stroke="#475569" strokeWidth="1" />
      {/* 传感器栅格 */}
      <line x1="12" y1="22" x2="36" y2="22" stroke="#334155" strokeWidth="1.5" />
      <line x1="24" y1="10" x2="24" y2="34" stroke="#334155" strokeWidth="1.5" />
      {/* 内芯 */}
      <circle cx="24" cy="22" r="7" fill="#0f172a" stroke="#475569" strokeWidth="0.8" />
      <circle cx="24" cy="22" r="3.5" fill="#1e293b" />
      {/* 状态 LED */}
      <circle className="svg-led" cx="24" cy="22" r="2.5" fill="#10b981" />
      <circle className="svg-led-glow" cx="24" cy="22" r="6" fill="none" stroke="#10b981" strokeWidth="0.5" opacity="0.4" />
    </svg>
  );
}
