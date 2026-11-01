import { useSystemClock } from '../../hooks/useSystemClock';

export function SystemClock() {
  const time = useSystemClock();

  return <div style={clockStyle}>{time}</div>;
}

const clockStyle: React.CSSProperties = {
  fontFamily: "'Share Tech Mono', monospace",
  fontSize: '13px',
  color: 'var(--primary-color)',
  textShadow: '0 0 5px var(--primary-glow)',
  background: 'var(--primary-glow-light)',
  border: '1px solid var(--primary-border)',
  padding: '4px 10px',
  borderRadius: '6px',
  letterSpacing: '0.5px',
  userSelect: 'none',
};
