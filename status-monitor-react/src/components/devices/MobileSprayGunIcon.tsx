/** 移动喷枪 SVG 图标 — 喷嘴 + 握把 + 喷射粒子 */
export function MobileSprayGunIcon() {
  return (
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <rect x="8" y="16" width="20" height="10" rx="3" fill="#334155" stroke="#475569" strokeWidth="1.5" />
      <rect x="28" y="18" width="10" height="6" rx="2" fill="#1e293b" stroke="#64748b" strokeWidth="1" />
      <rect x="38" y="19" width="6" height="4" rx="1" fill="#475569" />
      <rect x="12" y="26" width="8" height="14" rx="2" fill="#1e293b" stroke="#475569" strokeWidth="1" />
      <rect x="14" y="28" width="4" height="10" rx="1" fill="#0f172a" />
      <rect x="2" y="18" width="6" height="8" rx="2" fill="#334155" />
      <circle cx="5" cy="22" r="2" fill="#1e293b" />
      <circle className="svg-spray-dot svg-spray-dot1" cx="44" cy="20" r="1" fill="#00e676" opacity="0" />
      <circle className="svg-spray-dot svg-spray-dot2" cx="46" cy="22" r="1.2" fill="#10b981" opacity="0" />
      <circle className="svg-spray-dot svg-spray-dot3" cx="45" cy="24" r="0.8" fill="#34d399" opacity="0" />
      <circle className="svg-led" cx="18" cy="8" r="2" fill="#10b981" />
    </svg>
  );
}
