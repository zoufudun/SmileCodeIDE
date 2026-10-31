import { Component, type ReactNode } from 'react';

interface Props {
  children: ReactNode;
}

interface State {
  hasError: boolean;
  error: Error | null;
}

/**
 * 错误边界组件
 * 捕获子组件树中的渲染错误，显示友好的回退 UI
 */
export class ErrorBoundary extends Component<Props, State> {
  constructor(props: Props) {
    super(props);
    this.state = { hasError: false, error: null };
  }

  static getDerivedStateFromError(error: Error): State {
    return { hasError: true, error };
  }

  componentDidCatch(error: Error, errorInfo: React.ErrorInfo) {
    console.error('[ErrorBoundary] 捕获到渲染错误:', error, errorInfo);
  }

  render() {
    if (this.state.hasError) {
      return (
        <div style={containerStyle}>
          <div style={cardStyle}>
            <div style={iconStyle}>⚠️</div>
            <h2 style={titleStyle}>界面渲染错误</h2>
            <p style={descStyle}>
              应用遇到了意外错误。请尝试刷新页面。
            </p>
            {this.state.error && (
              <pre style={errorStyle}>{this.state.error.message}</pre>
            )}
            <button
              style={buttonStyle}
              onClick={() => {
                this.setState({ hasError: false, error: null });
                window.location.reload();
              }}
            >
              🔄 刷新页面
            </button>
          </div>
        </div>
      );
    }

    return this.props.children;
  }
}

const containerStyle: React.CSSProperties = {
  display: 'flex',
  justifyContent: 'center',
  alignItems: 'center',
  height: '100vh',
  background: 'var(--bg-color)',
  color: 'var(--text-primary)',
  fontFamily: "'Inter', -apple-system, sans-serif",
};

const cardStyle: React.CSSProperties = {
  background: 'var(--panel-bg-solid)',
  border: '1px solid var(--danger)',
  borderRadius: '12px',
  padding: '40px',
  maxWidth: '480px',
  textAlign: 'center',
  backdropFilter: 'blur(12px)',
};

const iconStyle: React.CSSProperties = {
  fontSize: '48px',
  marginBottom: '16px',
};

const titleStyle: React.CSSProperties = {
  fontSize: '20px',
  fontWeight: 700,
  color: 'var(--danger)',
  marginBottom: '12px',
  fontFamily: "'Orbitron', sans-serif",
};

const descStyle: React.CSSProperties = {
  fontSize: '14px',
  color: 'var(--text-secondary)',
  marginBottom: '20px',
  lineHeight: 1.6,
};

const errorStyle: React.CSSProperties = {
  background: 'rgba(0, 0, 0, 0.4)',
  border: '1px solid var(--panel-border)',
  borderRadius: '6px',
  padding: '12px',
  fontSize: '12px',
  color: '#f87171',
  marginBottom: '20px',
  textAlign: 'left',
  overflow: 'auto',
  maxHeight: '120px',
  fontFamily: "'Share Tech Mono', monospace",
};

const buttonStyle: React.CSSProperties = {
  padding: '10px 24px',
  border: '1px solid var(--primary-border)',
  background: 'var(--primary-glow-light)',
  color: 'var(--primary-color)',
  borderRadius: '8px',
  fontSize: '14px',
  fontWeight: 600,
  cursor: 'pointer',
  fontFamily: 'inherit',
};
