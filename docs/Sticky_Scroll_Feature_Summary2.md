# 粘性滚动 (Sticky Scroll / 多层级类名+函数名嵌套悬挂) 功能总结报告

---

## 1. 需求背景与功能目标

在面向对象与复杂 C/C++ 项目中，代码往往具有多层嵌套作用域（例如：外层是 `class FindReplaceWidget : public QWidget`，内层是构造函数 `explicit FindReplaceWidget(...)` 或成员方法）。当开发者在类内或方法体内浏览代码时，如果仅悬挂函数名或仅悬挂类名，无法完整呈现上下文嵌套关系。

**升级后的多层级悬挂目标**：
1. **多层级嵌套作用域同屏悬挂**：
   - 当浏览类中某个长函数/构造函数时，顶部第一行悬挂**类声明行**（如 `681  class FindReplaceWidget : public QWidget {`），紧随其后的第二行悬挂**函数声明行**（如 `683    explicit FindReplaceWidget(CodeEditor *editor, QWidget *parent = nullptr)`）；
   - 在未跳出该类和该函数时，类名和函数名始终同屏悬挂固定在顶部；
2. **生命周期自适应**：
   - 当向下滚出函数体、但仍在类内部时，函数悬挂行自动消失，类声明行继续保持悬挂；
   - 当完全滚出类定义时，类声明行平滑消失；
   - 若函数本身尚未滚出视口（例如第一行可见行在函数声明上方），则只悬挂类名；
3. **独立交互与精准跳转**：
   - 悬挂条中的每一行具有独立的行号显示、语法高亮与点击事件；
   - 点击类名行直接跳转至类定义（第 681 行），点击函数名行直接跳转至函数定义（第 683 行），自动聚焦并同步更新面包屑。

---

## 2. 架构设计与状态计算

```
                    [ 触发信号 ]
 (垂直滚动 / 水平滚动 / 光标移动 / 文本修改 / 窗口缩放)
                         │
                         ▼
        [ CodeEditor::updateStickyScroll() ]
                         │
                         ▼
      获取顶端第一行可见行: firstVisibleLine
        (SCI_GETFIRSTVISIBLELINE)
                         │
                         ▼
       搜集所有包含 firstVisibleLine 的外层作用域:
   - 查找最贴近的外层类/结构体 (SymbolClass / SymbolStruct)
   - 查找包含该行的函数/方法 (SymbolFunction)
                         │
                         ▼
       按 startLine 升序排列并去重构建层级列表:
   [ Level 0: 类声明行 (681) ] -> [ Level 1: 成员函数行 (683) ]
                         │
                         ▼
             传递至 StickyScrollWidget:
   - 动态计算高度: totalHeight = lineCount * singleLineHeight
   - 逐行绘制边距行号、代码语法着色与层间分割线
   - 支持多行分别悬停 Tooltip 与独立点击跳转
```

---

## 3. 核心实现代码

### 3.1 悬挂控件多行支持 [`PhudonTools/codeeditor.cpp`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.cpp)

```cpp
// 粘性滚动 (Sticky Scroll) / 多层级顶部悬挂函数头控件 (1:1 像素级复现 VS Code 风格，支持类名+函数名嵌套悬挂)
class StickyScrollWidget : public QWidget {
public:
  struct StickyLineData {
    int targetLine = -1;
    int targetCol = 0;
    QString scopedName;
    QString rawLineText;
    QString lineNumStr;
    CodeEditor::SymbolType type = CodeEditor::SymbolFunction;
  };

  StickyScrollWidget(CodeEditor *codeEditor, QWidget *parent = nullptr)
      : QWidget(parent), m_codeEditor(codeEditor), m_hoveredIndex(-1),
        m_marginWidth(48), m_xOffset(0), m_singleLineHeight(22),
        m_editorBg("#1E1E1E"), m_marginBg("#1E1E1E"), m_marginFg("#858585"),
        m_borderColor("#3E4451"), m_accentColor("#89B4FA"),
        m_keywordColor("#569CD6"), m_typeColor("#4EC9B0"),
        m_funcColor("#DCDCAA"), m_textColor("#D4D4D4")
  {
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    m_font = QFont("Consolas", 10);
    m_font.setFixedPitch(true);
    setFont(m_font);
  }

  void updateTheme(const IdeTheme::ThemePalette &p) {
    m_editorBg = QColor(p.editorBg);
    m_marginBg = QColor(p.marginBg);
    m_marginFg = QColor(p.marginFg);
    m_borderColor = QColor(p.border);
    m_accentColor = QColor(p.accent);
    m_keywordColor = QColor(p.synKeyword);
    m_typeColor = QColor(p.synGlobalClass);
    m_funcColor = QColor(p.isDark ? "#DCDCAA" : "#795E26");
    m_textColor = QColor(p.textMain);
    update();
  }

  void setStickyLines(const QList<StickyLineData> &lines, int marginWidth, int xOffset, int lineHeight) {
    m_stickyLines = lines;
    m_marginWidth = marginWidth;
    m_xOffset = xOffset;
    m_singleLineHeight = qBound(20, lineHeight + 2, 34);

    int totalHeight = m_stickyLines.size() * m_singleLineHeight;
    setFixedHeight(qMax(totalHeight, 1));
    update();
  }

  int lineCount() const { return m_stickyLines.size(); }
  int singleLineHeight() const { return m_singleLineHeight; }

protected:
  void paintEvent(QPaintEvent *) override {
    if (m_stickyLines.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setFont(m_font);

    QFontMetrics fm = painter.fontMetrics();
    int w = width();
    int marginW = m_marginWidth;
    int lineH = m_singleLineHeight;
    int totalCount = m_stickyLines.size();
    int totalH = totalCount * lineH;

    for (int k = 0; k < totalCount; ++k) {
      const StickyLineData &lineData = m_stickyLines[k];
      int rowY = k * lineH;
      bool isHovered = (m_hoveredIndex == k);

      // 1. 绘制左侧行号边距区域 (与 Scintilla 边距 100% 对齐)
      QRect marginRect(0, rowY, marginW, lineH);
      painter.fillRect(marginRect, m_marginBg);

      // 绘制行号 (右对齐，距离右侧分割线 6px)
      painter.setPen(isHovered ? m_accentColor : m_marginFg);
      painter.drawText(marginRect.adjusted(0, 0, -6, 0), Qt::AlignRight | Qt::AlignVCenter, lineData.lineNumStr);

      // 绘制边距分割竖线
      painter.setPen(m_borderColor);
      painter.drawLine(marginW - 1, rowY, marginW - 1, rowY + lineH);

      // 2. 绘制代码内容区域背景 (悬停时微亮呈现可交互感)
      QRect contentRect(marginW, rowY, w - marginW, lineH);
      QColor contentBg = isHovered ?
          (m_editorBg.lightness() < 128 ? m_editorBg.lighter(120) : m_editorBg.darker(106)) :
          m_editorBg;
      painter.fillRect(contentRect, contentBg);

      // 3. 绘制函数/类定义代码 (支持水平滚动偏移与关键字语法着色)
      painter.save();
      painter.setClipRect(contentRect);

      int textX = marginW + 4 - m_xOffset;
      int textY = rowY + (lineH + fm.ascent() - fm.descent()) / 2;

      renderStyledCode(painter, lineData.rawLineText, textX, textY, fm);

      painter.restore();

      // 4. 行间微细分割线 (多层嵌套时层级分明)
      if (k < totalCount - 1) {
        painter.setPen(QPen(m_borderColor.lighter(105), 1, Qt::SolidLine));
        painter.drawLine(0, rowY + lineH - 1, w, rowY + lineH - 1);
      }
    }

    // 5. 绘制整个悬浮栏底部的阴影/边框线
    painter.setPen(QPen(m_hoveredIndex >= 0 ? m_accentColor : m_borderColor, 1));
    painter.drawLine(0, totalH - 1, w, totalH - 1);
  }

  void renderStyledCode(QPainter &painter, const QString &line, int startX, int baselineY, const QFontMetrics &fm) {
    static const QSet<QString> keywords = {
        "static", "inline", "virtual", "explicit", "const", "constexpr", "volatile",
        "void", "int", "char", "short", "long", "float", "double", "bool", "auto",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t",
        "size_t", "class", "struct", "enum", "union", "template", "typename", "namespace",
        "override", "final", "noexcept", "public", "protected", "private", "signals", "slots", "emit"
    };

    int curX = startX;
    int len = line.length();
    int i = 0;

    while (i < len) {
      if (line[i].isSpace()) {
        int spaceStart = i;
        while (i < len && line[i].isSpace()) ++i;
        QString spaces = line.mid(spaceStart, i - spaceStart);
        curX += fm.horizontalAdvance(spaces);
        continue;
      }

      if (line[i].isLetter() || line[i] == '_' || line[i] == '~') {
        int wordStart = i;
        while (i < len && (line[i].isLetterOrNumber() || line[i] == '_' || line[i] == '~' || line[i] == ':')) {
          if (line[i] == ':' && (i + 1 >= len || line[i + 1] != ':') && (i == 0 || line[i - 1] != ':')) {
            break;
          }
          ++i;
        }
        QString word = line.mid(wordStart, i - wordStart);
        QString bareWord = word;
        if (bareWord.contains("::")) {
          bareWord = bareWord.split("::").last();
        }

        if (keywords.contains(word) || keywords.contains(bareWord)) {
          painter.setPen(m_keywordColor);
        } else if (word.startsWith("Q") || word.endsWith("_t") || word.endsWith("TypeDef") ||
                   (word.length() > 0 && word[0].isUpper() && !word.contains('('))) {
          painter.setPen(m_typeColor);
        } else if (i < len && (line[i] == '(' || (i + 1 < len && line[i].isSpace() && line.indexOf('(', i) != -1))) {
          painter.setPen(m_funcColor);
          QFont f = painter.font();
          f.setBold(true);
          painter.setFont(f);
          painter.drawText(curX, baselineY, word);
          f.setBold(false);
          painter.setFont(f);
          curX += fm.horizontalAdvance(word);
          continue;
        } else {
          painter.setPen(m_textColor);
        }

        painter.drawText(curX, baselineY, word);
        curX += fm.horizontalAdvance(word);
        continue;
      }

      // 标点符号与运算符
      QString opStr = line.mid(i, 1);
      painter.setPen(m_textColor);
      painter.drawText(curX, baselineY, opStr);
      curX += fm.horizontalAdvance(opStr);
      ++i;
    }
  }

  void mouseMoveEvent(QMouseEvent *event) override {
    if (m_singleLineHeight > 0) {
      int idx = event->pos().y() / m_singleLineHeight;
      if (idx >= 0 && idx < m_stickyLines.size()) {
        if (m_hoveredIndex != idx) {
          m_hoveredIndex = idx;
          update();
        }
        const StickyLineData &data = m_stickyLines[idx];
        QString typeStr = (data.type == CodeEditor::SymbolClass || data.type == CodeEditor::SymbolStruct) ? "类定义" : "函数定义";
        setToolTip(QString("📍 点击跳转至%1: %2 (第 %3 行)").arg(typeStr).arg(data.scopedName).arg(data.targetLine + 1));
        return;
      }
    }
    if (m_hoveredIndex != -1) {
      m_hoveredIndex = -1;
      update();
    }
    QWidget::mouseMoveEvent(event);
  }

  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton && m_codeEditor && m_singleLineHeight > 0) {
      int idx = event->pos().y() / m_singleLineHeight;
      if (idx >= 0 && idx < m_stickyLines.size()) {
        const StickyLineData &data = m_stickyLines[idx];
        QsciScintilla *editor = m_codeEditor->currentEditor();
        if (editor && data.targetLine >= 0) {
          editor->setCursorPosition(data.targetLine, data.targetCol);
          editor->ensureLineVisible(data.targetLine);
          int visibleLines = editor->SendScintilla(QsciScintilla::SCI_LINESONSCREEN);
          int scrollLine = qMax(0, data.targetLine - visibleLines / 3);
          editor->SendScintilla(QsciScintilla::SCI_SETFIRSTVISIBLELINE, scrollLine);
          editor->setFocus();
          m_codeEditor->updateBreadcrumb(data.targetLine);
        }
      }
    }
    QWidget::mousePressEvent(event);
  }

  void enterEvent(QEvent *event) override {
    update();
    QWidget::enterEvent(event);
  }

  void leaveEvent(QEvent *event) override {
    m_hoveredIndex = -1;
    update();
    QWidget::leaveEvent(event);
  }

private:
  CodeEditor *m_codeEditor;
  QList<StickyLineData> m_stickyLines;
  int m_hoveredIndex;
  int m_marginWidth;
  int m_xOffset;
  int m_singleLineHeight;
  QFont m_font;

  QColor m_editorBg;
  QColor m_marginBg;
  QColor m_marginFg;
  QColor m_borderColor;
  QColor m_accentColor;
  QColor m_keywordColor;
  QColor m_typeColor;
  QColor m_funcColor;
  QColor m_textColor;
};
```

---

### 3.2 多层嵌套作用域计算 `CodeEditor::updateStickyScroll()`

```cpp
void CodeEditor::updateStickyScroll() {
  if (!m_currentEditor || !m_stickyScrollWidget) return;

  int firstVisibleLine = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETFIRSTVISIBLELINE);
  int totalLines = m_currentEditor->lines();
  if (firstVisibleLine < 0 || firstVisibleLine >= totalLines) {
    m_stickyScrollWidget->hide();
    return;
  }

  // 搜集所有包含当前首可见行 firstVisibleLine 的外层作用域（类、结构体、函数/方法）
  // 按照外层到内层（startLine 升序）构建多级嵌套悬挂列表
  QList<const FunctionInfo *> matchingScopes;

  // 1. 查找最贴近的外层类 / 结构体 (SymbolClass, SymbolStruct)
  const FunctionInfo *enclosingClass = nullptr;
  for (const FunctionInfo &fn : m_functions) {
    if (fn.type == SymbolClass || fn.type == SymbolStruct) {
      if (firstVisibleLine > fn.startLine && firstVisibleLine <= fn.endLine) {
        if (!enclosingClass || (fn.startLine >= enclosingClass->startLine && fn.endLine <= enclosingClass->endLine)) {
          enclosingClass = &fn;
        }
      }
    }
  }

  if (enclosingClass) {
    matchingScopes.append(enclosingClass);
  }

  // 2. 查找包含 firstVisibleLine 的最内层函数/方法 (SymbolFunction)
  const FunctionInfo *enclosingFunc = nullptr;
  for (const FunctionInfo &fn : m_functions) {
    if (fn.type == SymbolFunction) {
      if (firstVisibleLine > fn.startLine && firstVisibleLine <= fn.endLine) {
        if (!enclosingFunc || (fn.startLine >= enclosingFunc->startLine && fn.endLine <= enclosingFunc->endLine)) {
          enclosingFunc = &fn;
        }
      }
    }
  }

  if (enclosingFunc && enclosingFunc != enclosingClass) {
    matchingScopes.append(enclosingFunc);
  }

  if (matchingScopes.isEmpty()) {
    m_stickyScrollWidget->hide();
    return;
  }

  // 按照 startLine 升序排列 (外层类在上方，内部函数在下方)
  std::sort(matchingScopes.begin(), matchingScopes.end(), [](const FunctionInfo *a, const FunctionInfo *b) {
    return a->startLine < b->startLine;
  });

  // 去重 (相同起始行仅保留一个)
  QList<const FunctionInfo *> uniqueScopes;
  for (const FunctionInfo *scope : matchingScopes) {
    bool dup = false;
    for (const FunctionInfo *u : uniqueScopes) {
      if (u->startLine == scope->startLine) {
        dup = true;
        break;
      }
    }
    if (!dup) {
      uniqueScopes.append(scope);
    }
  }

  // 构建 StickyLineData 列表
  QList<StickyScrollWidget::StickyLineData> linesData;
  for (const FunctionInfo *scope : uniqueScopes) {
    int defLine = scope->startLine;
    if (defLine < 0 || defLine >= totalLines) continue;

    QString lineText = m_currentEditor->text(defLine);
    while (lineText.endsWith('\r') || lineText.endsWith('\n')) {
      lineText.chop(1);
    }

    StickyScrollWidget::StickyLineData item;
    item.targetLine = scope->startLine;
    item.targetCol = scope->startCol;
    item.scopedName = scope->scopedName;
    item.rawLineText = lineText;
    item.lineNumStr = QString::number(scope->startLine + 1);
    item.type = scope->type;
    linesData.append(item);
  }

  if (linesData.isEmpty()) {
    m_stickyScrollWidget->hide();
    return;
  }

  // 计算边距宽度、水平滚动偏移与行高 (与 Scintilla 视口 100% 像素对齐)
  int margin0 = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETMARGINWIDTHN, 0);
  int margin1 = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETMARGINWIDTHN, 1);
  int margin2 = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETMARGINWIDTHN, 2);
  int totalMarginWidth = margin0 + margin1 + margin2;
  int xOffset = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETXOFFSET);
  int lineHeight = m_currentEditor->SendScintilla(QsciScintilla::SCI_TEXTHEIGHT, 0);
  if (lineHeight <= 0) lineHeight = 20;

  static_cast<StickyScrollWidget *>(m_stickyScrollWidget)->setStickyLines(
      linesData, totalMarginWidth, xOffset, lineHeight);

  int vBarWidth = (m_currentEditor->verticalScrollBar() && m_currentEditor->verticalScrollBar()->isVisible())
                      ? m_currentEditor->verticalScrollBar()->width()
                      : 0;
  int widgetWidth = m_currentEditor->width() - vBarWidth;
  int widgetHeight = static_cast<StickyScrollWidget *>(m_stickyScrollWidget)->height();

  if (m_stickyScrollWidget->parent() != m_currentEditor) {
    m_stickyScrollWidget->setParent(m_currentEditor);
  }
  m_stickyScrollWidget->setGeometry(0, 0, widgetWidth, widgetHeight);
  m_stickyScrollWidget->show();
  m_stickyScrollWidget->raise();
}
```

---

## 4. 编译验证
- 执行构建命令：`mingw32-make -j8`
- 编译状态：**Exit Code 0**（零错误零警告通过编译并完成链接打包）。
