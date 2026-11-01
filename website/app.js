/**
 * SmileCodeIDE Official Website Scripts (v2.3.1)
 * Features:
 *  - Dual Theme Switching: Cyber Dark Mode & Warm Parchment (Beige) Mode
 *  - Navigation Mega Menu (Figure 4)
 *  - Right-top capsules (Figure 2)
 */

document.addEventListener('DOMContentLoaded', () => {
  // 1. Dual Theme Management (Dark Mode & Warm Parchment Mode)
  const themeBtn = document.getElementById('themeToggleBtn');
  const themeIcon = document.getElementById('themeToggleIcon');
  const body = document.body;

  // Retrieve saved theme or default to parchment/dark
  const savedTheme = localStorage.getItem('smilecode_theme') || 'parchment';
  applyTheme(savedTheme);

  if (themeBtn) {
    themeBtn.addEventListener('click', () => {
      const current = body.classList.contains('parchment-theme') ? 'parchment' : 'dark';
      const nextTheme = current === 'parchment' ? 'dark' : 'parchment';
      applyTheme(nextTheme);
      localStorage.setItem('smilecode_theme', nextTheme);
    });
  }

  function applyTheme(theme) {
    if (theme === 'dark') {
      body.classList.remove('parchment-theme');
      body.classList.add('dark-theme');
      if (themeIcon) {
        // In dark mode, show sun to switch to warm parchment
        themeIcon.innerHTML = `<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>`;
      }
    } else {
      body.classList.remove('dark-theme');
      body.classList.add('parchment-theme');
      if (themeIcon) {
        // In warm mode, show moon to switch to dark mode (Figure 2)
        themeIcon.innerHTML = `<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"/></svg>`;
      }
    }
  }

  // 2. Mobile Menu Toggle
  const mobileToggle = document.getElementById('mobileToggle');
  const navLinks = document.querySelector('.nav-links');
  if (mobileToggle && navLinks) {
    mobileToggle.addEventListener('click', () => {
      if (navLinks.style.display === 'flex') {
        navLinks.style.display = 'none';
      } else {
        navLinks.style.display = 'flex';
        navLinks.style.flexDirection = 'column';
        navLinks.style.position = 'absolute';
        navLinks.style.top = '68px';
        navLinks.style.left = '0';
        navLinks.style.right = '0';
        navLinks.style.background = 'var(--bg-card)';
        navLinks.style.padding = '20px';
        navLinks.style.borderBottom = '1px solid var(--border-color)';
      }
    });
  }

  // 3. Navbar background shadow on scroll
  const navbar = document.getElementById('navbar');
  window.addEventListener('scroll', () => {
    if (window.scrollY > 30) {
      navbar.style.boxShadow = '0 4px 20px rgba(0,0,0,0.08)';
    } else {
      navbar.style.boxShadow = 'none';
    }
  });
});
