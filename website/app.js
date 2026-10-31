/**
 * SmileCodeIDE Official Website Scripts
 * Dynamic interactivity: Live Optimizer Preview, FAQ Accordion, Theme switcher
 */

document.addEventListener('DOMContentLoaded', () => {
  // 1. Navbar Scroll Effect
  const navbar = document.getElementById('navbar');
  window.addEventListener('scroll', () => {
    if (window.scrollY > 40) {
      navbar.style.background = 'rgba(7, 10, 18, 0.95)';
      navbar.style.boxShadow = '0 10px 30px rgba(0, 0, 0, 0.5)';
    } else {
      navbar.style.background = 'rgba(7, 10, 18, 0.82)';
      navbar.style.boxShadow = 'none';
    }
  });

  // 2. Optimization Interactive Showcase Data & Logic
  const optData = {
    '-O0': {
      title: '【-O0 无优化 · 完整单步调试】',
      content: '关闭所有优化。生成的机器码与 C 语言源码语句严格一一对应，编译器绝不调整指令次序或清除局部变量。适合单步断点逐行调试、分析硬件寄存器时序及初学者排查疑难杂症。',
      tags: ['单步调试', '变量完好', '无指令重排', '体积较大']
    },
    '-O1': {
      title: '【-O1 基础优化 · 轻度提速】',
      content: '开启基础的代码流与基础表达式优化，减少代码体积并提升执行速度，同时不花费大量编译时间。适合开发中期日常编译验证。',
      tags: ['快速构建', '基础内联', '轻微提速']
    },
    '-O2': {
      title: '【-O2 推荐平衡优化 · 官方发布首选】',
      content: '官方推荐用于正式发布版本。在编译速度、代码执行性能与生成固件大小之间取得最佳平衡。开启几乎所有安全的指令调度与循环优化。',
      tags: ['流水线优化', '内联扩展', '生产部署首选', '高度平衡']
    },
    '-O3': {
      title: '【-O3 极致性能 · 算力优先】',
      content: '在 -O2 基础上增加激进的循环展开 (loop unrolling)、函数向量化与更激进的内联策略。生成更庞大的二进制文件以换取微秒级的极致运行速度，适合数字信号处理 (DSP)、音频与数学算法。',
      tags: ['算力爆发', '循环展开', 'SIMD/向量化', 'DSP算法推荐']
    },
    '-Os': {
      title: '【-Os 极小代码尺寸 · 资源敏感】',
      content: '专为微控制器 (MCU) 严苛的内部 Flash 容量设计。关闭所有可能增加代码体积的优化（如内联与循环展开），适合 Flash 资源极度紧张的小容量单片机。',
      tags: ['代码极小', '节省Flash', '小容量MCU', '资源紧凑']
    },
    '-Og': {
      title: '【-Og 调试友好优化 · 现代推荐】',
      content: '专门为标准“编辑-编译-调试”工作流设计的优化级别。在保留大部分代码优化的同时，确保调试信息的完整性与断点位置的准确性，兼得性能与可调试性。',
      tags: ['调试推荐', '保留断点', '连贯体验', '现代GCC特性']
    }
  };

  let currentOpt = '-O2';
  const optButtons = document.querySelectorAll('.opt-choice-btn');
  const chkDwarf2 = document.getElementById('chkDwarf2');
  const chkPermissive = document.getElementById('chkPermissive');
  const liveFlags = document.getElementById('liveGeneratedFlags');
  const dynamicTitle = document.getElementById('dynamicInfoTitle');
  const dynamicContent = document.getElementById('dynamicInfoContent');
  const dynamicTags = document.getElementById('dynamicInfoTags');
  const btnCopyFlags = document.getElementById('btnCopyFlags');

  function updateLivePreview() {
    let flags = `CFLAGS += ${currentOpt}`;
    if (chkDwarf2.checked) {
      flags += ' -g -gdwarf-2';
    }
    if (chkPermissive.checked) {
      flags += ' -fcommon';
    }
    liveFlags.textContent = flags;

    const info = optData[currentOpt] || optData['-O2'];
    dynamicTitle.textContent = info.title;
    dynamicContent.textContent = info.content;

    dynamicTags.innerHTML = '';
    info.tags.forEach(tag => {
      const span = document.createElement('span');
      span.className = 'info-badge';
      span.textContent = tag;
      dynamicTags.appendChild(span);
    });
  }

  optButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      optButtons.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      currentOpt = btn.getAttribute('data-opt');
      updateLivePreview();
    });
  });

  chkDwarf2.addEventListener('change', updateLivePreview);
  chkPermissive.addEventListener('change', updateLivePreview);

  // Copy Flags to Clipboard
  btnCopyFlags.addEventListener('click', () => {
    const textToCopy = liveFlags.textContent;
    navigator.clipboard.writeText(textToCopy).then(() => {
      const originalText = btnCopyFlags.textContent;
      btnCopyFlags.textContent = '已复制！';
      btnCopyFlags.style.borderColor = '#10b981';
      btnCopyFlags.style.color = '#34d399';
      setTimeout(() => {
        btnCopyFlags.textContent = originalText;
        btnCopyFlags.style.borderColor = '';
        btnCopyFlags.style.color = '';
      }, 2000);
    });
  });

  // 3. Screenshot Dark / Light Theme Tab Switcher
  const themeTabBtns = document.querySelectorAll('.theme-tab-btn');
  const screenshotImg = document.getElementById('dialogScreenshot');

  themeTabBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      themeTabBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      const theme = btn.getAttribute('data-img');
      screenshotImg.style.opacity = '0.3';
      setTimeout(() => {
        if (theme === 'dark') {
          screenshotImg.src = 'assets/toolchain_config_dialog_dark.png';
          screenshotImg.alt = '工具链与编译器优化配置界面 (Dark)';
        } else {
          screenshotImg.src = 'assets/toolchain_config_dialog_light.png';
          screenshotImg.alt = '工具链与编译器优化配置界面 (Light)';
        }
        screenshotImg.style.opacity = '1';
      }, 150);
    });
  });

  // 4. FAQ Accordion
  const faqItems = document.querySelectorAll('.faq-item');
  faqItems.forEach(item => {
    const questionBtn = item.querySelector('.faq-question');
    questionBtn.addEventListener('click', () => {
      const isActive = item.classList.contains('active');
      // Collapse others
      faqItems.forEach(other => {
        if (other !== item) other.classList.remove('active');
      });
      // Toggle current
      item.classList.toggle('active', !isActive);
    });
  });

  // 5. Mobile Navigation Toggle
  const mobileToggle = document.getElementById('mobileToggle');
  const navLinks = document.querySelector('.nav-links');
  if (mobileToggle) {
    mobileToggle.addEventListener('click', () => {
      if (navLinks.style.display === 'flex') {
        navLinks.style.display = 'none';
      } else {
        navLinks.style.display = 'flex';
        navLinks.style.flexDirection = 'column';
        navLinks.style.position = 'absolute';
        navLinks.style.top = '72px';
        navLinks.style.left = '0';
        navLinks.style.right = '0';
        navLinks.style.background = '#070a12';
        navLinks.style.padding = '20px';
        navLinks.style.borderBottom = '1px solid rgba(255, 255, 255, 0.1)';
      }
    });
  }

  // Initial update
  updateLivePreview();
});

