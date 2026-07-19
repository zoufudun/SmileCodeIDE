/** 1301气体钢瓶 — 红色瓶体 + 压力表盘 + 泄漏时表针摆动 */
export function CylinderIcon() {
  return (
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <linearGradient id="cyl-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor="#dc2626" />
          <stop offset="30%" stopColor="#ef4444" />
          <stop offset="70%" stopColor="#b91c1c" />
          <stop offset="100%" stopColor="#7f1d1d" />
        </linearGradient>
        <linearGradient id="cyl-shine" x1="0" y1="0" x2="1" y2="0">
          <stop offset="0%" stopColor="rgba(255,255,255,0.15)" />
          <stop offset="40%" stopColor="rgba(255,255,255,0.02)" />
          <stop offset="100%" stopColor="rgba(255,255,255,0.0)" />
        </linearGradient>
      </defs>
      <rect x="16" y="8" width="16" height="30" rx="8" fill="url(#cyl-grad)" stroke="#7f1d1d" strokeWidth="1.5" />
      <rect x="19" y="11" width="5" height="24" rx="2" fill="url(#cyl-shine)" />
      <rect x="19" y="3" width="10" height="7" rx="2" fill="#475569" stroke="#334155" strokeWidth="1" />
      <rect x="18" y="0" width="12" height="4" rx="1" fill="#64748b" />
      <circle cx="24" cy="2" r="2.5" fill="#94a3b8" stroke="#cbd5e1" strokeWidth="0.5" />
      <circle cx="24" cy="27" r="6.5" fill="#0f172a" stroke="#475569" strokeWidth="1.2" />
      <circle cx="24" cy="27" r="5" fill="#1e293b" />
      <line x1="24" y1="22" x2="24" y2="23.5" stroke="#ef4444" strokeWidth="1" />
      <line x1="20" y1="27" x2="21.5" y2="27" stroke="#10b981" strokeWidth="0.8" />
      <line className="svg-gauge-needle" x1="24" y1="27" x2="24" y2="22.5" stroke="#10b981" strokeWidth="1.8" strokeLinecap="round" />
      <circle cx="24" cy="27" r="1.5" fill="#cbd5e1" />
      <text x="24" y="31" textAnchor="middle" fontFamily="sans-serif" fontSize="3.5" fontWeight="bold" fill="#fbbf24">1301</text>
      <circle className="svg-led" cx="24" cy="42" r="2" fill="#10b981" />
    </svg>
  );
}
