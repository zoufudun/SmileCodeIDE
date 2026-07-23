#ifndef CANTHEME_H
#define CANTHEME_H

#include <QString>
#include <QStringList>

// CAN 工具的界面主题。内置若干流行配色，通过调色板生成统一的 QSS 样式表。
namespace CanTheme {

struct Palette {
  QString window;    // 窗口背景
  QString panel;     // 分组框/面板背景
  QString base;      // 输入框/表格背景
  QString alt;       // 表格交替行
  QString text;      // 主文字
  QString subtext;   // 次要文字（标题等）
  QString border;    // 边框
  QString accent;    // 主强调色（按钮、选中）
  QString accentHi;  // 强调色悬停
  QString accentTxt; // 强调色上的文字
  QString selection; // 选中行背景
};

inline Palette paletteFor(const QString &name) {
  if (name == QStringLiteral("深色 (Dark)")) {
    return {"#282c34", "#2f343f", "#21252b", "#2c313a", "#abb2bf",
            "#7f8794", "#3b4048", "#61afef", "#7cc0ff", "#0d1117",
            "#3a3f4b"};
  }
  if (name == QStringLiteral("Dracula")) {
    return {"#282a36", "#343746", "#21222c", "#2b2e3b", "#f8f8f2",
            "#bd93f9", "#44475a", "#bd93f9", "#caa6ff", "#21222c",
            "#44475a"};
  }
  if (name == QStringLiteral("Nord")) {
    return {"#2e3440", "#3b4252", "#272c36", "#39404e", "#eceff4",
            "#88c0d0", "#434c5e", "#88c0d0", "#8fbcbb", "#2e3440",
            "#434c5e"};
  }
  if (name == QStringLiteral("GitHub Dark")) {
    return {"#0d1117", "#161b22", "#010409", "#11161d", "#c9d1d9",
            "#8b949e", "#30363d", "#238636", "#2ea043", "#ffffff",
            "#1f6feb"};
  }
  if (name == QStringLiteral("One Dark")) {
    return {"#282c34", "#21252b", "#1e222a", "#2c313c", "#abb2bf",
            "#5c6370", "#3e4452", "#98c379", "#aed581", "#282c34",
            "#3e4452"};
  }
  if (name == QStringLiteral("Monokai")) {
    return {"#272822", "#1e1f1c", "#2d2e2c", "#3e3d32", "#f8f8f2",
            "#75715e", "#49483e", "#a6e22e", "#b3e5fc", "#272822",
            "#49483e"};
  }
  if (name == QStringLiteral("赛博朋克 (Cyberpunk)")) {
    return {"#120424", "#1d0b3a", "#0d021a", "#2a0c4f", "#00f0ff",
            "#ff007f", "#ff007f", "#ff007f", "#00f0ff", "#120424",
            "#36136b"};
  }
  // 默认：浅色 (Light)
  return {"#f5f6f8", "#ffffff", "#ffffff", "#f0f3f7", "#2c3e50",
          "#5b6b7b", "#d6dbe1", "#2d8cf0", "#57a3f3", "#ffffff",
          "#e3f0ff"};
}

inline QStringList names() {
  return {QStringLiteral("深色 (Dark)"), QStringLiteral("浅色 (Light)"),
          QStringLiteral("Dracula"), QStringLiteral("Nord"),
          QStringLiteral("GitHub Dark"), QStringLiteral("One Dark"),
          QStringLiteral("Monokai"), QStringLiteral("赛博朋克 (Cyberpunk)")};
}

inline QString styleSheet(const QString &name) {
  const Palette p = paletteFor(name);
  QString s = QStringLiteral(R"QSS(
* { font-family: "Microsoft YaHei", "Segoe UI", Arial; font-size: 13px; }
QDialog, QWidget#canRoot { background: {WINDOW}; color: {TEXT}; }
QLabel { color: {TEXT}; background: transparent; }
QGroupBox {
  background: {PANEL};
  border: 1px solid {BORDER};
  border-radius: 8px;
  margin-top: 14px;
  padding: 10px 10px 8px 10px;
  font-weight: bold;
  color: {SUBTEXT};
}
QGroupBox::title {
  subcontrol-origin: margin;
  subcontrol-position: top left;
  left: 12px;
  padding: 0 5px;
}
QPushButton {
  background: {ACCENT};
  color: {ACCENTTXT};
  border: none;
  border-radius: 6px;
  padding: 6px 14px;
  font-weight: bold;
}
QPushButton:hover { background: {ACCENTHI}; }
QPushButton:pressed { background: {ACCENT}; padding-top: 7px; }
QPushButton:disabled { background: {BORDER}; color: {SUBTEXT}; }
QComboBox, QLineEdit, QSpinBox, QPlainTextEdit, QTextEdit {
  background: {BASE};
  color: {TEXT};
  border: 1px solid {BORDER};
  border-radius: 6px;
  padding: 4px 8px;
  selection-background-color: {ACCENT};
  selection-color: {ACCENTTXT};
}
QComboBox:!editable {
  background: {BASE};
  color: {TEXT};
}
QComboBox:hover, QLineEdit:hover, QSpinBox:hover { border: 1px solid {ACCENT}; }
QComboBox::drop-down { border: none; width: 20px; }
QComboBox QAbstractItemView {
  background: {BASE};
  color: {TEXT};
  border: 1px solid {BORDER};
  selection-background-color: {ACCENT};
  selection-color: {ACCENTTXT};
  outline: 0;
}
QCheckBox, QRadioButton { color: {TEXT}; spacing: 6px; background: transparent; }
QCheckBox::indicator, QRadioButton::indicator { width: 16px; height: 16px; }
QCheckBox::indicator {
  border: 1px solid {BORDER}; border-radius: 4px; background: {BASE};
}
QRadioButton::indicator {
  border: 1px solid {BORDER}; border-radius: 8px; background: {BASE};
}
QCheckBox::indicator:checked, QRadioButton::indicator:checked {
  background: {ACCENT}; border: 1px solid {ACCENT};
}
QTreeWidget, QTableWidget, QTreeView, QTableView {
  background: {BASE};
  alternate-background-color: {ALT};
  color: {TEXT};
  border: 1px solid {BORDER};
  border-radius: 6px;
  gridline-color: {BORDER};
  outline: 0;
}
QTreeWidget::item, QTableWidget::item { padding: 3px 4px; }
QTreeWidget::item:selected, QTableWidget::item:selected {
  background: {SELECTION}; color: {TEXT};
}
QHeaderView::section {
  background: {PANEL};
  color: {SUBTEXT};
  border: none;
  border-right: 1px solid {BORDER};
  border-bottom: 1px solid {BORDER};
  padding: 6px 8px;
  font-weight: bold;
}
QTabWidget::pane { border: 1px solid {BORDER}; border-radius: 8px; top: -1px; }
QTabBar::tab {
  background: transparent;
  color: {SUBTEXT};
  padding: 8px 18px;
  border-top-left-radius: 6px;
  border-top-right-radius: 6px;
  margin-right: 2px;
}
QTabBar::tab:selected {
  background: {PANEL};
  color: {ACCENT};
  border: 1px solid {BORDER};
  border-bottom: 2px solid {ACCENT};
}
QTabBar::tab:hover:!selected { color: {TEXT}; }
QScrollBar:vertical { background: transparent; width: 11px; margin: 2px; }
QScrollBar::handle:vertical {
  background: {BORDER}; border-radius: 5px; min-height: 28px;
}
QScrollBar::handle:vertical:hover { background: {ACCENT}; }
QScrollBar:horizontal { background: transparent; height: 11px; margin: 2px; }
QScrollBar::handle:horizontal {
  background: {BORDER}; border-radius: 5px; min-width: 28px;
}
QScrollBar::handle:horizontal:hover { background: {ACCENT}; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
)QSS");

  s.replace("{WINDOW}", p.window);
  s.replace("{PANEL}", p.panel);
  s.replace("{BASE}", p.base);
  s.replace("{ALT}", p.alt);
  s.replace("{TEXT}", p.text);
  s.replace("{SUBTEXT}", p.subtext);
  s.replace("{BORDER}", p.border);
  s.replace("{ACCENTHI}", p.accentHi);
  s.replace("{ACCENTTXT}", p.accentTxt);
  s.replace("{ACCENT}", p.accent);
  s.replace("{SELECTION}", p.selection);
  return s;
}

} // namespace CanTheme

#endif // CANTHEME_H
