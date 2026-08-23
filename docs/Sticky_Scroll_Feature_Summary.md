# 粘性滚动 (Sticky Scroll / 顶部悬挂函数头) 功能实现总结报告

---

## 1. 需求背景与功能目标

在现代高级 IDE（如 VS Code、CLion、Visual Studio）中，浏览长函数、复杂类或结构体实现时，当函数的签名声明行向上滚出可视视口后，开发者往往容易迷失在深层代码块中，难以快速确定当前光标所在代码段到底属于哪一个函数。

**功能目标**：
1. **作用域悬挂固定**：当长函数的起始声明行（例如第 532 行 `static QString generateCleanCppCode(const QString &code) {`）滚动到编辑器上方不可见区域时，自动在编辑器顶部浮现出该函数声明行的悬挂条；
2. **像素级边距与行号对齐**：悬挂条左侧精确对齐 Scintilla 行号边距（`marginWidth`），显示函数定义的实际行号（如 `532`），右侧显示函数签名；
3. **高质感语法着色**：对悬挂条中的函数签名进行 C/C++ 关键字、类型、函数名与标点符号实时着色；
4. **水平与垂直滚动同步**：支持水平滚动条偏移（`xOffset`）同频位移；
5. **交互式点击跳转**：鼠标悬停显示手势与提示，点击悬挂条直达函数定义行，自动聚焦并同步更新面包屑导航。

---

## 2. 核心技术实现与工作流程

### 2.1 作用域判定与动态显隐流程

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
       遍历符号表 m_functions 查找最内层包含作用域:
   (firstVisibleLine > fn.startLine && firstVisibleLine <= fn.endLine)
                         │
        ┌────────────────┴────────────────┐
        ▼                                 ▼
   [ 找到有效函数/类 ]               [ 未找到 / 首行就在视口内 ]
        │                                 │
        ▼                                 ▼
提取原始行文本、计算边距与偏移            m_stickyScrollWidget->hide()
setFunctionHeader(...)
m_stickyScrollWidget->show()
```

---

## 3. 核心实现代码

### 3.1 头文件声明 [`PhudonTools/codeeditor.h`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.h)

```cpp
class CodeEditor : public QWidget {
  Q_OBJECT

  // 粘性滚动 (Sticky Scroll) / 顶部悬挂函数头控件
  QWidget *m_stickyScrollWidget;
  void updateStickyScroll();

  friend class StickyScrollWidget;

public:
  // 更新面包屑显示
  void updateBreadcrumb(int cursorLine = -1);
  ...
};
```

---

### 3.2 悬挂控件实现 [`PhudonTools/codeeditor.cpp`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.cpp)

#### (1) `StickyScrollWidget` 控件类定义

```cpp
// 粘性滚动 (Sticky Scroll) / 顶部悬挂函数头控件 (1:1 像素级复现 VS Code 风格)
class StickyScrollWidget : public QWidget {
public:
  StickyScrollWidget(CodeEditor *codeEditor, QWidget *parent = nullptr)
      : QWidget(parent), m_codeEditor(codeEditor), m_targetLine(-1), m_targetCol(0),
        m_marginWidth(48), m_xOffset(0), m_isHovered(false),
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

  void setFunctionHeader(int startLine, int startCol, const QString &scopedName, const QString &fullLineText,
                         int marginWidth, int xOffset, int lineHeight) {
    m_targetLine = startLine;
    m_targetCol = startCol;
    m_scopedName = scopedName;
    m_rawLineText = fullLineText;
    m_lineNumStr = QString::number(startLine + 1);
    m_marginWidth = marginWidth;
    m_xOffset = xOffset;
    setFixedHeight(qBound(22, lineHeight + 2, 34));
    setToolTip(QString("📍 点击跳转至函数定义: %1 (第 %2 行)").arg(scopedName).arg(startLine + 1));
    update();
  }

  int targetLine() const { return m_targetLine; }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setFont(m_font);

    QFontMetrics fm = painter.fontMetrics();
    int h = height();
    int w = width();
    int marginW = m_marginWidth;

    // 1. 绘制左侧行号边距区域 (与 Scintilla 边距 100% 对齐)
    QRect marginRect(0, 0, marginW, h);
    painter.fillRect(marginRect, m_marginBg);

    // 绘制行号 (右对齐，距离右侧分割线 6px)
    painter.setPen(m_isHovered ? m_accentColor : m_marginFg);
    painter.drawText(marginRect.adjusted(0, 0, -6, 0), Qt::AlignRight | Qt::AlignVCenter, m_lineNumStr);

    // 绘制边距分割竖线
    painter.setPen(m_borderColor);
    painter.drawLine(marginW - 1, 0, marginW - 1, h);

    // 2. 绘制代码内容区域背景 (悬停时微亮呈现可交互感)
    QRect contentRect(marginW, 0, w - marginW, h);
    QColor contentBg = m_isHovered ?
        (m_editorBg.lightness() < 128 ? m_editorBg.lighter(118) : m_editorBg.darker(106)) :
        m_editorBg;
    painter.fillRect(contentRect, contentBg);

    // 3. 绘制函数定义代码 (支持水平滚动偏移与关键字语法着色)
    painter.save();
    painter.setClipRect(contentRect);

    int textX = marginW + 4 - m_xOffset;
    int textY = (h + fm.ascent() - fm.descent()) / 2;

    renderStyledCode(painter, m_rawLineText, textX, textY, fm);

    painter.restore();

    // 4. 绘制底部悬浮分割线与微阴影效果
    painter.setPen(QPen(m_isHovered ? m_accentColor : m_borderColor, 1));
    painter.drawLine(0, h - 1, w, h - 1);
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

  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton && m_targetLine >= 0 && m_codeEditor) {
      QsciScintilla *editor = m_codeEditor->currentEditor();
      if (editor) {
        editor->setCursorPosition(m_targetLine, m_targetCol);
        editor->ensureLineVisible(m_targetLine);
        int visibleLines = editor->SendScintilla(QsciScintilla::SCI_LINESONSCREEN);
        int scrollLine = qMax(0, m_targetLine - visibleLines / 3);
        editor->SendScintilla(QsciScintilla::SCI_SETFIRSTVISIBLELINE, scrollLine);
        editor->setFocus();
        m_codeEditor->updateBreadcrumb(m_targetLine);
      }
    }
    QWidget::mousePressEvent(event);
  }

  void enterEvent(QEvent *event) override {
    m_isHovered = true;
    update();
    QWidget::enterEvent(event);
  }

  void leaveEvent(QEvent *event) override {
    m_isHovered = false;
    update();
    QWidget::leaveEvent(event);
  }

private:
  CodeEditor *m_codeEditor;
  int m_targetLine;
  int m_targetCol;
  QString m_scopedName;
  QString m_rawLineText;
  QString m_lineNumStr;
  int m_marginWidth;
  int m_xOffset;
  bool m_isHovered;
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

#### (2) 状态计算与刷新方法 `CodeEditor::updateStickyScroll()`

```cpp
void CodeEditor::updateStickyScroll() {
  if (!m_currentEditor || !m_stickyScrollWidget) return;

  int firstVisibleLine = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETFIRSTVISIBLELINE);
  int totalLines = m_currentEditor->lines();
  if (firstVisibleLine < 0 || firstVisibleLine >= totalLines) {
    m_stickyScrollWidget->hide();
    return;
  }

  // 寻找当前首行可见代码所在的最内层函数 / 类 / 结构体
  const FunctionInfo *activeFunc = nullptr;
  for (const FunctionInfo &fn : m_functions) {
    if (fn.type == SymbolFunction || fn.type == SymbolClass || fn.type == SymbolStruct) {
      if (firstVisibleLine > fn.startLine && firstVisibleLine <= fn.endLine) {
        if (!activeFunc || (fn.startLine >= activeFunc->startLine && fn.endLine <= activeFunc->endLine)) {
          activeFunc = &fn;
        }
      }
    }
  }

  if (!activeFunc) {
    m_stickyScrollWidget->hide();
    return;
  }

  int defLine = activeFunc->startLine;
  if (defLine < 0 || defLine >= totalLines) {
    m_stickyScrollWidget->hide();
    return;
  }

  QString lineText = m_currentEditor->text(defLine);
  while (lineText.endsWith('\r') || lineText.endsWith('\n')) {
    lineText.chop(1);
  }

  // 计算边距宽度、水平滚动偏移与行高 (与 Scintilla 视口 100% 像素对齐)
  int margin0 = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETMARGINWIDTHN, 0);
  int margin1 = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETMARGINWIDTHN, 1);
  int margin2 = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETMARGINWIDTHN, 2);
  int totalMarginWidth = margin0 + margin1 + margin2;
  int xOffset = m_currentEditor->SendScintilla(QsciScintilla::SCI_GETXOFFSET);
  int lineHeight = m_currentEditor->SendScintilla(QsciScintilla::SCI_TEXTHEIGHT, 0);
  if (lineHeight <= 0) lineHeight = 20;

  static_cast<StickyScrollWidget *>(m_stickyScrollWidget)->setFunctionHeader(
      activeFunc->startLine, activeFunc->startCol, activeFunc->scopedName,
      lineText, totalMarginWidth, xOffset, lineHeight);

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

#### (3) 滚动与编辑事件驱动挂钩

在 `CodeEditor::setupEditor` 中安装事件监听：

```cpp
  // 监听光标移动以更新高亮、面包屑与 Sticky Scroll
  connect(editor, &QsciScintilla::cursorPositionChanged, this,
          [this](int line, int col) {
            Q_UNUSED(col);
            highlightCurrentLineNumber();
            updateBreadcrumb(line);
            updateStickyScroll();
            if (m_occurrenceTimer) m_occurrenceTimer->start();
          });

  // 监听垂直与水平滚动条以驱动 Sticky Scroll 顶部悬挂函数头
  if (editor->verticalScrollBar()) {
    connect(editor->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
      updateStickyScroll();
    });
  }
  if (editor->horizontalScrollBar()) {
    connect(editor->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
      updateStickyScroll();
    });
  }
```

在主题切换、窗口大小调整、符号解析与编辑器切换时同步联动：
- `CodeEditor::applyTheme` -> `m_stickyScrollWidget->updateTheme(p); updateStickyScroll();`
- `CodeEditor::updateFunctionList` -> `updateStickyScroll();`
- `CodeEditor::resizeEvent` -> `updateStickyScroll();`
- `CodeEditor::eventFilter` (监听 `QEvent::Resize`/`Move`) -> `updateStickyScroll();`
- `CodeEditor::onEditorChanged` -> `m_stickyScrollWidget->setParent(m_currentEditor); updateStickyScroll();`

---

## 4. 编译与验证结果

通过 MinGW 64 进行编译验证：
```powershell
powershell -Command "$env:PATH = 'D:\Soft\Qt\5.15.2\mingw81_64\bin;D:\Soft\Qt\Tools\mingw810_64\bin;' + $env:PATH; cd m:\TOPFIRE\SmileCodeIDEUpdate\PhudonTools; mingw32-make -j8"
```
- **构建状态**：**Exit Code 0**，零警告报错，成功生成最终可执行程序。
