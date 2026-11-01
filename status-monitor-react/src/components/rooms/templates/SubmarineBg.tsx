/**
 * 核潜艇 SVG 背景剖视图
 * 包含壳体、指挥塔、螺旋桨、舵面、隔舱线、反应堆发光
 */
export function SubmarineBg() {
  return (
    <svg
      viewBox="0 0 2000 700"
      width="2000"
      height="700"
      xmlns="http://www.w3.org/2000/svg"
      style={{ position: 'absolute', left: 20, top: 20 }}
    >
      <defs>
        <linearGradient id="sub-gradient" x1="0%" y1="0%" x2="100%" y2="100%">
          <stop offset="0%" stopColor="#0f172a" stopOpacity={0.95} />
          <stop offset="50%" stopColor="#1e293b" stopOpacity={0.85} />
          <stop offset="100%" stopColor="#0b1329" stopOpacity={0.95} />
        </linearGradient>
        <linearGradient id="reactor-glow" x1="0%" y1="0%" x2="0%" y2="100%">
          <stop offset="0%" stopColor="#ef4444" stopOpacity={0.3} />
          <stop offset="100%" stopColor="#ef4444" stopOpacity={0.0} />
        </linearGradient>
      </defs>

      {/* Propeller Shaft & Propeller Blades */}
      <rect x="1800" y="385" width="40" height="15" fill="#334155" stroke="var(--primary-color)" strokeWidth="1" opacity={0.8} />
      <path d="M 1830,310 C 1830,310 1845,392 1810,392 M 1810,392 C 1845,392 1830,475 1830,475" stroke="var(--primary-color)" strokeWidth="3" fill="none" opacity={0.6} />

      {/* Rudder / Vertical stabilizers */}
      <path d="M 1700,280 L 1750,220 L 1790,220 L 1760,340 Z" fill="#0f172a" stroke="var(--primary-color)" strokeWidth="2" opacity={0.8} />
      <path d="M 1700,500 L 1750,560 L 1790,560 L 1760,440 Z" fill="#0f172a" stroke="var(--primary-color)" strokeWidth="2" opacity={0.8} />

      {/* Conning Tower / Sail / Command deck */}
      <path d="M 400,320 L 410,130 L 520,130 L 550,320 Z" fill="url(#sub-gradient)" stroke="var(--primary-color)" strokeWidth="3" />
      {/* Sail details / Periscope masts */}
      <line x1="450" y1="130" x2="450" y2="70" stroke="var(--primary-color)" strokeWidth="3" opacity={0.8} />
      <circle cx="450" cy="70" r="3" fill="var(--primary-color)" />
      <line x1="480" y1="130" x2="480" y2="50" stroke="var(--primary-color)" strokeWidth="2" opacity={0.8} />
      <line x1="480" y1="50" x2="490" y2="50" stroke="var(--primary-color)" strokeWidth="2" opacity={0.8} />

      {/* Main Outer Hull */}
      <path
        d="M 50,392 C 70,280 250,220 500,220 L 1700,220 C 1780,220 1810,320 1810,392 C 1810,460 1780,560 1700,560 L 500,560 C 250,560 70,500 50,392 Z"
        fill="url(#sub-gradient)"
        stroke="var(--primary-color)"
        strokeWidth="3.5"
        filter="drop-shadow(0 0 15px var(--primary-glow))"
      />

      {/* Torpedo Tube Outlets */}
      <path d="M 55,360 L 75,360 M 50,392 L 72,392 M 55,420 L 75,420" stroke="var(--primary-color)" strokeWidth="2" opacity={0.7} />

      {/* Hull Internal Bulkhead Dividers */}
      <line x1="350" y1="240" x2="350" y2="540" stroke="var(--primary-color)" strokeWidth="1.5" strokeOpacity={0.3} strokeDasharray="4, 4" />
      <line x1="640" y1="240" x2="640" y2="540" stroke="var(--primary-color)" strokeWidth="1.5" strokeOpacity={0.3} strokeDasharray="4, 4" />
      <line x1="860" y1="240" x2="860" y2="540" stroke="var(--primary-color)" strokeWidth="1.5" strokeOpacity={0.3} strokeDasharray="4, 4" />
      <line x1="1080" y1="240" x2="1080" y2="540" stroke="var(--primary-color)" strokeWidth="1.5" strokeOpacity={0.3} strokeDasharray="4, 4" />

      {/* Reactor Compartment Glow */}
      <rect x="865" y="240" width="210" height="300" fill="url(#reactor-glow)" />
      <circle cx="970" cy="390" r="45" fill="none" stroke="#ef4444" strokeWidth="1.5" strokeOpacity={0.4} strokeDasharray="3, 3" />
      <line x1="970" y1="310" x2="970" y2="370" stroke="#ef4444" strokeWidth="3" strokeOpacity={0.5} />
      <line x1="955" y1="310" x2="955" y2="370" stroke="#ef4444" strokeWidth="3" strokeOpacity={0.5} />
      <line x1="985" y1="310" x2="985" y2="370" stroke="#ef4444" strokeWidth="3" strokeOpacity={0.5} />
    </svg>
  );
}
