/**
 * 水面战舰 SVG 背景轮廓
 */
export function WarshipBg() {
  return (
    <svg
      viewBox="0 0 1100 500"
      width="100%"
      height="100%"
      xmlns="http://www.w3.org/2000/svg"
    >
      {/* Warship Hull & Superstructure */}
      <path d="M 50,300 L 120,200 L 350,200 L 370,140 L 480,140 L 500,200 L 650,200 L 670,160 L 750,160 L 770,220 L 980,220 L 1050,300 L 50,300 Z" fill="none" stroke="var(--primary-color)" strokeWidth="2" strokeOpacity={0.18} strokeDasharray="10, 5" />
      {/* Main deck line */}
      <line x1="50" y1="300" x2="1050" y2="300" stroke="var(--primary-color)" strokeWidth="1.5" strokeOpacity={0.15} />
      {/* Gun turret on bow */}
      <path d="M 170,200 L 190,185 L 220,185 L 230,200 Z" fill="none" stroke="var(--primary-color)" strokeWidth="1.5" strokeOpacity={0.15} />
      <line x1="220" y1="190" x2="255" y2="190" stroke="var(--primary-color)" strokeWidth="2" strokeOpacity={0.15} />
      {/* Radar mast */}
      <line x1="420" y1="140" x2="420" y2="70" stroke="var(--primary-color)" strokeWidth="2" strokeOpacity={0.15} />
      <path d="M 405,70 C 405,70 420,60 435,70" stroke="var(--primary-color)" strokeWidth="1.5" strokeOpacity={0.15} />
    </svg>
  );
}
