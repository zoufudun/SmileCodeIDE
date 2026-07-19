/**
 * 控制分配阀 / 泵 SVG 图标 (48x48)
 * 水平管道 + 旋转阀芯 + 水流粒子动画
 */
export function ValveIcon() {
  return (
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <linearGradient id="pipe-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor="#475569" />
          <stop offset="30%" stopColor="#64748b" />
          <stop offset="70%" stopColor="#475569" />
          <stop offset="100%" stopColor="#334155" />
        </linearGradient>
        <radialGradient id="valve-grad" cx="50%" cy="40%" r="50%">
          <stop offset="0%" stopColor="#64748b" />
          <stop offset="60%" stopColor="#334155" />
          <stop offset="100%" stopColor="#1e293b" />
        </radialGradient>
        <filter id="flow-glow">
          <feGaussianBlur stdDeviation="1.5" result="blur" />
          <feMerge>
            <feMergeNode in="blur" />
            <feMergeNode in="SourceGraphic" />
          </feMerge>
        </filter>
      </defs>
      {/* 左管道 */}
      <rect x="0" y="21" width="12" height="6" rx="2" fill="url(#pipe-grad)" />
      {/* 右管道 */}
      <rect x="36" y="21" width="12" height="6" rx="2" fill="url(#pipe-grad)" />
      {/* 水流路径 */}
      <line className="svg-flow-path" x1="2" y1="24" x2="46" y2="24" stroke="#475569" strokeWidth="3" strokeLinecap="round" />
      {/* 流动粒子 */}
      <circle className="svg-flow-dot svg-flow-dot1" cx="8" cy="24" r="1.5" fill="#10b981" opacity="0" />
      <circle className="svg-flow-dot svg-flow-dot2" cx="20" cy="24" r="1.5" fill="#10b981" opacity="0" />
      <circle className="svg-flow-dot svg-flow-dot3" cx="32" cy="24" r="1.5" fill="#10b981" opacity="0" />
      <circle className="svg-flow-dot svg-flow-dot4" cx="42" cy="24" r="1.5" fill="#10b981" opacity="0" />
      {/* 法兰 */}
      <rect x="12" y="18" width="3" height="12" rx="1" fill="#64748b" stroke="#475569" strokeWidth="0.5" />
      <rect x="33" y="18" width="3" height="12" rx="1" fill="#64748b" stroke="#475569" strokeWidth="0.5" />
      {/* 阀体 */}
      <circle cx="24" cy="24" r="9" fill="url(#valve-grad)" stroke="#64748b" strokeWidth="1.5" />
      <circle cx="24" cy="24" r="7" fill="none" stroke="#475569" strokeWidth="0.5" opacity="0.5" />
      {/* 阀芯蝶板 */}
      <rect className="svg-wheel" x="22.5" y="18" width="3" height="12" rx="1.5" fill="#f59e0b" stroke="#d97706" strokeWidth="0.5" />
      {/* 中心轴 */}
      <circle cx="24" cy="24" r="2.5" fill="#cbd5e1" stroke="#94a3b8" strokeWidth="0.5" />
      {/* 顶部手轮 */}
      <rect x="22" y="13" width="4" height="3" rx="0.5" fill="#ef4444" />
      <rect x="20" y="10" width="8" height="3" rx="1" fill="#64748b" />
    </svg>
  );
}
