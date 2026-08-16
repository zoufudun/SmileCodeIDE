#ifndef CANTHEME_H
#define CANTHEME_H

#include "idetheme.h"
#include <QString>
#include <QStringList>

// CAN 工具的界面主题体系 (与全 IDE 旗舰主题深度融合)
namespace CanTheme {

struct Palette {
  QString window;    // 窗口背景
  QString panel;     // 分组框/面板背景
  QString base;      // 输入框/表格背景
  QString alt;       // 表格交替行
  QString text;      // 主文字
  QString subtext;   // 次要文字
  QString border;    // 边框
  QString accent;    // 主强调色
  QString accentHi;  // 强调色悬停
  QString accentTxt; // 强调色上的文字
  QString selection; // 选中行背景
};

inline Palette paletteFor(const QString &name) {
  QString mappedId = name;
  if (name.contains(QStringLiteral("深色")) || name.contains("Dark")) {
    mappedId = "dark";
  } else if (name.contains(QStringLiteral("浅色")) || name.contains("Light")) {
    mappedId = "light";
  } else if (name.contains(QStringLiteral("赛博朋克")) || name.contains("Cyber")) {
    mappedId = "cyberneon";
  } else if (name.contains("Dracula")) {
    mappedId = "dracula";
  } else if (name.contains("Nord")) {
    mappedId = "nord";
  } else if (name.contains("One Dark")) {
    mappedId = "onedark";
  } else if (name.contains("Monokai")) {
    mappedId = "monokaipro";
  } else if (name.contains(QStringLiteral("熔岩")) || name.contains("Gold") || name.contains("Ember")) {
    mappedId = "obsidiangold";
  }

  const IdeTheme::ThemePalette p = IdeTheme::paletteFor(mappedId);
  return {
    p.windowBg,
    p.panelBg,
    p.cardBg,
    p.baseAltBg,
    p.textMain,
    p.textSub,
    p.border,
    p.accent,
    p.accentHover,
    p.accentText,
    p.selectionBg
  };
}

inline QStringList names() {
  return {
    QStringLiteral("⚡ 极客钛金 (Titanium Dark)"),
    QStringLiteral("🌌 赛博霓虹 (Cyber Neon)"),
    QStringLiteral("🌋 熔岩黑金 (Obsidian Gold)"),
    QStringLiteral("🔮 星云紫晶 (Dracula)"),
    QStringLiteral("🌊 碧海深渊 (Nord)"),
    QStringLiteral("🌲 极客翡翠 (Vue Matrix)"),
    QStringLiteral("☀️ 纯白曜石 (Crystal Light)"),
    QStringLiteral("📜 暖阳羊皮 (Solarized Light)")
  };
}

inline QString styleSheet(const QString &name) {
  QString mappedId = name;
  if (name.contains(QStringLiteral("深色")) || name.contains("Dark") || name.contains(QStringLiteral("钛金"))) {
    mappedId = "dark";
  } else if (name.contains(QStringLiteral("浅色")) || name.contains("Light") || name.contains(QStringLiteral("纯白"))) {
    mappedId = "light";
  } else if (name.contains(QStringLiteral("赛博")) || name.contains("Cyber")) {
    mappedId = "cyberneon";
  } else if (name.contains("Dracula") || name.contains(QStringLiteral("星云"))) {
    mappedId = "dracula";
  } else if (name.contains("Nord") || name.contains(QStringLiteral("碧海"))) {
    mappedId = "nord";
  } else if (name.contains(QStringLiteral("翡翠")) || name.contains("Vue")) {
    mappedId = "vue";
  } else if (name.contains(QStringLiteral("熔岩")) || name.contains("Gold") || name.contains("Ember")) {
    mappedId = "obsidiangold";
  } else if (name.contains(QStringLiteral("羊皮")) || name.contains("Solarized")) {
    mappedId = "solarizedlight";
  }

  return IdeTheme::generateStyleSheet(mappedId);
}

} // namespace CanTheme

#endif // CANTHEME_H
