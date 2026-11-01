/** 手动报警按钮 — 逼真消防报警按钮（红色按钮+面板+螺丝） */
export function ManualCallPointIcon() {
  return (
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <linearGradient id="mcp-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor="#dc2626" />
          <stop offset="50%" stopColor="#ef4444" />
          <stop offset="100%" stopColor="#b91c1c" />
        </linearGradient>
        <filter id="mcp-shadow">
          <feDropShadow dx="0" dy="1" stdDeviation="2" floodColor="#000" floodOpacity="0.4" />
        </filter>
      </defs>
      <rect x="6" y="4" width="36" height="40" rx="8" fill="#1e293b" stroke="#475569" strokeWidth="2.5" />
      <rect x="10" y="8" width="28" height="32" rx="6" fill="#0f172a" stroke="#334155" strokeWidth="1.5" />
      <circle cx="24" cy="24" r="12" fill="#7f1d1d" stroke="#991b1b" strokeWidth="1" />
      <circle className="svg-mcp-button" cx="24" cy="24" r="10" fill="url(#mcp-grad)" stroke="#dc2626" strokeWidth="1.5" filter="url(#mcp-shadow)" />
      <ellipse cx="21" cy="21" rx="4" ry="3" fill="rgba(255,255,255,0.2)" />
      <text x="24" y="25" textAnchor="middle" fontFamily="Arial, sans-serif" fontSize="5.5" fontWeight="bold" fill="#ffffff">FIRE</text>
      <text x="24" y="31" textAnchor="middle" fontFamily="Arial, sans-serif" fontSize="3" fontWeight="bold" fill="#fecaca">报警</text>
      <circle className="svg-led" cx="24" cy="42" r="2" fill="#10b981" />
      <circle cx="12" cy="10" r="1.2" fill="#475569" />
      <circle cx="36" cy="10" r="1.2" fill="#475569" />
      <circle cx="12" cy="38" r="1.2" fill="#475569" />
      <circle cx="36" cy="38" r="1.2" fill="#475569" />
      <rect x="17" y="6" width="14" height="3" rx="1" fill="#475569" />
    </svg>
  );
}
