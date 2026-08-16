#ifndef SCOPETHEME_H
#define SCOPETHEME_H

#include <QString>
#include <QStringList>
#include <QColor>
#include <QVector>

namespace ScopeTheme {

// 示波器 7 大主流主题枚举
enum ThemeType {
  CyberNeon = 0,     // 🌌 极客霓虹 (赛博朋克深黑 + 荧光青蓝)
  KeysightDark,      // ⚡ 专业仪器 (是德/泰克高端仪器经典深灰)
  ModernSlate,       // 💻 现代暗黑 (VS Code / Modern Slate 极简石板黑)
  DeepOcean,         // 🌊 浩瀚深海 (沉浸深海夜蓝 + 碧蓝湖绿)
  CleanLight,        // ☀️ 现代实验室 (科研明亮浅灰白 + 高对比度清晰波形)
  RetroCrt,          // 🔮 复古荧光绿 (经典阴极射线管模拟示波器)
  MoltenEmber        // 🌋 熔岩烈焰 (黑曜石黑金 + 炽热熔岩橙)
};

// 示波器全套调色板结构体
struct ScopePalette {
  QString name;           // 主题名称
  QString icon;           // 图标符号
  
  // 窗口与面板基调
  QString windowBg;       // 主窗口背景
  QString panelBg;        // 工具栏/Dock标题/菜单背景
  QString dockWidgetBg;   // Dock 内容区背景
  QString baseBg;         // 表格/输入框/下拉框背景
  QString baseAltBg;      // 表格交替行背景
  QString plotBg;         // QCustomPlot 波形图表背景
  QString fftBg;          // FFT 频谱图表背景
  
  // 边框与分割线
  QString border;         // 常规边框颜色
  QString borderDark;     // 深色分割线/凹槽
  QString borderLight;    // 高亮微边框
  QString splitterHandle; // 分割器手柄颜色
  
  // 文本
  QString textMain;       // 主要文本
  QString textSub;        // 次要/注释文本
  QString textDisabled;   // 禁用文本
  
  // 主题强调色
  QString accent;         // 强调色 (按钮/选中/边框聚焦)
  QString accentHover;    // 强调色悬停
  QString accentPressed;  // 强调色按下
  QString accentText;     // 强调色上的文字颜色
  
  // 选项卡 & 表头
  QString tabBg;          // 标签栏普通标签背景
  QString tabSelectedBg;  // 标签栏选中背景
  QString headerBg;       // 表格表头背景
  QString cornerBg;       // 表格左上角及滚动角标背景 (消灭白块)
  
  // 滚动条与控件
  QString scrollTrack;    // 滚动条滑道背景
  QString scrollThumb;    // 滚动条滑块背景
  QString scrollThumbHover;// 滚动条滑块悬停
  
  // 图表元素 (QColor 格式方便 QCustomPlot 使用)
  QColor plotBgColor;     // 图表背景 QColor
  QColor axisPenColor;    // 坐标轴轴线颜色
  QColor tickPenColor;    // 刻度线颜色
  QColor subTickPenColor; // 子刻度线颜色
  QColor tickLabelColor;  // 刻度数值文字颜色
  QColor axisLabelColor;  // 轴标题文字颜色
  QColor gridPenColor;    // 主网格线颜色
  QColor subGridPenColor; // 子网格线颜色
  QColor cursorXColor;    // 时间游标颜色
  QColor cursorYColor;    // 电压游标颜色
  QColor triggerLineColor;// 触发电平虚线颜色
  
  // 16 通道预设波形色彩
  QVector<QColor> channelColors;
};

// 获取主题信息列表
inline QStringList themeNames() {
  return {
    QStringLiteral("🌌 极客霓虹 (Cyber Neon)"),
    QStringLiteral("⚡ 专业仪器 (Keysight / Tek)"),
    QStringLiteral("💻 现代暗黑 (Modern Slate)"),
    QStringLiteral("🌊 浩瀚深海 (Deep Ocean)"),
    QStringLiteral("☀️ 现代实验室 (Clean Light)"),
    QStringLiteral("🔮 复古荧光绿 (Retro CRT)"),
    QStringLiteral("🌋 熔岩烈焰 (Molten Ember)")
  };
}

// 获取具体主题调色板
inline ScopePalette paletteFor(ThemeType type) {
  ScopePalette p;
  
  switch (type) {
  case CyberNeon: { // 🌌 极客霓虹 (Cyber Neon)
    p.name = QStringLiteral("极客霓虹");
    p.icon = QStringLiteral("🌌");
    p.windowBg = "#0B0F19";
    p.panelBg = "#111827";
    p.dockWidgetBg = "#0F172A";
    p.baseBg = "#090D16";
    p.baseAltBg = "#0F172A";
    p.plotBg = "#060911";
    p.fftBg = "#060911";
    p.border = "#1F2937";
    p.borderDark = "#111827";
    p.borderLight = "#374151";
    p.splitterHandle = "#1E293B";
    p.textMain = "#E2E8F0";
    p.textSub = "#94A3B8";
    p.textDisabled = "#4B5563";
    p.accent = "#00E5FF";
    p.accentHover = "#38BDF8";
    p.accentPressed = "#00B4D8";
    p.accentText = "#0B0F19";
    p.tabBg = "#1E293B";
    p.tabSelectedBg = "#0B0F19";
    p.headerBg = "#111827";
    p.cornerBg = "#111827";
    p.scrollTrack = "#0B0F19";
    p.scrollThumb = "#1E293B";
    p.scrollThumbHover = "#00E5FF";
    
    p.plotBgColor = QColor("#060911");
    p.axisPenColor = QColor("#334155");
    p.tickPenColor = QColor("#64748B");
    p.subTickPenColor = QColor("#475569");
    p.tickLabelColor = QColor("#94A3B8");
    p.axisLabelColor = QColor("#38BDF8");
    p.gridPenColor = QColor("#1E293B");
    p.subGridPenColor = QColor("#111827");
    p.cursorXColor = QColor("#FFD700");
    p.cursorYColor = QColor("#00E5FF");
    p.triggerLineColor = QColor("#FF1744");
    
    p.channelColors = {
      QColor("#00E5FF"), QColor("#FFD700"), QColor("#00E676"), QColor("#FF5252"),
      QColor("#E040FB"), QColor("#FF9100"), QColor("#40C4FF"), QColor("#EEFF41"),
      QColor("#FF4081"), QColor("#7C4DFF"), QColor("#1DE9B6"), QColor("#FF6E40"),
      QColor("#B388FF"), QColor("#69F0AE"), QColor("#FFAB40"), QColor("#EA80FC")
    };
    break;
  }
  
  case KeysightDark: { // ⚡ 专业仪器 (Keysight / Tektronix 经典铁灰)
    p.name = QStringLiteral("专业仪器");
    p.icon = QStringLiteral("⚡");
    p.windowBg = "#1C2028";
    p.panelBg = "#232833";
    p.dockWidgetBg = "#1C2028";
    p.baseBg = "#161920";
    p.baseAltBg = "#1E222B";
    p.plotBg = "#12141A";
    p.fftBg = "#12141A";
    p.border = "#333A48";
    p.borderDark = "#191D24";
    p.borderLight = "#434B5C";
    p.splitterHandle = "#2E3442";
    p.textMain = "#E6EDF3";
    p.textSub = "#8B949E";
    p.textDisabled = "#484F58";
    p.accent = "#0284C7";
    p.accentHover = "#38BDF8";
    p.accentPressed = "#0369A1";
    p.accentText = "#FFFFFF";
    p.tabBg = "#282E3B";
    p.tabSelectedBg = "#1C2028";
    p.headerBg = "#232833";
    p.cornerBg = "#232833";
    p.scrollTrack = "#1C2028";
    p.scrollThumb = "#333A48";
    p.scrollThumbHover = "#0284C7";
    
    p.plotBgColor = QColor("#12141A");
    p.axisPenColor = QColor("#434B5C");
    p.tickPenColor = QColor("#768390");
    p.subTickPenColor = QColor("#545D68");
    p.tickLabelColor = QColor("#ADBAC7");
    p.axisLabelColor = QColor("#38BDF8");
    p.gridPenColor = QColor("#282E3B");
    p.subGridPenColor = QColor("#1C2028");
    p.cursorXColor = QColor("#F59E0B");
    p.cursorYColor = QColor("#06B6D4");
    p.triggerLineColor = QColor("#EF4444");
    
    p.channelColors = {
      QColor("#FACC15"), QColor("#22C55E"), QColor("#38BDF8"), QColor("#F43F5E"),
      QColor("#A855F7"), QColor("#FB923C"), QColor("#2DD4BF"), QColor("#E879F9"),
      QColor("#FDE047"), QColor("#4ADE80"), QColor("#60A5FA"), QColor("#FB7185"),
      QColor("#C084FC"), QColor("#FDBA74"), QColor("#5EEAD4"), QColor("#F472B6")
    };
    break;
  }

  case ModernSlate: { // 💻 现代暗黑 (Modern Slate Dark)
    p.name = QStringLiteral("现代暗黑");
    p.icon = QStringLiteral("💻");
    p.windowBg = "#18181B";
    p.panelBg = "#27272A";
    p.dockWidgetBg = "#18181B";
    p.baseBg = "#09090B";
    p.baseAltBg = "#141417";
    p.plotBg = "#0D0D10";
    p.fftBg = "#0D0D10";
    p.border = "#3F3F46";
    p.borderDark = "#18181B";
    p.borderLight = "#52525B";
    p.splitterHandle = "#27272A";
    p.textMain = "#F4F4F5";
    p.textSub = "#A1A1AA";
    p.textDisabled = "#52525B";
    p.accent = "#6366F1";
    p.accentHover = "#818CF8";
    p.accentPressed = "#4F46E5";
    p.accentText = "#FFFFFF";
    p.tabBg = "#27272A";
    p.tabSelectedBg = "#18181B";
    p.headerBg = "#27272A";
    p.cornerBg = "#27272A";
    p.scrollTrack = "#18181B";
    p.scrollThumb = "#3F3F46";
    p.scrollThumbHover = "#6366F1";
    
    p.plotBgColor = QColor("#0D0D10");
    p.axisPenColor = QColor("#3F3F46");
    p.tickPenColor = QColor("#71717A");
    p.subTickPenColor = QColor("#52525B");
    p.tickLabelColor = QColor("#A1A1AA");
    p.axisLabelColor = QColor("#818CF8");
    p.gridPenColor = QColor("#222226");
    p.subGridPenColor = QColor("#16161A");
    p.cursorXColor = QColor("#FBBF24");
    p.cursorYColor = QColor("#818CF8");
    p.triggerLineColor = QColor("#F43F5E");
    
    p.channelColors = {
      QColor("#818CF8"), QColor("#34D399"), QColor("#F472B6"), QColor("#38BDF8"),
      QColor("#FBBF24"), QColor("#A78BFA"), QColor("#FB923C"), QColor("#4ADE80"),
      QColor("#60A5FA"), QColor("#F87171"), QColor("#C084FC"), QColor("#2DD4BF"),
      QColor("#FCD34D"), QColor("#93C5FD"), QColor("#FDA4AF"), QColor("#86EFAC")
    };
    break;
  }

  case DeepOcean: { // 🌊 浩瀚深海 (Deep Ocean)
    p.name = QStringLiteral("浩瀚深海");
    p.icon = QStringLiteral("🌊");
    p.windowBg = "#0A1128";
    p.panelBg = "#0E1A3D";
    p.dockWidgetBg = "#0A1128";
    p.baseBg = "#050917";
    p.baseAltBg = "#0A122B";
    p.plotBg = "#040713";
    p.fftBg = "#040713";
    p.border = "#1B2A56";
    p.borderDark = "#080E21";
    p.borderLight = "#253A75";
    p.splitterHandle = "#13234E";
    p.textMain = "#E0F2FE";
    p.textSub = "#7DD3FC";
    p.textDisabled = "#334E68";
    p.accent = "#00B4D8";
    p.accentHover = "#48CAE4";
    p.accentPressed = "#0077B6";
    p.accentText = "#03071E";
    p.tabBg = "#13234E";
    p.tabSelectedBg = "#0A1128";
    p.headerBg = "#0E1A3D";
    p.cornerBg = "#0E1A3D";
    p.scrollTrack = "#0A1128";
    p.scrollThumb = "#1B2A56";
    p.scrollThumbHover = "#00B4D8";
    
    p.plotBgColor = QColor("#040713");
    p.axisPenColor = QColor("#1B2A56");
    p.tickPenColor = QColor("#38BDF8");
    p.subTickPenColor = QColor("#1E3A8A");
    p.tickLabelColor = QColor("#7DD3FC");
    p.axisLabelColor = QColor("#38BDF8");
    p.gridPenColor = QColor("#102047");
    p.subGridPenColor = QColor("#081126");
    p.cursorXColor = QColor("#FBBF24");
    p.cursorYColor = QColor("#00E5FF");
    p.triggerLineColor = QColor("#FF0055");
    
    p.channelColors = {
      QColor("#00F0FF"), QColor("#00FF87"), QColor("#FF007F"), QColor("#FFE600"),
      QColor("#7000FF"), QColor("#00B4D8"), QColor("#FF6B00"), QColor("#72EFDD"),
      QColor("#48CAE4"), QColor("#52B788"), QColor("#F72585"), QColor("#FFD166"),
      QColor("#B5179E"), QColor("#4CC9F0"), QColor("#F3722C"), QColor("#90BE6D")
    };
    break;
  }

  case CleanLight: { // ☀️ 现代实验室 (Clean Light Lab 浅色高对比)
    p.name = QStringLiteral("现代实验室");
    p.icon = QStringLiteral("☀️");
    p.windowBg = "#F1F5F9";
    p.panelBg = "#FFFFFF";
    p.dockWidgetBg = "#F8FAFC";
    p.baseBg = "#FFFFFF";
    p.baseAltBg = "#F1F5F9";
    p.plotBg = "#FFFFFF";
    p.fftBg = "#FFFFFF";
    p.border = "#CBD5E1";
    p.borderDark = "#94A3B8";
    p.borderLight = "#E2E8F0";
    p.splitterHandle = "#CBD5E1";
    p.textMain = "#0F172A";
    p.textSub = "#475569";
    p.textDisabled = "#94A3B8";
    p.accent = "#2563EB";
    p.accentHover = "#3B82F6";
    p.accentPressed = "#1D4ED8";
    p.accentText = "#FFFFFF";
    p.tabBg = "#E2E8F0";
    p.tabSelectedBg = "#FFFFFF";
    p.headerBg = "#E2E8F0";
    p.cornerBg = "#E2E8F0";
    p.scrollTrack = "#F1F5F9";
    p.scrollThumb = "#CBD5E1";
    p.scrollThumbHover = "#2563EB";
    
    p.plotBgColor = QColor("#FFFFFF");
    p.axisPenColor = QColor("#64748B");
    p.tickPenColor = QColor("#475569");
    p.subTickPenColor = QColor("#94A3B8");
    p.tickLabelColor = QColor("#1E293B");
    p.axisLabelColor = QColor("#2563EB");
    p.gridPenColor = QColor("#E2E8F0");
    p.subGridPenColor = QColor("#F8FAFC");
    p.cursorXColor = QColor("#D97706");
    p.cursorYColor = QColor("#2563EB");
    p.triggerLineColor = QColor("#DC2626");
    
    p.channelColors = {
      QColor("#2563EB"), QColor("#DC2626"), QColor("#059669"), QColor("#D97706"),
      QColor("#7C3AED"), QColor("#0891B2"), QColor("#DB2777"), QColor("#4F46E5"),
      QColor("#16A34A"), QColor("#EA580C"), QColor("#9333EA"), QColor("#0284C7"),
      QColor("#E11D48"), QColor("#65A30D"), QColor("#C026D3"), QColor("#475569")
    };
    break;
  }

  case RetroCrt: { // 🔮 复古荧光绿 (Retro CRT Phosphor)
    p.name = QStringLiteral("复古荧光绿");
    p.icon = QStringLiteral("🔮");
    p.windowBg = "#0A160C";
    p.panelBg = "#102213";
    p.dockWidgetBg = "#0A160C";
    p.baseBg = "#050D06";
    p.baseAltBg = "#0A170C";
    p.plotBg = "#030A04";
    p.fftBg = "#030A04";
    p.border = "#1C3B20";
    p.borderDark = "#0C1D0E";
    p.borderLight = "#28542E";
    p.splitterHandle = "#17301B";
    p.textMain = "#86EFAC";
    p.textSub = "#4ADE80";
    p.textDisabled = "#22542B";
    p.accent = "#00FF66";
    p.accentHover = "#33FF85";
    p.accentPressed = "#00CC52";
    p.accentText = "#030A04";
    p.tabBg = "#17301B";
    p.tabSelectedBg = "#0A160C";
    p.headerBg = "#102213";
    p.cornerBg = "#102213";
    p.scrollTrack = "#0A160C";
    p.scrollThumb = "#1C3B20";
    p.scrollThumbHover = "#00FF66";
    
    p.plotBgColor = QColor("#030A04");
    p.axisPenColor = QColor("#22542B");
    p.tickPenColor = QColor("#4ADE80");
    p.subTickPenColor = QColor("#1C3B20");
    p.tickLabelColor = QColor("#86EFAC");
    p.axisLabelColor = QColor("#00FF66");
    p.gridPenColor = QColor("#122916");
    p.subGridPenColor = QColor("#08170A");
    p.cursorXColor = QColor("#EAB308");
    p.cursorYColor = QColor("#00FF66");
    p.triggerLineColor = QColor("#EF4444");
    
    p.channelColors = {
      QColor("#00FF66"), QColor("#22C55E"), QColor("#86EFAC"), QColor("#FDE047"),
      QColor("#38BDF8"), QColor("#FB923C"), QColor("#A78BFA"), QColor("#F472B6"),
      QColor("#4ADE80"), QColor("#34D399"), QColor("#FACC15"), QColor("#60A5FA"),
      QColor("#E879F9"), QColor("#F87171"), QColor("#2DD4BF"), QColor("#A3E635")
    };
    break;
  }

  case MoltenEmber: { // 🌋 熔岩烈焰 (Molten Ember)
    p.name = QStringLiteral("熔岩烈焰");
    p.icon = QStringLiteral("🌋");
    p.windowBg = "#171412";
    p.panelBg = "#241E1A";
    p.dockWidgetBg = "#171412";
    p.baseBg = "#0C0A09";
    p.baseAltBg = "#1C1714";
    p.plotBg = "#090807";
    p.fftBg = "#090807";
    p.border = "#3D2E26";
    p.borderDark = "#1A1410";
    p.borderLight = "#543F34";
    p.splitterHandle = "#2D221C";
    p.textMain = "#FED7AA";
    p.textSub = "#FB923C";
    p.textDisabled = "#574236";
    p.accent = "#F97316";
    p.accentHover = "#FB923C";
    p.accentPressed = "#EA580C";
    p.accentText = "#0C0A09";
    p.tabBg = "#2D221C";
    p.tabSelectedBg = "#171412";
    p.headerBg = "#241E1A";
    p.cornerBg = "#241E1A";
    p.scrollTrack = "#171412";
    p.scrollThumb = "#3D2E26";
    p.scrollThumbHover = "#F97316";
    
    p.plotBgColor = QColor("#090807");
    p.axisPenColor = QColor("#443228");
    p.tickPenColor = QColor("#F97316");
    p.subTickPenColor = QColor("#2E2018");
    p.tickLabelColor = QColor("#FDBA74");
    p.axisLabelColor = QColor("#FB923C");
    p.gridPenColor = QColor("#221813");
    p.subGridPenColor = QColor("#140E0B");
    p.cursorXColor = QColor("#FACC15");
    p.cursorYColor = QColor("#FB923C");
    p.triggerLineColor = QColor("#EF4444");
    
    p.channelColors = {
      QColor("#F97316"), QColor("#FACC15"), QColor("#EF4444"), QColor("#EC4899"),
      QColor("#A855F7"), QColor("#38BDF8"), QColor("#22C55E"), QColor("#FB923C"),
      QColor("#F43F5E"), QColor("#FDE047"), QColor("#D946EF"), QColor("#06B6D4"),
      QColor("#4ADE80"), QColor("#E11D48"), QColor("#FB7185"), QColor("#FCD34D")
    };
    break;
  }
  }

  return p;
}

// 生成全局零白块一体化 QSS 样式表
inline QString generateStyleSheet(ThemeType type) {
  const ScopePalette p = paletteFor(type);
  
  QString qss = QStringLiteral(R"QSS(
/* 全局基础设置 */
QMainWindow {
  background-color: %1;
  color: %2;
  font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;
  font-size: 12px;
}
QWidget {
  color: %2;
  font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;
  font-size: 12px;
}

/* 主工具栏 */
QToolBar {
  background-color: %3;
  border-bottom: 1px solid %4;
  spacing: 6px;
  padding: 4px 8px;
}
QToolBar::separator {
  background-color: %4;
  width: 1px;
  margin: 4px 4px;
}

/* 分割器 Splitter (消除手柄白点) */
QSplitter {
  background-color: %1;
}
QSplitter::handle {
  background-color: %5;
}
QSplitter::handle:horizontal {
  width: 3px;
}
QSplitter::handle:vertical {
  height: 3px;
}
QMainWindow::separator {
  background-color: %5;
  width: 3px;
  height: 3px;
}
QMainWindow::separator:hover {
  background-color: %6;
}

/* 停靠窗口 DockWidget */
QDockWidget {
  titlebar-close-icon: url();
  titlebar-normal-icon: url();
  color: %6;
  font-weight: bold;
  font-size: 12px;
  background-color: %7;
}
QDockWidget::title {
  background-color: %3;
  border: 1px solid %4;
  padding: 6px 8px;
  text-align: left;
  color: %6;
}
QDockWidget::close-button, QDockWidget::float-button {
  background-color: transparent;
  border: none;
  padding: 2px;
}
QDockWidget::close-button:hover, QDockWidget::float-button:hover {
  background-color: %4;
  border-radius: 3px;
}

/* 选项卡 TabWidget (消除溢出翻页白块) */
QTabWidget::pane {
  border: 1px solid %4;
  background: %7;
  top: -1px;
}
QTabBar {
  background: %3;
}
QTabBar::tab {
  background: %8;
  color: %9;
  padding: 6px 14px;
  border: 1px solid %4;
  border-bottom: none;
  border-top-left-radius: 4px;
  border-top-right-radius: 4px;
  font-size: 12px;
  margin-right: 2px;
}
QTabBar::tab:selected {
  background: %10;
  color: %6;
  font-weight: bold;
  border-color: %6;
  border-bottom: 2px solid %6;
}
QTabBar::tab:hover:!selected {
  background: %4;
  color: %2;
}
/* 消除选项卡翻页按钮白块 */
QTabBar QToolButton {
  background-color: %3;
  color: %2;
  border: 1px solid %4;
  border-radius: 3px;
}
QTabBar QToolButton:hover {
  background-color: %6;
  color: %11;
}

/* 表格控件 TableWidget (彻底消灭左上角纯白角标与交界角标) */
QTableWidget, QTableView {
  background-color: %12;
  alternate-background-color: %13;
  color: %2;
  gridline-color: %4;
  border: 1px solid %4;
  font-size: 12px;
  selection-background-color: %5;
  selection-color: %2;
  outline: 0;
}
QHeaderView {
  background-color: %14;
  border: none;
}
QHeaderView::section {
  background-color: %14;
  color: %6;
  font-weight: bold;
  padding: 5px 6px;
  border: 1px solid %4;
  font-size: 12px;
}
/* 核心：修复表格左上角白方块 */
QTableCornerButton::section {
  background-color: %15;
  border: 1px solid %4;
}
/* 核心：修复滚动条交界右下角白方块 */
QAbstractScrollArea::corner {
  background-color: %15;
  border: none;
}

/* 全局滚动条 ScrollBar (消灭所有原生白色箭头与白底) */
QScrollBar:vertical {
  background-color: %16;
  width: 9px;
  margin: 0px;
  border: none;
}
QScrollBar::handle:vertical {
  background-color: %17;
  border-radius: 4px;
  min-height: 24px;
  margin: 1px;
}
QScrollBar::handle:vertical:hover {
  background-color: %6;
}
QScrollBar:horizontal {
  background-color: %16;
  height: 9px;
  margin: 0px;
  border: none;
}
QScrollBar::handle:horizontal {
  background-color: %17;
  border-radius: 4px;
  min-width: 24px;
  margin: 1px;
}
QScrollBar::handle:horizontal:hover {
  background-color: %6;
}
QScrollBar::add-line, QScrollBar::sub-line {
  background: transparent;
  border: none;
  width: 0px;
  height: 0px;
}
QScrollBar::add-page, QScrollBar::sub-page {
  background: transparent;
}

/* 分组框 GroupBox */
QGroupBox {
  font-weight: bold;
  color: %6;
  border: 1px solid %4;
  border-radius: 6px;
  margin-top: 10px;
  padding-top: 12px;
  font-size: 12px;
  background-color: transparent;
}
QGroupBox::title {
  subcontrol-origin: margin;
  subcontrol-position: top left;
  left: 10px;
  padding: 0 5px;
  background-color: transparent;
}

/* 标签 Label */
QLabel {
  color: %9;
  font-size: 12px;
  background: transparent;
}

/* 输入框与选择框 */
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
  background-color: %12;
  color: %2;
  border: 1px solid %4;
  border-radius: 4px;
  padding: 4px 8px;
  font-size: 12px;
  min-height: 20px;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
  border: 1px solid %6;
}
QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled {
  background-color: %4;
  color: %18;
}

/* 下拉框 ComboBox 内部弹窗 */
QComboBox::drop-down {
  border: none;
  width: 18px;
  subcontrol-position: right center;
}
QComboBox QAbstractItemView {
  background-color: %3;
  color: %2;
  border: 1px solid %6;
  selection-background-color: %6;
  selection-color: %11;
  outline: 0;
  padding: 4px;
}

/* 数字微调框上下按钮消除白底 */
QSpinBox::up-button, QDoubleSpinBox::up-button {
  background-color: %3;
  border-left: 1px solid %4;
  border-bottom: 1px solid %4;
  width: 14px;
}
QSpinBox::down-button, QDoubleSpinBox::down-button {
  background-color: %3;
  border-left: 1px solid %4;
  width: 14px;
}
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
  background-color: %6;
}

/* 按钮 PushButton */
QPushButton {
  background-color: %8;
  color: %6;
  border: 1px solid %6;
  border-radius: 4px;
  padding: 5px 12px;
  font-weight: bold;
  font-size: 12px;
}
QPushButton:hover {
  background-color: %6;
  color: %11;
}
QPushButton:pressed {
  background-color: %19;
  color: %11;
}
QPushButton:disabled {
  background-color: %4;
  color: %18;
  border-color: %4;
}

/* 复选框 CheckBox (消除表格内白框) */
QCheckBox {
  color: %2;
  spacing: 5px;
  background: transparent;
}
QCheckBox::indicator {
  width: 14px;
  height: 14px;
  border: 1px solid %4;
  border-radius: 3px;
  background-color: %12;
}
QCheckBox::indicator:checked {
  background-color: %6;
  border-color: %6;
}

/* 单选框 RadioButton */
QRadioButton {
  color: %2;
  spacing: 5px;
  background: transparent;
}
QRadioButton::indicator {
  width: 14px;
  height: 14px;
  border: 1px solid %4;
  border-radius: 7px;
  background-color: %12;
}
QRadioButton::indicator:checked {
  background-color: %6;
  border-color: %6;
}

/* 状态栏 StatusBar */
QStatusBar {
  background-color: %1;
  color: %9;
  border-top: 1px solid %4;
  font-size: 12px;
}

/* 文本日志框 */
QTextEdit, QPlainTextEdit {
  background-color: %12;
  color: %2;
  border: 1px solid %4;
  border-radius: 4px;
  font-family: 'Consolas', 'Courier New', monospace;
  font-size: 11px;
}
)QSS")
  .arg(p.windowBg)         // %1
  .arg(p.textMain)         // %2
  .arg(p.panelBg)          // %3
  .arg(p.border)           // %4
  .arg(p.splitterHandle)   // %5
  .arg(p.accent)           // %6
  .arg(p.dockWidgetBg)     // %7
  .arg(p.tabBg)            // %8
  .arg(p.textSub)          // %9
  .arg(p.tabSelectedBg)    // %10
  .arg(p.accentText)       // %11
  .arg(p.baseBg)           // %12
  .arg(p.baseAltBg)        // %13
  .arg(p.headerBg)         // %14
  .arg(p.cornerBg)         // %15
  .arg(p.scrollTrack)      // %16
  .arg(p.scrollThumb)      // %17
  .arg(p.textDisabled)     // %18
  .arg(p.accentPressed);   // %19

  return qss;
}

} // namespace ScopeTheme

#endif // SCOPETHEME_H
