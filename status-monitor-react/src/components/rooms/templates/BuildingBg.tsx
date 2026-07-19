/**
 * 写字楼 SVG 背景轮廓
 */
export function BuildingBg() {
  return (
    <svg
      viewBox="0 0 1100 500"
      width="100%"
      height="100%"
      xmlns="http://www.w3.org/2000/svg"
    >
      {/* Outer building shell */}
      <rect x="80" y="80" width="640" height="380" rx="10" fill="none" stroke="var(--primary-color)" strokeWidth="2" strokeOpacity={0.18} strokeDasharray="10, 5" />
      {/* Floor dividers */}
      <line x1="80" y1="205" x2="720" y2="205" stroke="var(--primary-color)" strokeWidth="1.5" strokeOpacity={0.15} />
      <line x1="80" y1="330" x2="720" y2="330" stroke="var(--primary-color)" strokeWidth="1.5" strokeOpacity={0.15} />
      {/* Window grids */}
      <line x1="200" y1="80" x2="200" y2="460" stroke="var(--primary-color)" strokeWidth="1" strokeDasharray="2, 8" strokeOpacity={0.08} />
      <line x1="320" y1="80" x2="320" y2="460" stroke="var(--primary-color)" strokeWidth="1" strokeDasharray="2, 8" strokeOpacity={0.08} />
      <line x1="440" y1="80" x2="440" y2="460" stroke="var(--primary-color)" strokeWidth="1" strokeDasharray="2, 8" strokeOpacity={0.08} />
      <line x1="560" y1="80" x2="560" y2="460" stroke="var(--primary-color)" strokeWidth="1" strokeDasharray="2, 8" strokeOpacity={0.08} />
      {/* Rooftop antenna */}
      <line x1="400" y1="80" x2="400" y2="30" stroke="var(--primary-color)" strokeWidth="2" strokeOpacity={0.15} />
      <circle cx="400" cy="30" r="3" fill="var(--primary-color)" opacity={0.2} />
    </svg>
  );
}
