/** 压力开关 SVG 图标 — 膜片式 + 电气触点 + 报警阈值 */
export function PressureSwitchIcon() {
  return (
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <rect x="12" y="30" width="24" height="12" rx="4" fill="#1e293b" stroke="#475569" strokeWidth="1.5" />
      <circle cx="20" cy="36" r="2.5" fill="#334155" stroke="#64748b" strokeWidth="1" />
      <circle cx="28" cy="36" r="2.5" fill="#334155" stroke="#64748b" strokeWidth="1" />
      <line x1="22" y1="36" x2="25" y2="36" stroke="#475569" strokeWidth="1.5" />
      <path d="M 14 30 C 14 18, 34 18, 34 30" fill="#1e293b" stroke="#475569" strokeWidth="1.5" />
      <path d="M 16 30 C 16 20, 32 20, 32 30" fill="none" stroke="#334155" strokeWidth="1" />
      <rect x="21" y="38" width="6" height="8" rx="1" fill="#334155" />
      <circle className="svg-led" cx="24" cy="8" r="2.5" fill="#10b981" />
      <line x1="12" y1="24" x2="36" y2="24" stroke="#ef4444" strokeWidth="0.8" strokeDasharray="2,2" opacity="0.4" />
    </svg>
  );
}
