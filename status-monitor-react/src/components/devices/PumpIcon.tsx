/** 泵 SVG 图标 — 电机 + 旋转叶轮 + 管道水流 */
export function PumpIcon() {
  return (
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <linearGradient id="motor-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor="#64748b" />
          <stop offset="50%" stopColor="#475569" />
          <stop offset="100%" stopColor="#334155" />
        </linearGradient>
      </defs>
      <rect x="10" y="38" width="28" height="6" rx="3" fill="#1e293b" stroke="#334155" strokeWidth="1" />
      <circle cx="24" cy="24" r="12" fill="url(#motor-grad)" stroke="#475569" strokeWidth="1.5" />
      <circle cx="24" cy="24" r="8" fill="#1e293b" stroke="#334155" strokeWidth="1" />
      <g className="svg-impeller" transform-origin="24 24">
        <path d="M24 18 L26 22 L30 24 L26 26 L24 30 L22 26 L18 24 L22 22 Z" fill="#475569" stroke="#64748b" strokeWidth="0.5" />
      </g>
      <circle cx="24" cy="24" r="2.5" fill="#64748b" stroke="#94a3b8" strokeWidth="0.8" />
      <rect x="4" y="21" width="8" height="6" rx="2" fill="#334155" />
      <rect x="36" y="21" width="8" height="6" rx="2" fill="#334155" />
      <line className="svg-flow-path" x1="6" y1="24" x2="42" y2="24" stroke="#475569" strokeWidth="2" strokeLinecap="round" />
      <circle className="svg-flow-dot svg-flow-dot1" cx="10" cy="24" r="1.2" fill="#10b981" opacity="0" />
      <circle className="svg-flow-dot svg-flow-dot2" cx="38" cy="24" r="1.2" fill="#10b981" opacity="0" />
      <circle className="svg-led" cx="24" cy="41" r="1.8" fill="#10b981" />
    </svg>
  );
}
