#ifndef IDETHEME_H
#define IDETHEME_H

#include <QString>
#include <QStringList>
#include <QColor>
#include <QVector>

namespace IdeTheme {

// 旗舰级主题调色板
struct ThemePalette {
  QString id;             // 主题ID
  QString displayName;    // 显示名称
  QString icon;           // 图标表情
  bool isDark = true;     // 是否深色主题

  // 基础与分层背景
  QString windowBg;       // 主窗口底色
  QString windowBgGrad;   // 主窗口微渐变
  QString panelBg;        // 工具栏/Dock/卡片面板背景
  QString panelBgGrad;    // 面板微渐变
  QString sidebarBg;      // 侧边栏/项目树/大纲背景
  QString editorBg;       // 代码编辑器/控制台底色
  QString cardBg;         // 控件/组卡片底色
  QString baseAltBg;      // 表格/列表交替行底色
  QString headerBg;       // 视图头部/Dock标题背景

  // 边框与光影
  QString border;         // 基础边框
  QString borderLight;    // 高亮/微光边框
  QString borderDark;     // 深色/对比边框
  QString borderGlow;     // 霓虹发光边框
  QString splitterHandle; // 分割条颜色

  // 文字层级 (超清晰对比度)
  QString textMain;       // 主文字 (高对比)
  QString textSub;        // 次级文字 (清晰说明/注释)
  QString textDisabled;   // 禁用文字

  // 强调色与华丽交互 (Gradients & Accents)
  QString accent;         // 主强调色 (电光/霓虹/主色)
  QString accentGrad;     // 强调色渐变
  QString accentHover;    // 悬停渐变/色
  QString accentPressed;  // 按下态
  QString accentText;     // 强调色前景色
  QString accentGlow;     // 强调色外发光 RGBA
  QString menuHover;      // 菜单/项目悬停底色
  QString selectionBg;    // 选中背景
  QString selectionText;  // 选中文字

  // 状态指示
  QString success;        // 成功/运行中 (绿色)
  QString warning;        // 警告/待机 (橙黄色)
  QString danger;         // 危险/错误 (红色)
  QString info;           // 信息/就绪 (蓝色)

  // 状态栏
  QString statusBarBg;    // 状态栏背景
  QString statusBarText;  // 状态栏文字

  // 语法高亮色系 (C/C++)
  QColor synKeyword;      // 关键字
  QColor synString;       // 字符串
  QColor synChar;         // 字符
  QColor synNumber;       // 数字
  QColor synComment;      // 注释
  QColor synPreprocessor; // 预处理器宏
  QColor synGlobalClass;  // 类/结构体
  QColor synFunction;     // 函数名
  QColor synIdentifier;   // 变量/标识符
  QColor synOperator;     // 运算符

  // 编辑器专用
  QColor caretColor;          // 光标色
  QColor caretLineBg;        // 当前行底色
  QColor marginBg;           // 行号边距背景
  QColor marginFg;           // 行号文字
  QColor foldMarginBg;       // 折叠边距

  // 图表 (QCustomPlot) 专用调色
  QColor plotBg;             // 图表背景
  QColor plotAxis;           // 坐标轴
  QColor plotTick;           // 主刻度
  QColor plotSubTick;        // 子刻度
  QColor plotGrid;           // 主网格
  QColor plotSubGrid;        // 子网格
  QVector<QColor> chartCurves; // 曲线预设色谱 (16色)

  // 彩虹括号
  QVector<QColor> rainbowBrackets;
};

// 获取旗舰级原始调色板
inline ThemePalette getRawPaletteFor(const QString &themeName) {
  ThemePalette p;
  QString id = themeName.toLower().trimmed();

  // 1. 🌌 赛博霓虹 (Cyber Neon) - 旗舰极夜黑曜 + 霓虹青蓝 & 荧光粉
  if (id == "cyberneon" || id == "cyberpunk") {
    p.id = "cyberneon";
    p.displayName = QStringLiteral("赛博霓虹 (Cyber Neon)");
    p.icon = QStringLiteral("🌌");
    p.isDark = true;
    p.windowBg = "#070B14";
    p.windowBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0D1527, stop:1 #060911)";
    p.panelBg = "#0F1A2E";
    p.panelBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #152440, stop:1 #0E182A)";
    p.sidebarBg = "#090E1A";
    p.editorBg = "#060A12";
    p.cardBg = "#111E36";
    p.baseAltBg = "#0D172A";
    p.headerBg = "#162542";
    p.border = "#1E355B";
    p.borderLight = "#2F548F";
    p.borderDark = "#122038";
    p.borderGlow = "rgba(0, 229, 255, 0.45)";
    p.splitterHandle = "#192B48";
    p.textMain = "#FFFFFF";
    p.textSub = "#94A9C9";
    p.textDisabled = "#475D7E";
    p.accent = "#00E5FF";
    p.accentGrad = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00E5FF, stop:1 #0088FF)";
    p.accentHover = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #33ECFF, stop:1 #339FFF)";
    p.accentPressed = "#0066CC";
    p.accentText = "#050810";
    p.accentGlow = "rgba(0, 229, 255, 0.5)";
    p.menuHover = "#1A2E50";
    p.selectionBg = "#154273";
    p.selectionText = "#00E5FF";
    p.success = "#00E676";
    p.warning = "#FFB300";
    p.danger = "#FF1744";
    p.info = "#00E5FF";
    p.statusBarBg = "#090E1A";
    p.statusBarText = "#00E5FF";

    p.synKeyword = QColor("#FF007F");
    p.synString = QColor("#00E5FF");
    p.synChar = QColor("#00E5FF");
    p.synNumber = QColor("#FFD600");
    p.synComment = QColor("#6B82A6");
    p.synPreprocessor = QColor("#D500F9");
    p.synGlobalClass = QColor("#00F5D4");
    p.synFunction = QColor("#B388FF");
    p.synIdentifier = QColor("#F1F5F9");
    p.synOperator = QColor("#FF007F");

    p.caretColor = QColor("#00E5FF");
    p.caretLineBg = QColor("#0E182A");
    p.marginBg = "#090E1A";
    p.marginFg = "#6B82A6";
    p.foldMarginBg = "#090E1A";

    p.plotBg = QColor("#060A12");
    p.plotAxis = QColor("#2F548F");
    p.plotTick = QColor("#00E5FF");
    p.plotSubTick = QColor("#1E355B");
    p.plotGrid = QColor("#152440");
    p.plotSubGrid = QColor("#0C1525");
    p.chartCurves = {
      QColor("#00E5FF"), QColor("#FF007F"), QColor("#00E676"), QColor("#FFD600"),
      QColor("#D500F9"), QColor("#FF6D00"), QColor("#00B0FF"), QColor("#76FF03"),
      QColor("#F50057"), QColor("#1DE9B6"), QColor("#651FFF"), QColor("#FFEA00"),
      QColor("#00E5FF"), QColor("#FF4081"), QColor("#00E676"), QColor("#FF9100")
    };

    p.rainbowBrackets = {
      QColor("#00E5FF"), QColor("#FF007F"), QColor("#00E676"),
      QColor("#FFD600"), QColor("#D500F9"), QColor("#FF6D00")
    };
    return p;
  }

  // 2. 🌋 熔岩黑金 (Obsidian Gold) - 曜石深黑 + 熔岩赤金/琥珀尊贵光华
  if (id == "obsidiangold" || id == "moltenember") {
    p.id = "obsidiangold";
    p.displayName = QStringLiteral("熔岩黑金 (Obsidian Gold)");
    p.icon = QStringLiteral("🌋");
    p.isDark = true;
    p.windowBg = "#120F0D";
    p.windowBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1F1915, stop:1 #0D0B0A)";
    p.panelBg = "#221B16";
    p.panelBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2C231C, stop:1 #1B1511)";
    p.sidebarBg = "#15110E";
    p.editorBg = "#0E0B09";
    p.cardBg = "#271E18";
    p.baseAltBg = "#1A1410";
    p.headerBg = "#2F231B";
    p.border = "#473528";
    p.borderLight = "#694E3B";
    p.borderDark = "#2B2018";
    p.borderGlow = "rgba(245, 158, 11, 0.45)";
    p.splitterHandle = "#36281E";
    p.textMain = "#FFFBF5";
    p.textSub = "#BFA693";
    p.textDisabled = "#6B5749";
    p.accent = "#F59E0B";
    p.accentGrad = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #F59E0B, stop:1 #D97706)";
    p.accentHover = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #FBBF24, stop:1 #F59E0B)";
    p.accentPressed = "#B45309";
    p.accentText = "#120F0D";
    p.accentGlow = "rgba(245, 158, 11, 0.5)";
    p.menuHover = "#382920";
    p.selectionBg = "#54381C";
    p.selectionText = "#FBBF24";
    p.success = "#10B981";
    p.warning = "#F59E0B";
    p.danger = "#EF4444";
    p.info = "#F59E0B";
    p.statusBarBg = "#15110E";
    p.statusBarText = "#F59E0B";

    p.synKeyword = QColor("#F97316");
    p.synString = QColor("#FCD34D");
    p.synChar = QColor("#FCD34D");
    p.synNumber = QColor("#34D399");
    p.synComment = QColor("#8C7260");
    p.synPreprocessor = QColor("#FB7185");
    p.synGlobalClass = QColor("#FBBF24");
    p.synFunction = QColor("#F59E0B");
    p.synIdentifier = QColor("#FFFBF5");
    p.synOperator = QColor("#FB923C");

    p.caretColor = QColor("#F59E0B");
    p.caretLineBg = QColor("#1D1612");
    p.marginBg = "#15110E";
    p.marginFg = "#8C7260";
    p.foldMarginBg = "#15110E";

    p.plotBg = QColor("#0E0B09");
    p.plotAxis = QColor("#694E3B");
    p.plotTick = QColor("#F59E0B");
    p.plotSubTick = QColor("#473528");
    p.plotGrid = QColor("#271E18");
    p.plotSubGrid = QColor("#15110E");
    p.chartCurves = {
      QColor("#F59E0B"), QColor("#EF4444"), QColor("#10B981"), QColor("#3B82F6"),
      QColor("#EC4899"), QColor("#F97316"), QColor("#8B5CF6"), QColor("#84CC16"),
      QColor("#FCD34D"), QColor("#06B6D4"), QColor("#F43F5E"), QColor("#A855F7"),
      QColor("#F59E0B"), QColor("#EF4444"), QColor("#10B981"), QColor("#F97316")
    };

    p.rainbowBrackets = {
      QColor("#F59E0B"), QColor("#F97316"), QColor("#34D399"),
      QColor("#FBBF24"), QColor("#FB7185"), QColor("#60A5FA")
    };
    return p;
  }

  // 3. 🔮 星云紫晶 (Nebula Violet) / Dracula - 幽邃深空紫 + 霓虹紫罗兰
  if (id == "dracula" || id == "nebulaviolet" || id == "purple") {
    p.id = "dracula";
    p.displayName = QStringLiteral("星云紫晶 (Nebula Violet)");
    p.icon = QStringLiteral("🔮");
    p.isDark = true;
    p.windowBg = "#13111C";
    p.windowBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1F1B2E, stop:1 #0F0E17)";
    p.panelBg = "#201B30";
    p.panelBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2B2440, stop:1 #1A1628)";
    p.sidebarBg = "#161322";
    p.editorBg = "#0E0C16";
    p.cardBg = "#251F38";
    p.baseAltBg = "#191527";
    p.headerBg = "#2E2645";
    p.border = "#42385F";
    p.borderLight = "#61528C";
    p.borderDark = "#29233B";
    p.borderGlow = "rgba(189, 147, 249, 0.45)";
    p.splitterHandle = "#332B4A";
    p.textMain = "#FFFFFF";
    p.textSub = "#A89EC0";
    p.textDisabled = "#615777";
    p.accent = "#BD93F9";
    p.accentGrad = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #BD93F9, stop:1 #FF79C6)";
    p.accentHover = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #CAA6FF, stop:1 #FF92D0)";
    p.accentPressed = "#9B63E6";
    p.accentText = "#13111C";
    p.accentGlow = "rgba(189, 147, 249, 0.5)";
    p.menuHover = "#352B4E";
    p.selectionBg = "#493C6E";
    p.selectionText = "#FFFFFF";
    p.success = "#50FA7B";
    p.warning = "#F1FA8C";
    p.danger = "#FF5555";
    p.info = "#8BE9FD";
    p.statusBarBg = "#161322";
    p.statusBarText = "#BD93F9";

    p.synKeyword = QColor("#FF79C6");
    p.synString = QColor("#F1FA8C");
    p.synChar = QColor("#F1FA8C");
    p.synNumber = QColor("#BD93F9");
    p.synComment = QColor("#7584B3");
    p.synPreprocessor = QColor("#FF79C6");
    p.synGlobalClass = QColor("#8BE9FD");
    p.synFunction = QColor("#50FA7B");
    p.synIdentifier = QColor("#FFFFFF");
    p.synOperator = QColor("#FF79C6");

    p.caretColor = QColor("#F8F8F0");
    p.caretLineBg = QColor("#1D182B");
    p.marginBg = "#161322";
    p.marginFg = "#7584B3";
    p.foldMarginBg = "#161322";

    p.plotBg = QColor("#0E0C16");
    p.plotAxis = QColor("#61528C");
    p.plotTick = QColor("#BD93F9");
    p.plotSubTick = QColor("#42385F");
    p.plotGrid = QColor("#251F38");
    p.plotSubGrid = QColor("#161322");
    p.chartCurves = {
      QColor("#BD93F9"), QColor("#FF79C6"), QColor("#50FA7B"), QColor("#8BE9FD"),
      QColor("#F1FA8C"), QColor("#FFB86C"), QColor("#FF5555"), QColor("#E1BEE7"),
      QColor("#BD93F9"), QColor("#FF79C6"), QColor("#50FA7B"), QColor("#8BE9FD"),
      QColor("#F1FA8C"), QColor("#FFB86C"), QColor("#FF5555"), QColor("#E1BEE7")
    };

    p.rainbowBrackets = {
      QColor("#FF79C6"), QColor("#8BE9FD"), QColor("#50FA7B"),
      QColor("#F1FA8C"), QColor("#BD93F9"), QColor("#FFB86C")
    };
    return p;
  }

  // 4. 🌊 碧海深渊 (Abyssal Ocean) / Nord - 深渊墨蓝 + 冰晶碧蓝
  if (id == "nord" || id == "nightowl" || id == "abyssalocean" || id == "ocean") {
    p.id = "nord";
    p.displayName = QStringLiteral("碧海深渊 (Abyssal Ocean)");
    p.icon = QStringLiteral("🌊");
    p.isDark = true;
    p.windowBg = "#08101E";
    p.windowBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0F1D33, stop:1 #060B14)";
    p.panelBg = "#12233C";
    p.panelBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1A2F4F, stop:1 #0F1E34)";
    p.sidebarBg = "#0A1424";
    p.editorBg = "#050B14";
    p.cardBg = "#152845";
    p.baseAltBg = "#0E1A2D";
    p.headerBg = "#1C3357";
    p.border = "#23406D";
    p.borderLight = "#355F9E";
    p.borderDark = "#152742";
    p.borderGlow = "rgba(0, 180, 216, 0.45)";
    p.splitterHandle = "#1A3052";
    p.textMain = "#FFFFFF";
    p.textSub = "#8CB4E0";
    p.textDisabled = "#456488";
    p.accent = "#00B4D8";
    p.accentGrad = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00B4D8, stop:1 #0077B6)";
    p.accentHover = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #48CAE4, stop:1 #0096C7)";
    p.accentPressed = "#023E8A";
    p.accentText = "#08101E";
    p.accentGlow = "rgba(0, 180, 216, 0.5)";
    p.menuHover = "#1C375C";
    p.selectionBg = "#1E477A";
    p.selectionText = "#FFFFFF";
    p.success = "#06D6A0";
    p.warning = "#FFD166";
    p.danger = "#EF476F";
    p.info = "#00B4D8";
    p.statusBarBg = "#0A1424";
    p.statusBarText = "#00B4D8";

    p.synKeyword = QColor("#81A1C1");
    p.synString = QColor("#A3BE8C");
    p.synChar = QColor("#A3BE8C");
    p.synNumber = QColor("#B48EAD");
    p.synComment = QColor("#6D7F9E");
    p.synPreprocessor = QColor("#5E81AC");
    p.synGlobalClass = QColor("#8FBCBB");
    p.synFunction = QColor("#88C0D0");
    p.synIdentifier = QColor("#FFFFFF");
    p.synOperator = QColor("#81A1C1");

    p.caretColor = QColor("#D8DEE9");
    p.caretLineBg = QColor("#102038");
    p.marginBg = "#0A1424";
    p.marginFg = "#6D7F9E";
    p.foldMarginBg = "#0A1424";

    p.plotBg = QColor("#050B14");
    p.plotAxis = QColor("#355F9E");
    p.plotTick = QColor("#00B4D8");
    p.plotSubTick = QColor("#23406D");
    p.plotGrid = QColor("#152845");
    p.plotSubGrid = QColor("#0A1424");
    p.chartCurves = {
      QColor("#00B4D8"), QColor("#06D6A0"), QColor("#FFD166"), QColor("#EF476F"),
      QColor("#118AB2"), QColor("#7209B7"), QColor("#48CAE4"), QColor("#F72585"),
      QColor("#00B4D8"), QColor("#06D6A0"), QColor("#FFD166"), QColor("#EF476F"),
      QColor("#118AB2"), QColor("#7209B7"), QColor("#48CAE4"), QColor("#F72585")
    };

    p.rainbowBrackets = {
      QColor("#88C0D0"), QColor("#81A1C1"), QColor("#B48EAD"),
      QColor("#A3BE8C"), QColor("#EBCB8B"), QColor("#D08770")
    };
    return p;
  }

  // 5. 🌲 极客翡翠 (Emerald Matrix) / Vue - 矩阵深墨绿 + 极光翡翠科技风
  if (id == "vue" || id == "emeraldmatrix" || id == "matrix") {
    p.id = "vue";
    p.displayName = QStringLiteral("极客翡翠 (Emerald Matrix)");
    p.icon = QStringLiteral("🌲");
    p.isDark = true;
    p.windowBg = "#0A140F";
    p.windowBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #12241C, stop:1 #060D0A)";
    p.panelBg = "#152B20";
    p.panelBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E3B2C, stop:1 #11231A)";
    p.sidebarBg = "#0D1A14";
    p.editorBg = "#060D0A";
    p.cardBg = "#1A3629";
    p.baseAltBg = "#0F2018";
    p.headerBg = "#204231";
    p.border = "#2B5741";
    p.borderLight = "#3E7D5E";
    p.borderDark = "#193527";
    p.borderGlow = "rgba(16, 185, 129, 0.45)";
    p.splitterHandle = "#204231";
    p.textMain = "#FFFFFF";
    p.textSub = "#7CE8BE";
    p.textDisabled = "#40775F";
    p.accent = "#10B981";
    p.accentGrad = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #10B981, stop:1 #059669)";
    p.accentHover = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #34D399, stop:1 #10B981)";
    p.accentPressed = "#047857";
    p.accentText = "#0A140F";
    p.accentGlow = "rgba(16, 185, 129, 0.5)";
    p.menuHover = "#224A37";
    p.selectionBg = "#175E46";
    p.selectionText = "#FFFFFF";
    p.success = "#10B981";
    p.warning = "#FBBF24";
    p.danger = "#F87171";
    p.info = "#38BDF8";
    p.statusBarBg = "#0D1A14";
    p.statusBarText = "#10B981";

    p.synKeyword = QColor("#10B981");
    p.synString = QColor("#34D399");
    p.synChar = QColor("#34D399");
    p.synNumber = QColor("#FBBF24");
    p.synComment = QColor("#5B8C75");
    p.synPreprocessor = QColor("#2DD4BF");
    p.synGlobalClass = QColor("#6EE7B7");
    p.synFunction = QColor("#38BDF8");
    p.synIdentifier = QColor("#FFFFFF");
    p.synOperator = QColor("#10B981");

    p.caretColor = QColor("#10B981");
    p.caretLineBg = QColor("#11261C");
    p.marginBg = "#0D1A14";
    p.marginFg = "#5B8C75";
    p.foldMarginBg = "#0D1A14";

    p.plotBg = QColor("#060D0A");
    p.plotAxis = QColor("#3E7D5E");
    p.plotTick = QColor("#10B981");
    p.plotSubTick = QColor("#2B5741");
    p.plotGrid = QColor("#1A3629");
    p.plotSubGrid = QColor("#0D1A14");
    p.chartCurves = {
      QColor("#10B981"), QColor("#38BDF8"), QColor("#FBBF24"), QColor("#F472B6"),
      QColor("#A78BFA"), QColor("#34D399"), QColor("#FB923C"), QColor("#4ADE80"),
      QColor("#10B981"), QColor("#38BDF8"), QColor("#FBBF24"), QColor("#F472B6"),
      QColor("#A78BFA"), QColor("#34D399"), QColor("#FB923C"), QColor("#4ADE80")
    };

    p.rainbowBrackets = {
      QColor("#10B981"), QColor("#38BDF8"), QColor("#FBBF24"),
      QColor("#34D399"), QColor("#F472B6"), QColor("#A78BFA")
    };
    return p;
  }

  // 6. ☀️ 纯白曜石 (Crystal Light) / Light - 全屏纯净瓷白 + 宝石湛蓝高对比 (零深色残留)
  if (id == "light" || id == "githublight" || id == "materiallight" || id == "crystallight") {
    p.id = "light";
    p.displayName = QStringLiteral("纯白曜石 (Crystal Light)");
    p.icon = QStringLiteral("☀️");
    p.isDark = false;
    p.windowBg = "#F8FAFC";
    p.windowBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFFFFF, stop:1 #F1F5F9)";
    p.panelBg = "#FFFFFF";
    p.panelBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFFFFF, stop:1 #F8FAFC)";
    p.sidebarBg = "#F1F5F9";
    p.editorBg = "#FFFFFF";
    p.cardBg = "#FFFFFF";
    p.baseAltBg = "#F8FAFC";
    p.headerBg = "#E2E8F0";
    p.border = "#CBD5E1";
    p.borderLight = "#E2E8F0";
    p.borderDark = "#94A3B8";
    p.borderGlow = "rgba(37, 99, 235, 0.25)";
    p.splitterHandle = "#E2E8F0";
    p.textMain = "#0F172A";
    p.textSub = "#475569";
    p.textDisabled = "#94A3B8";
    p.accent = "#2563EB";
    p.accentGrad = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3B82F6, stop:1 #2563EB)";
    p.accentHover = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #60A5FA, stop:1 #3B82F6)";
    p.accentPressed = "#1D4ED8";
    p.accentText = "#FFFFFF";
    p.accentGlow = "rgba(37, 99, 235, 0.3)";
    p.menuHover = "#E0E7FF";
    p.selectionBg = "#BFDBFE";
    p.selectionText = "#1E3A8A";
    p.success = "#16A34A";
    p.warning = "#D97706";
    p.danger = "#DC2626";
    p.info = "#2563EB";
    p.statusBarBg = "#F1F5F9";
    p.statusBarText = "#1E293B";

    p.synKeyword = QColor("#2563EB");
    p.synString = QColor("#DC2626");
    p.synChar = QColor("#DC2626");
    p.synNumber = QColor("#0D9488");
    p.synComment = QColor("#64748B");
    p.synPreprocessor = QColor("#7C3AED");
    p.synGlobalClass = QColor("#0284C7");
    p.synFunction = QColor("#D97706");
    p.synIdentifier = QColor("#0F172A");
    p.synOperator = QColor("#0F172A");

    p.caretColor = QColor("#0F172A");
    p.caretLineBg = QColor("#F1F5F9");
    p.marginBg = "#F8FAFC";
    p.marginFg = "#64748B";
    p.foldMarginBg = "#F8FAFC";

    p.plotBg = QColor("#FFFFFF");
    p.plotAxis = QColor("#94A3B8");
    p.plotTick = QColor("#2563EB");
    p.plotSubTick = QColor("#CBD5E1");
    p.plotGrid = QColor("#E2E8F0");
    p.plotSubGrid = QColor("#F8FAFC");
    p.chartCurves = {
      QColor("#2563EB"), QColor("#DC2626"), QColor("#16A34A"), QColor("#D97706"),
      QColor("#7C3AED"), QColor("#0891B2"), QColor("#DB2777"), QColor("#EA580C"),
      QColor("#2563EB"), QColor("#DC2626"), QColor("#16A34A"), QColor("#D97706"),
      QColor("#7C3AED"), QColor("#0891B2"), QColor("#DB2777"), QColor("#EA580C")
    };

    p.rainbowBrackets = {
      QColor("#2563EB"), QColor("#DC2626"), QColor("#16A34A"),
      QColor("#D97706"), QColor("#7C3AED"), QColor("#0891B2")
    };
    return p;
  }

  // 7. 📜 暖阳羊皮 (Solarized Light) - 经典暖黄羊皮纸护眼
  if (id == "solarizedlight") {
    p.id = "solarizedlight";
    p.displayName = QStringLiteral("暖阳羊皮 (Solarized Light)");
    p.icon = QStringLiteral("📜");
    p.isDark = false;
    p.windowBg = "#FDF6E3";
    p.windowBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FDF6E3, stop:1 #EEE8D5)";
    p.panelBg = "#FDF6E3";
    p.panelBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FDF6E3, stop:1 #F5EEDB)";
    p.sidebarBg = "#EEE8D5";
    p.editorBg = "#FDF6E3";
    p.cardBg = "#FDF6E3";
    p.baseAltBg = "#E9E2CE";
    p.headerBg = "#E4DCBF";
    p.border = "#D5CDAE";
    p.borderLight = "#E6DEC7";
    p.borderDark = "#BDB490";
    p.borderGlow = "rgba(181, 137, 0, 0.3)";
    p.splitterHandle = "#D5CDAE";
    p.textMain = "#4B5B61";
    p.textSub = "#657B83";
    p.textDisabled = "#93A1A1";
    p.accent = "#268BD2";
    p.accentGrad = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #268BD2, stop:1 #2AA198)";
    p.accentHover = "#42A5F5";
    p.accentPressed = "#1A74B3";
    p.accentText = "#FDF6E3";
    p.accentGlow = "rgba(38, 139, 210, 0.3)";
    p.menuHover = "#E8DFCA";
    p.selectionBg = "#EEE8D5";
    p.selectionText = "#268BD2";
    p.success = "#859900";
    p.warning = "#B58900";
    p.danger = "#DC322F";
    p.info = "#268BD2";
    p.statusBarBg = "#EEE8D5";
    p.statusBarText = "#4B5B61";

    p.synKeyword = QColor("#859900");
    p.synString = QColor("#2AA198");
    p.synChar = QColor("#2AA198");
    p.synNumber = QColor("#D33682");
    p.synComment = QColor("#93A1A1");
    p.synPreprocessor = QColor("#CB4B16");
    p.synGlobalClass = QColor("#B58900");
    p.synFunction = QColor("#268BD2");
    p.synIdentifier = QColor("#4B5B61");
    p.synOperator = QColor("#859900");

    p.caretColor = QColor("#657B83");
    p.caretLineBg = QColor("#EEE8D5");
    p.marginBg = "#EEE8D5";
    p.marginFg = "#93A1A1";
    p.foldMarginBg = "#EEE8D5";

    p.plotBg = QColor("#FDF6E3");
    p.plotAxis = QColor("#93A1A1");
    p.plotTick = QColor("#268BD2");
    p.plotSubTick = QColor("#D5CDAE");
    p.plotGrid = QColor("#EEE8D5");
    p.plotSubGrid = QColor("#E9E2CE");
    p.chartCurves = {
      QColor("#268BD2"), QColor("#DC322F"), QColor("#859900"), QColor("#B58900"),
      QColor("#6C71C4"), QColor("#2AA198"), QColor("#D33682"), QColor("#CB4B16"),
      QColor("#268BD2"), QColor("#DC322F"), QColor("#859900"), QColor("#B58900"),
      QColor("#6C71C4"), QColor("#2AA198"), QColor("#D33682"), QColor("#CB4B16")
    };

    p.rainbowBrackets = {
      QColor("#B58900"), QColor("#CB4B16"), QColor("#D33682"),
      QColor("#6C71C4"), QColor("#268BD2"), QColor("#2AA198")
    };
    return p;
  }

  // 默认：⚡ 极客钛金 (Titanium Dark)
  p.id = "dark";
  p.displayName = QStringLiteral("极客钛金 (Titanium Dark)");
  p.icon = QStringLiteral("⚡");
  p.isDark = true;
  p.windowBg = "#14171E";
  p.windowBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1D212A, stop:1 #111319)";
  p.panelBg = "#1E232D";
  p.panelBgGrad = "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #272D3A, stop:1 #1B1F27)";
  p.sidebarBg = "#161921";
  p.editorBg = "#101217";
  p.cardBg = "#232834";
  p.baseAltBg = "#181C24";
  p.headerBg = "#2B3240";
  p.border = "#363E50";
  p.borderLight = "#4C5770";
  p.borderDark = "#232834";
  p.borderGlow = "rgba(99, 102, 241, 0.45)";
  p.splitterHandle = "#282E3C";
  p.textMain = "#FFFFFF";
  p.textSub = "#A4B3C6";
  p.textDisabled = "#536077";
  p.accent = "#6366F1";
  p.accentGrad = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #6366F1, stop:1 #4F46E5)";
  p.accentHover = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #818CF8, stop:1 #6366F1)";
  p.accentPressed = "#4338CA";
  p.accentText = "#FFFFFF";
  p.accentGlow = "rgba(99, 102, 241, 0.5)";
  p.menuHover = "#2E3647";
  p.selectionBg = "#374261";
  p.selectionText = "#FFFFFF";
  p.success = "#10B981";
  p.warning = "#F59E0B";
  p.danger = "#EF4444";
  p.info = "#38BDF8";
  p.statusBarBg = "#161921";
  p.statusBarText = "#A5B4FC";

  p.synKeyword = QColor("#818CF8");
  p.synString = QColor("#34D399");
  p.synChar = QColor("#34D399");
  p.synNumber = QColor("#FBBF24");
  p.synComment = QColor("#6B7A90");
  p.synPreprocessor = QColor("#C084FC");
  p.synGlobalClass = QColor("#38BDF8");
  p.synFunction = QColor("#60A5FA");
  p.synIdentifier = QColor("#FFFFFF");
  p.synOperator = QColor("#CBD5E1");

  p.caretColor = QColor("#818CF8");
  p.caretLineBg = QColor("#1C202B");
  p.marginBg = "#161921";
  p.marginFg = "#6B7A90";
  p.foldMarginBg = "#161921";

  p.plotBg = QColor("#101217");
  p.plotAxis = QColor("#4C5770");
  p.plotTick = QColor("#818CF8");
  p.plotSubTick = QColor("#363E50");
  p.plotGrid = QColor("#232834");
  p.plotSubGrid = QColor("#161921");
  p.chartCurves = {
    QColor("#818CF8"), QColor("#10B981"), QColor("#F59E0B"), QColor("#EF4444"),
    QColor("#38BDF8"), QColor("#EC4899"), QColor("#A855F7"), QColor("#14B8A6"),
    QColor("#818CF8"), QColor("#10B981"), QColor("#F59E0B"), QColor("#EF4444"),
    QColor("#38BDF8"), QColor("#EC4899"), QColor("#A855F7"), QColor("#14B8A6")
  };

  p.rainbowBrackets = {
    QColor("#818CF8"), QColor("#34D399"), QColor("#FBBF24"),
    QColor("#38BDF8"), QColor("#F472B6"), QColor("#A78BFA")
  };

  return p;
}

// 获取旗舰级调色板 (自动填充 fallback 与兼容字段)
inline ThemePalette paletteFor(const QString &themeName) {
  ThemePalette p = getRawPaletteFor(themeName);
  if (p.borderDark.isEmpty()) {
    p.borderDark = p.border;
  }
  if (p.menuHover.isEmpty()) {
    p.menuHover = p.panelBg;
  }
  return p;
}

// 获取所有旗舰级主题的列表
inline QStringList themeList() {
  return {
    "dark",           // ⚡ 极客钛金
    "cyberneon",      // 🌌 赛博霓虹
    "obsidiangold",   // 🌋 熔岩黑金
    "dracula",        // 🔮 星云紫晶
    "nord",           // 🌊 碧海深渊
    "vue",            // 🌲 极客翡翠
    "light",          // ☀️ 纯白曜石
    "solarizedlight"  // 📜 暖阳羊皮
  };
}

// 生成覆盖全 IDE、串口调试助手、CAN 调试助手的殿堂级 QSS
inline QString generateStyleSheet(const QString &themeName) {
  const ThemePalette p = paletteFor(themeName);

  QString qss = QStringLiteral(R"QSS(
/* 全局基础设定与高端字体渲染 */
QMainWindow, QDialog, QWidget#centralWidget, QWidget#canRoot, QWidget#serialRoot, QWidget#serialContainer, QWidget#appHubRoot, QWidget#hubCentral, QWidget#networkToolRoot, QWidget#iapToolRoot, QWidget#oscilloscopeRoot, QWidget#tabCornerWidget {
  background-color: {{WINDOW_BG}};
  color: {{TEXT_MAIN}};
  font-family: 'Segoe UI', 'Microsoft YaHei', 'PingFang SC', sans-serif;
  font-size: 12px;
}

/* 主工作台 AppHub 现代融合视觉 */
QWidget#appHubHeader {
  background: {{HEADER_BG}};
  border-bottom: 1px solid {{BORDER}};
}
QWidget#appHubSidebar {
  background-color: {{SIDEBAR_BG}};
  border-right: 1px solid {{BORDER}};
}
QScrollArea#appHubScrollArea {
  background-color: {{WINDOW_BG}};
  border: none;
}
QWidget#appHubFooter {
  background-color: {{STATUSBAR_BG}};
  border-top: 1px solid {{BORDER}};
}
QLineEdit#appHubSearchEdit {
  background-color: {{CARD_BG}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER}};
  border-radius: 17px;
  padding: 0 16px;
  font-size: 13px;
}
QLineEdit#appHubSearchEdit:focus {
  border: 1px solid {{ACCENT}};
  background-color: {{CARD_BG}};
}

/* 视图头部与标题栏 (CAN/串口/示波器一体化卡片头) */
QWidget#viewHeader {
  background: {{HEADER_BG}};
  border-bottom: 1px solid {{BORDER}};
  border-top-left-radius: 6px;
  border-top-right-radius: 6px;
}
QLabel#viewTitle {
  color: {{TEXT_MAIN}};
  font-weight: bold;
  font-size: 13px;
}
QPushButton#viewCloseBtn {
  background-color: transparent;
  color: {{TEXT_SUB}};
  border: none;
  font-weight: bold;
  font-size: 13px;
  border-radius: 4px;
}
QPushButton#viewCloseBtn:hover {
  background-color: #EF4444;
  color: #FFFFFF;
}

/* 垂直侧边栏 (VerticalTabWidget / ChromeTab) */
QWidget#vtSidebar {
  background-color: {{SIDEBAR_BG}};
  border-right: 1px solid {{BORDER}};
}
QWidget#vtTabContainer {
  background: transparent;
}
QStackedWidget#vtStacked {
  background: transparent;
  border: none;
}
QPushButton#vtNewTabBtn {
  background: transparent;
  color: {{TEXT_SUB}};
  font-size: 20px;
  font-weight: 300;
  border: none;
}
QPushButton#vtNewTabBtn:hover {
  background-color: {{PANEL_BG}};
  color: {{ACCENT}};
  border-radius: 4px;
}

/* 顶部菜单栏 */
QMenuBar {
  background: {{HEADER_BG}};
  color: {{TEXT_MAIN}};
  border-bottom: 1px solid {{BORDER}};
  padding: 2px 4px;
}
QMenuBar::item {
  background: transparent;
  padding: 5px 10px;
  border-radius: 4px;
  margin: 1px;
}
QMenuBar::item:selected {
  background: {{PANEL_BG}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER}};
}

/* 下拉菜单 */
QMenu {
  background-color: {{CARD_BG}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER_LIGHT}};
  border-radius: 6px;
  padding: 5px;
}
QMenu::item {
  padding: 6px 26px 6px 20px;
  border-radius: 4px;
  color: {{TEXT_MAIN}};
}
QMenu::item:selected {
  background: {{ACCENT_GRAD}};
  color: {{ACCENT_TEXT}};
}
QMenu::separator {
  height: 1px;
  background-color: {{BORDER}};
  margin: 4px 6px;
}

/* 顶级工具栏 ToolBar & canToolbar */
QToolBar, QWidget#canToolbar {
  background: {{PANEL_BG_GRAD}};
  border-bottom: 1px solid {{BORDER}};
  spacing: 5px;
  padding: 4px 8px;
}
QToolBar::separator {
  background-color: {{BORDER}};
  width: 1px;
  margin: 4px 6px;
}
QToolButton, QToolButton#canNavButton {
  background: transparent;
  border: 1px solid transparent;
  border-radius: 6px;
  padding: 4px 6px;
  color: {{TEXT_MAIN}};
  font-weight: 500;
}
QToolButton:hover, QToolButton#canNavButton:hover {
  background-color: {{PANEL_BG}};
  border: 1px solid {{BORDER_LIGHT}};
  color: {{TEXT_MAIN}};
}
QToolButton:pressed, QToolButton:checked, QToolButton#canNavButton:pressed {
  background: {{ACCENT_GRAD}};
  color: {{ACCENT_TEXT}};
  border: 1px solid {{ACCENT}};
}

/* 右上角主题切换快捷图标按钮 (0xe622) */
QToolButton#btnThemeIcon {
  background: transparent;
  color: {{ACCENT}};
  border: 1px solid {{BORDER}};
  border-radius: 6px;
  padding: 3px 8px;
  font-weight: bold;
}
QToolButton#btnThemeIcon:hover {
  background: {{ACCENT_GRAD}};
  color: {{ACCENT_TEXT}};
  border: 1px solid {{ACCENT}};
}

/* 串口控制中心专属高阶样式 */
QPushButton#btnSerialOpenClose {
  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #10B981, stop:1 #059669);
  color: #FFFFFF;
  font-size: 13px;
  font-weight: bold;
  border: 1px solid #10B981;
  border-radius: 8px;
  padding: 6px 16px;
  min-height: 24px;
}
QPushButton#btnSerialOpenClose:hover {
  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #34D399, stop:1 #10B981);
  border-color: #34D399;
}
QPushButton#btnSerialOpenClose:checked {
  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #EF4444, stop:1 #DC2626);
  border: 1px solid #EF4444;
  color: #FFFFFF;
}
QPushButton#btnSerialOpenClose:checked:hover {
  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #F87171, stop:1 #EF4444);
  border-color: #F87171;
}
QPushButton#btnSerialRefresh {
  background: {{PANEL_BG_GRAD}};
  color: {{TEXT_MAIN}};
  font-size: 12px;
  font-weight: bold;
  border: 1px solid {{BORDER}};
  border-radius: 8px;
  padding: 6px 12px;
  min-height: 24px;
}
QPushButton#btnSerialRefresh:hover {
  background: {{ACCENT_GRAD}};
  color: {{ACCENT_TEXT}};
  border-color: {{ACCENT}};
}
QLabel#lblSerialStatusBadge {
  background-color: {{CARD_BG}};
  color: {{TEXT_SUB}};
  border: 1px solid {{BORDER}};
  border-radius: 14px;
  padding: 4px 14px;
  font-size: 12px;
  font-weight: 600;
}

/* 分割器 Splitter (消除所有白块与死角) */
QSplitter {
  background-color: {{WINDOW_BG}};
}
QSplitter::handle {
  background-color: {{SPLITTER_HANDLE}};
}
QSplitter::handle:horizontal {
  width: 4px;
}
QSplitter::handle:vertical {
  height: 4px;
}
QMainWindow::separator {
  background-color: {{SPLITTER_HANDLE}};
  width: 4px;
  height: 4px;
}
QMainWindow::separator:hover {
  background: {{ACCENT}};
}

/* 停靠窗口 DockWidget & dataDock 消除白边一体化 */
QDockWidget, QDockWidget#dataDock {
  background-color: {{WINDOW_BG}};
  color: {{TEXT_MAIN}};
  font-weight: bold;
  font-size: 12px;
  border: 1px solid {{BORDER}};
  border-radius: 8px;
}
QDockWidget::title, QDockWidget#dataDock::title {
  background: {{PANEL_BG_GRAD}};
  border: none;
  border-bottom: 1px solid {{BORDER}};
  border-top-left-radius: 8px;
  border-top-right-radius: 8px;
  padding: 6px 10px;
  text-align: left;
  color: {{TEXT_MAIN}};
}
QDockWidget QWidget#dockContentWidget, QDockWidget#dataDock QWidget#dockContentWidget {
  background-color: {{WINDOW_BG}};
  border: none;
  border-bottom-left-radius: 8px;
  border-bottom-right-radius: 8px;
}

/* 选项卡 TabWidget 胶囊化与发光下划线 (消除顶部右侧白色死角) */
QTabWidget {
  background-color: transparent;
}
QTabWidget::pane {
  border: 1px solid {{BORDER}};
  border-radius: 0 0 6px 6px;
  background: {{WINDOW_BG}};
  top: -1px;
}
QTabWidget::left-corner, QTabWidget::right-corner {
  background: transparent;
  border: none;
}
QTabBar {
  background: transparent;
}
QTabBar::tab {
  background: {{SIDEBAR_BG}};
  color: {{TEXT_SUB}};
  padding: 7px 16px;
  border: 1px solid {{BORDER}};
  border-bottom: none;
  border-top-left-radius: 6px;
  border-top-right-radius: 6px;
  font-size: 12px;
  margin-right: 3px;
  font-weight: 500;
}
QTabBar::tab:selected {
  background: {{PANEL_BG_GRAD}};
  color: {{TEXT_MAIN}};
  font-weight: bold;
  border-top: 2px solid {{ACCENT}};
  border-bottom: 2px solid transparent;
}
QTabBar::tab:hover:!selected {
  background: {{PANEL_BG}};
  color: {{TEXT_MAIN}};
}
QTabBar QToolButton {
  background-color: {{PANEL_BG_GRAD}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER}};
  border-radius: 4px;
}
QTabBar QToolButton:hover {
  background: {{ACCENT_GRAD}};
  color: {{ACCENT_TEXT}};
}

/* 报文列表与项目树 QTreeView / QTableWidget / QTreeWidget / QTableView */
QTreeView, QTableWidget, QTableView, QTreeWidget, QListWidget {
  background-color: {{CARD_BG}};
  alternate-background-color: {{BASE_ALT_BG}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER}};
  border-radius: 6px;
  gridline-color: {{BORDER}};
  outline: 0;
}
QTreeView::item, QTableWidget::item, QTreeWidget::item, QListWidget::item {
  padding: 4px 6px;
  border-radius: 3px;
  color: {{TEXT_MAIN}};
}
QTreeView::item:hover, QTableWidget::item:hover, QTreeWidget::item:hover, QListWidget::item:hover {
  background-color: {{PANEL_BG}};
}
QTreeView::item:selected, QTableWidget::item:selected, QTreeWidget::item:selected, QListWidget::item:selected {
  background-color: {{SELECTION_BG}};
  color: {{SELECTION_TEXT}};
  font-weight: 500;
}
QHeaderView {
  background: {{PANEL_BG_GRAD}};
  border: none;
}
QHeaderView::section {
  background: {{PANEL_BG_GRAD}};
  color: {{TEXT_SUB}};
  font-weight: bold;
  padding: 6px 8px;
  border: 1px solid {{BORDER}};
  font-size: 12px;
}
QTableCornerButton::section {
  background: {{PANEL_BG_GRAD}};
  border: 1px solid {{BORDER}};
}
QAbstractScrollArea::corner {
  background: {{PANEL_BG_GRAD}};
  border: none;
}

/* 滚动条 ScrollBar */
QScrollBar:vertical {
  background-color: transparent;
  width: 9px;
  margin: 0px;
}
QScrollBar::handle:vertical {
  background-color: {{BORDER}};
  border-radius: 4px;
  min-height: 24px;
  margin: 1px;
}
QScrollBar::handle:vertical:hover {
  background-color: {{ACCENT}};
}
QScrollBar:horizontal {
  background-color: transparent;
  height: 9px;
  margin: 0px;
}
QScrollBar::handle:horizontal {
  background-color: {{BORDER}};
  border-radius: 4px;
  min-width: 24px;
  margin: 1px;
}
QScrollBar::handle:horizontal:hover {
  background-color: {{ACCENT}};
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

/* 文本编辑器与控制台输入 (编译输出/调试控制台/接收区/发送区全覆盖，彻底消除白色背景) */
QTextEdit, QPlainTextEdit {
  background-color: {{EDITOR_BG}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER}};
  border-radius: 6px;
  font-family: 'Consolas', 'Cascadia Code', 'Courier New', monospace;
  font-size: 12px;
  padding: 4px;
}
QTextEdit:focus, QPlainTextEdit:focus {
  border: 1px solid {{ACCENT}};
}

/* 输入框与下拉选框 QLineEdit / QComboBox / QSpinBox */
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
  background-color: {{CARD_BG}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER}};
  border-radius: 5px;
  padding: 4px 10px;
  font-size: 12px;
  min-height: 22px;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
  border: 1px solid {{ACCENT}};
}
QComboBox::drop-down {
  border: none;
  width: 20px;
}
QComboBox QAbstractItemView {
  background-color: {{CARD_BG}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER_LIGHT}};
  selection-background-color: {{ACCENT_GRAD}};
  selection-color: {{ACCENT_TEXT}};
  outline: 0;
  padding: 4px;
  border-radius: 4px;
}

/* 华丽高阶按钮 PushButton */
QPushButton {
  background: {{PANEL_BG_GRAD}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER}};
  border-radius: 6px;
  padding: 6px 14px;
  font-weight: bold;
  font-size: 12px;
}
QPushButton:hover {
  background: {{ACCENT_GRAD}};
  color: {{ACCENT_TEXT}};
  border: 1px solid {{ACCENT}};
}
QPushButton:pressed {
  background: {{ACCENT_PRESSED}};
  color: {{ACCENT_TEXT}};
}
QPushButton:disabled {
  background-color: {{WINDOW_BG}};
  color: {{TEXT_DISABLED}};
  border-color: {{BORDER}};
}

/* 卡片容器与参数分组框 GroupBox / Card */
QGroupBox, QWidget#cardWidget, QWidget#scopeQuickPanel {
  font-weight: bold;
  color: {{TEXT_SUB}};
  border: 1px solid {{BORDER}};
  border-radius: 8px;
  margin-top: 12px;
  padding: 10px;
  font-size: 12px;
  background-color: {{CARD_BG}};
}
QGroupBox::title {
  subcontrol-origin: margin;
  subcontrol-position: top left;
  left: 12px;
  padding: 0 6px;
  color: {{ACCENT}};
}
QLabel#cardTitle {
  font-weight: bold;
  font-size: 13px;
  color: {{ACCENT}};
  padding-bottom: 4px;
}

/* 进度条 ProgressBar */
QProgressBar {
  background-color: {{CARD_BG}};
  color: {{TEXT_MAIN}};
  border: 1px solid {{BORDER}};
  border-radius: 6px;
  text-align: center;
  font-weight: bold;
  font-size: 11px;
}
QProgressBar::chunk {
  background: {{ACCENT_GRAD}};
  border-radius: 5px;
}

/* 标签 Label */
QLabel {
  color: {{TEXT_MAIN}};
  background: transparent;
}

/* 勾选框 CheckBox 与单选框 RadioButton */
QCheckBox, QRadioButton {
  color: {{TEXT_MAIN}};
  spacing: 6px;
  font-size: 12px;
  background: transparent;
}
QCheckBox::indicator, QRadioButton::indicator {
  width: 16px;
  height: 16px;
  border: 1px solid {{BORDER}};
  border-radius: 3px;
  background-color: {{CARD_BG}};
}
QRadioButton::indicator {
  border-radius: 8px;
}
QCheckBox::indicator:checked, QRadioButton::indicator:checked {
  background-color: {{ACCENT}};
  border-color: {{ACCENT}};
}

/* 底部状态栏 StatusBar & canStatusBar & sessionStatusBar */
QStatusBar, QWidget#canStatusBar, QWidget#sessionStatusBar {
  background-color: {{STATUSBAR_BG}};
  color: {{STATUSBAR_TEXT}};
  border-top: 1px solid {{BORDER}};
  font-size: 12px;
  font-weight: 500;
}
QStatusBar QLabel, QWidget#canStatusBar QLabel, QWidget#sessionStatusBar QLabel {
  color: {{STATUSBAR_TEXT}};
}
QLabel#canStatusText, QLabel#sessionStatusWelcome {
  font-weight: bold;
  color: {{ACCENT}};
}
QLabel#sessionStatusStat {
  font-weight: bold;
  color: {{TEXT_SUB}};
}
)QSS");

  qss.replace("{{WINDOW_BG}}", p.windowBg)
     .replace("{{WINDOW_BG_GRAD}}", p.windowBgGrad)
     .replace("{{PANEL_BG}}", p.panelBg)
     .replace("{{PANEL_BG_GRAD}}", p.panelBgGrad)
     .replace("{{SIDEBAR_BG}}", p.sidebarBg)
     .replace("{{EDITOR_BG}}", p.editorBg)
     .replace("{{CARD_BG}}", p.cardBg)
     .replace("{{BASE_ALT_BG}}", p.baseAltBg)
     .replace("{{HEADER_BG}}", p.headerBg)
     .replace("{{BORDER}}", p.border)
     .replace("{{BORDER_LIGHT}}", p.borderLight)
     .replace("{{BORDER_DARK}}", p.borderDark)
     .replace("{{SPLITTER_HANDLE}}", p.splitterHandle)
     .replace("{{TEXT_MAIN}}", p.textMain)
     .replace("{{TEXT_SUB}}", p.textSub)
     .replace("{{TEXT_DISABLED}}", p.textDisabled)
     .replace("{{ACCENT}}", p.accent)
     .replace("{{ACCENT_GRAD}}", p.accentGrad)
     .replace("{{ACCENT_HOVER}}", p.accentHover)
     .replace("{{ACCENT_PRESSED}}", p.accentPressed)
     .replace("{{ACCENT_TEXT}}", p.accentText)
     .replace("{{MENU_HOVER}}", p.menuHover)
     .replace("{{SELECTION_BG}}", p.selectionBg)
     .replace("{{SELECTION_TEXT}}", p.selectionText)
     .replace("{{STATUSBAR_BG}}", p.statusBarBg)
     .replace("{{STATUSBAR_TEXT}}", p.statusBarText);

  return qss;
}

} // namespace IdeTheme

#endif // IDETHEME_H
