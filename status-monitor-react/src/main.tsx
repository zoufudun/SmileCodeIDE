import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';
import { ErrorBoundary } from './components/common/ErrorBoundary';

// 全局样式
import './styles/_variables.css';
import './styles/_animations.css';
import './styles/_base.css';
import './styles/_scrollbar.css';
import './styles/_editmode.css';

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <ErrorBoundary>
      <App />
    </ErrorBoundary>
  </React.StrictMode>
);
