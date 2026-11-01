/**
 * SmileCode Studio Official Website Scripts (v2.3.1)
 * Features:
 *  - Dual Theme Switching: Cyber Dark Mode & Warm Parchment (Beige) Mode
 *  - Real Studio Application Hub Simulator Interactivity (Figure 2 Replica)
 *  - Studio Search Filtering, Category Tab Switching & Star Favorites
 *  - Fixed About & Update Center Modal with Simulation
 *  - Navigation Mega Menu & Top Right Actions
 */

document.addEventListener('DOMContentLoaded', () => {
    // =========================================================================
  // 1. Dual Theme Engine for Website & Simulator (Warm Parchment / Titanium Dark)
  // =========================================================================
  const themeBtn = document.getElementById('themeToggleBtn');
  const themeIcon = document.getElementById('themeToggleIcon');

  const savedTheme = localStorage.getItem('smilecode_theme') || localStorage.getItem('smilecode-theme') || 'parchment';
  applyWebsiteTheme(savedTheme);

  if (themeBtn) {
    themeBtn.addEventListener('click', (e) => {
      e.preventDefault();
      const current = document.documentElement.getAttribute('data-theme') || (document.body.classList.contains('parchment-theme') ? 'parchment' : 'dark');
      const nextTheme = (current === 'dark') ? 'parchment' : 'dark';
      applyWebsiteTheme(nextTheme);
    });
  }

  // Bi-directional binding with Studio Simulator Theme Select Dropdown
  if (studioThemeSelect) {
    studioThemeSelect.addEventListener('change', (e) => {
      const selected = (e.target.value === 'dark' || e.target.value === 'titanium') ? 'dark' : 'parchment';
      applyWebsiteTheme(selected);
    });
  }

  function applyWebsiteTheme(theme) {
    try {
      localStorage.setItem('smilecode_theme', theme);
      localStorage.setItem('smilecode-theme', theme);
    } catch(err) {}
    
    document.documentElement.setAttribute('data-theme', theme);
    document.body.setAttribute('data-theme', theme);

    if (theme === 'dark') {
      document.documentElement.classList.remove('parchment-theme');
      document.documentElement.classList.add('dark-theme');
      document.body.classList.remove('parchment-theme');
      document.body.classList.add('dark-theme');
      if (themeIcon) {
        themeIcon.innerHTML = `<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>`;
      }
      if (studioThemeSelect) {
        studioThemeSelect.value = 'dark';
      }
    } else {
      document.documentElement.classList.remove('dark-theme');
      document.documentElement.classList.add('parchment-theme');
      document.body.classList.remove('dark-theme');
      document.body.classList.add('parchment-theme');
      if (themeIcon) {
        themeIcon.innerHTML = `<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"/></svg>`;
      }
      if (studioThemeSelect) {
        studioThemeSelect.value = 'parchment';
      }
    }

    // Clear any conflicting inline styles on simulator chassis to allow CSS theme to shine
    if (studioChassis) {
      studioChassis.removeAttribute('style');
    }
  }

  // =========================================================================
  // 2. Real Studio Simulator Interactivity (Figure 2 Matching)
  // =========================================================================
  const studioSearchInput = document.getElementById('studioSearchInput');
  const studioCardsList = document.getElementById('studioCardsList');
  const studioEmptyState = document.getElementById('studioEmptyState');
  const studioSidebar = document.getElementById('studioSidebar');
  const studioTotCount = document.getElementById('studioTotCount');
  const studioThemeSelect = document.getElementById('studioThemeSelect');
  const studioChassis = document.getElementById('studioChassis');

  let currentCategory = 'all';
  let searchQuery = '';

  // Filter Cards Function
  function filterCards() {
    if (!studioCardsList) return;
    const cards = studioCardsList.querySelectorAll('.studio-app-card');
    let visibleCount = 0;

    cards.forEach(card => {
      const cardCat = card.getAttribute('data-cat') || '';
      const isStarred = card.getAttribute('data-star') === 'true';
      const cardTitle = (card.querySelector('.studio-app-title')?.textContent || '').toLowerCase();
      const cardDesc = (card.querySelector('.studio-app-desc')?.textContent || '').toLowerCase();
      const cardTags = (card.getAttribute('data-tags') || '').toLowerCase();

      // Category check
      let matchesCategory = false;
      if (currentCategory === 'all') {
        matchesCategory = true;
      } else if (currentCategory === 'favorites') {
        matchesCategory = isStarred;
      } else {
        matchesCategory = (cardCat === currentCategory);
      }

      // Search query check
      let matchesSearch = true;
      if (searchQuery.trim() !== '') {
        const q = searchQuery.trim().toLowerCase();
        matchesSearch = cardTitle.includes(q) || cardDesc.includes(q) || cardTags.includes(q);
      }

      if (matchesCategory && matchesSearch) {
        card.style.display = 'flex';
        visibleCount++;
      } else {
        card.style.display = 'none';
      }
    });

    if (studioTotCount) {
      studioTotCount.textContent = visibleCount;
    }

    if (studioEmptyState) {
      studioEmptyState.style.display = visibleCount === 0 ? 'block' : 'none';
    }
  }

  // Search Input Event
  if (studioSearchInput) {
    studioSearchInput.addEventListener('input', (e) => {
      searchQuery = e.target.value;
      filterCards();
    });
  }

  // Sidebar Category Click Event
  if (studioSidebar) {
    const catItems = studioSidebar.querySelectorAll('.studio-cat-item');
    catItems.forEach(item => {
      item.addEventListener('click', () => {
        catItems.forEach(i => i.classList.remove('active'));
        item.classList.add('active');
        currentCategory = item.getAttribute('data-cat') || 'all';
        filterCards();
      });
    });
  }

  // Star Toggle
  if (studioCardsList) {
    studioCardsList.addEventListener('click', (e) => {
      const star = e.target.closest('.studio-app-star');
      if (star) {
        const card = star.closest('.studio-app-card');
        const currentlyStarred = card.getAttribute('data-star') === 'true';
        card.setAttribute('data-star', currentlyStarred ? 'false' : 'true');
        star.classList.toggle('starred', !currentlyStarred);
        star.textContent = currentlyStarred ? '☆' : '★';

        // Update favorite count
        const favCountBadge = document.getElementById('catCountFav');
        const starredCards = studioCardsList.querySelectorAll('.studio-app-card[data-star="true"]').length;
        if (favCountBadge) favCountBadge.textContent = starredCards;

        if (currentCategory === 'favorites') {
          filterCards();
        }
      }
    });
  }

  // Studio Internal Theme Selector handled via bi-directional binding in applyWebsiteTheme

  // =========================================================================
  // 3. About & Software Update Center Modal (Figure 1 & Figure 3)
  // =========================================================================
  const studioBtnAbout = document.getElementById('studioBtnAbout');
  const studioAboutModal = document.getElementById('studioAboutModal');
  const studioModalCloseBtn = document.getElementById('studioModalCloseBtn');
  const studioModalCloseBtnBottom = document.getElementById('studioModalCloseBtnBottom');
  const modalTabUpdates = document.getElementById('modalTabUpdates');
  const modalTabChangelog = document.getElementById('modalTabChangelog');
  const modalBodyUpdates = document.getElementById('modalBodyUpdates');
  const modalBodyChangelog = document.getElementById('modalBodyChangelog');
  const modalCheckContainerBtn = document.getElementById('modalCheckContainerBtn');
  const modalContainerStatus = document.getElementById('modalContainerStatus');
  const modalBatchCheckBtn = document.getElementById('modalBatchCheckBtn');

  function openAboutModal() {
    if (studioAboutModal) studioAboutModal.classList.add('open');
  }

  function closeAboutModal() {
    if (studioAboutModal) studioAboutModal.classList.remove('open');
  }

  if (studioBtnAbout) studioBtnAbout.addEventListener('click', openAboutModal);
  if (studioModalCloseBtn) studioModalCloseBtn.addEventListener('click', closeAboutModal);
  if (studioModalCloseBtnBottom) studioModalCloseBtnBottom.addEventListener('click', closeAboutModal);

  if (studioAboutModal) {
    studioAboutModal.addEventListener('click', (e) => {
      if (e.target === studioAboutModal) closeAboutModal();
    });
  }

  // Modal Tab Switching
  if (modalTabUpdates && modalTabChangelog) {
    modalTabUpdates.addEventListener('click', () => {
      modalTabUpdates.classList.add('active');
      modalTabChangelog.classList.remove('active');
      if (modalBodyUpdates) modalBodyUpdates.style.display = 'flex';
      if (modalBodyChangelog) modalBodyChangelog.style.display = 'none';
    });

    modalTabChangelog.addEventListener('click', () => {
      modalTabChangelog.classList.add('active');
      modalTabUpdates.classList.remove('active');
      if (modalBodyUpdates) modalBodyUpdates.style.display = 'none';
      if (modalBodyChangelog) modalBodyChangelog.style.display = 'flex';
    });
  }

  // Check Container Update Simulation
  if (modalCheckContainerBtn && modalContainerStatus) {
    modalCheckContainerBtn.addEventListener('click', () => {
      modalCheckContainerBtn.disabled = true;
      modalCheckContainerBtn.textContent = '正在检查...';
      modalContainerStatus.textContent = '正在连接 GitHub Release 服务器查询最新版本元数据...';
      setTimeout(() => {
        modalCheckContainerBtn.disabled = false;
        modalCheckContainerBtn.textContent = '🔄 检查更新';
        modalContainerStatus.textContent = '🟢 恭喜！当前 SmileCode Studio 容器已是最新稳定版 (v2.3.1)。';
      }, 1000);
    });
  }

  // Batch Check Plugins Update Simulation
  if (modalBatchCheckBtn) {
    modalBatchCheckBtn.addEventListener('click', () => {
      modalBatchCheckBtn.disabled = true;
      modalBatchCheckBtn.textContent = '正在批量检查...';
      const statusLabels = document.querySelectorAll('.modal-item-status');
      statusLabels.forEach(s => s.textContent = '🔄 检查中...');
      setTimeout(() => {
        statusLabels.forEach(s => s.textContent = '🟢 已是最新');
        modalBatchCheckBtn.disabled = false;
        modalBatchCheckBtn.textContent = '一键检查所有插件更新';
      }, 900);
    });
  }

  // Extension Center Button
  const studioBtnExt = document.getElementById('studioBtnExt');
  if (studioBtnExt) {
    studioBtnExt.addEventListener('click', () => {
      alert('🌸 SmileCode Studio 扩展中心已就绪！\n支持导入 .dll 格式的 Qt 动态插件，或一键从应用市场加载。');
    });
  }

  // Navbar Scroll Shadow
  const navbar = document.getElementById('navbar');
  window.addEventListener('scroll', () => {
    if (window.scrollY > 30) {
      if (navbar) navbar.style.boxShadow = '0 4px 20px rgba(0,0,0,0.08)';
    } else {
      if (navbar) navbar.style.boxShadow = 'none';
    }
  });
});
