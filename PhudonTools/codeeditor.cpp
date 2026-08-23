/*
 * @Description:
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-05 21:44:22
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-08-11 14:23:04
 */

#include "codeeditor.h"
#include "idetheme.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPair>
#include <QPushButton>
#include <QRegExp>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QStack>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextCodec>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include <vector>

// 绘制函数/方法/槽矢量图标（紫色 3D 棱箱/立方体线框，1:1 严格对齐截图 2、截图 3 呈现风格）
static QIcon createSymbolIcon(const QColor &color = QColor("#A855F7"), int size = 18) {
  QPixmap pix(size, size);
  pix.fill(Qt::transparent);
  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(QColor(color.red(), color.green(), color.blue(), 30));

  qreal cx = size / 2.0;
  qreal cy = size / 2.0;
  qreal r = size * 0.42;
  qreal dx = r * 0.8660254; // cos(30 deg)
  qreal dy = r * 0.5;       // sin(30 deg)

  QPointF pTop(cx, cy - r);
  QPointF pTopRight(cx + dx, cy - dy);
  QPointF pBottomRight(cx + dx, cy + dy);
  QPointF pBottom(cx, cy + r);
  QPointF pBottomLeft(cx - dx, cy + dy);
  QPointF pTopLeft(cx - dx, cy - dy);
  QPointF pCenter(cx, cy);

  QPolygonF hex;
  hex << pTop << pTopRight << pBottomRight << pBottom << pBottomLeft << pTopLeft;
  painter.drawPolygon(hex);

  // 绘制 3 条中心棱线构成经典 3D 几何立方体
  painter.drawLine(pCenter, pTop);
  painter.drawLine(pCenter, pBottomLeft);
  painter.drawLine(pCenter, pBottomRight);

  return QIcon(pix);
}

// 绘制函数大纲列表矢量图标
static QIcon createOutlineIcon(const QColor &color = QColor("#A855F7"), int size = 16) {
  QPixmap pix(size, size);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

  int yPositions[] = {3, 8, 13};
  for (int y : yPositions) {
    p.setBrush(QBrush(color));
    p.drawEllipse(QPointF(3, y), 1.2, 1.2);
    p.drawLine(QPointF(6, y), QPointF(13, y));
  }
  return QIcon(pix);
}

// 绘制变量矢量图标（青色方形小徽章，对齐图 3 风格）
static QIcon createVariableIcon(const QColor &color = QColor("#06B6D4"), int size = 18) {
  QPixmap pix(size, size);
  pix.fill(Qt::transparent);
  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(QColor(color.red(), color.green(), color.blue(), 35));

  painter.drawRoundedRect(QRectF(2.5, 2.5, size - 5, size - 5), 3, 3);
  painter.setBrush(color);
  painter.setPen(Qt::NoPen);
  painter.drawEllipse(QPointF(size / 2.0, size / 2.0), 1.6, 1.6);
  return QIcon(pix);
}

// 绘制文件类型徽标 (如 C, C++, H 浅青色徽章)
static QIcon createFileBadgeIcon(const QString &badge, const QColor &color = QColor("#00E5FF"), int size = 16) {
  int w = (badge.length() > 2) ? size * 2 : size + 6;
  QPixmap pix(w, size);
  pix.fill(Qt::transparent);
  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);

  QFont font("Segoe UI", 9, QFont::Bold);
  painter.setFont(font);
  painter.setPen(color);
  painter.drawText(QRectF(0, 0, w, size), Qt::AlignCenter, badge);
  return QIcon(pix);
}

// 绘制宏定义矢量图标（金黄色 # 徽标）
static QIcon createMacroIcon(const QColor &color = QColor("#F59E0B"), int size = 18) {
  QPixmap pix(size, size);
  pix.fill(Qt::transparent);
  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);

  painter.drawRoundedRect(QRectF(1.5, 1.5, size - 3, size - 3), 3.5, 3.5);

  QFont font("Consolas", 10, QFont::Bold);
  painter.setFont(font);
  painter.setPen(color);
  painter.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, "#");
  return QIcon(pix);
}

// 绘制类矢量图标（金黄色类符号）
static QIcon createClassIcon(const QColor &color = QColor("#EAB308"), int size = 18) {
  QPixmap pix(size, size);
  pix.fill(Qt::transparent);
  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(QColor(color.red(), color.green(), color.blue(), 30));

  painter.drawRoundedRect(QRectF(2, 2, size - 4, size - 4), 3, 3);
  painter.drawLine(QPointF(2, 6.5), QPointF(size - 2, 6.5));
  painter.drawLine(QPointF(6.5, 6.5), QPointF(6.5, size - 2));
  return QIcon(pix);
}

// 绘制结构体矢量图标（翡翠绿十字结构体徽标）
static QIcon createStructIcon(const QColor &color = QColor("#10B981"), int size = 18) {
  QPixmap pix(size, size);
  pix.fill(Qt::transparent);
  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(QColor(color.red(), color.green(), color.blue(), 30));

  painter.drawRoundedRect(QRectF(2, 2, size - 4, size - 4), 3, 3);
  painter.drawLine(QPointF(size / 2.0, 3.5), QPointF(size / 2.0, size - 3.5));
  painter.drawLine(QPointF(3.5, size / 2.0), QPointF(size - 3.5, size / 2.0));
  return QIcon(pix);
}

// 绘制联合体矢量图标（橙色 U 徽标）
static QIcon createUnionIcon(const QColor &color = QColor("#F97316"), int size = 18) {
  QPixmap pix(size, size);
  pix.fill(Qt::transparent);
  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(QRectF(1.5, 1.5, size - 3, size - 3), 3.5, 3.5);

  QFont font("Segoe UI", 9, QFont::Bold);
  painter.setFont(font);
  painter.setPen(color);
  painter.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, "U");
  return QIcon(pix);
}

// 绘制枚举矢量图标（浅紫色 E 徽标）
static QIcon createEnumIcon(const QColor &color = QColor("#8B5CF6"), int size = 18) {
  QPixmap pix(size, size);
  pix.fill(Qt::transparent);
  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(QRectF(1.5, 1.5, size - 3, size - 3), 3.5, 3.5);

  QFont font("Segoe UI", 9, QFont::Bold);
  painter.setFont(font);
  painter.setPen(color);
  painter.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, "E");
  return QIcon(pix);
}

// 根据符号类型获取对应的矢量图标
static QIcon getIconForSymbol(CodeEditor::SymbolType type) {
  switch (type) {
  case CodeEditor::SymbolVariable:
    return createVariableIcon(QColor("#06B6D4"), 18);
  case CodeEditor::SymbolMacro:
    return createMacroIcon(QColor("#F59E0B"), 18);
  case CodeEditor::SymbolClass:
    return createClassIcon(QColor("#EAB308"), 18);
  case CodeEditor::SymbolStruct:
    return createStructIcon(QColor("#10B981"), 18);
  case CodeEditor::SymbolUnion:
    return createUnionIcon(QColor("#F97316"), 18);
  case CodeEditor::SymbolEnum:
    return createEnumIcon(QColor("#8B5CF6"), 18);
  case CodeEditor::SymbolFunction:
  default:
    return createSymbolIcon(QColor("#A855F7"), 18);
  }
}

// 格式化符号显示文本 (1:1 严格对齐图 3 格式: 函数名 返回类型 参数列表 / 变量名 类型)
static QString formatSymbolDisplay(const CodeEditor::FunctionInfo &info) {
  if (info.type == CodeEditor::SymbolClass || info.type == CodeEditor::SymbolStruct ||
      info.type == CodeEditor::SymbolUnion || info.type == CodeEditor::SymbolEnum) {
    return info.scopedName;
  }

  if (info.type == CodeEditor::SymbolVariable) {
    if (!info.returnType.isEmpty()) {
      return QString("%1 %2").arg(info.scopedName, info.returnType);
    }
    return info.scopedName;
  }

  // SymbolFunction: 构造函数与析构函数
  if (info.returnType.isEmpty() || info.scopedName.startsWith('~') ||
      info.scopedName == info.parentClass || (!info.parentClass.isEmpty() && info.scopedName == info.parentClass + "::" + info.parentClass)) {
    return QString("%1 %2").arg(info.scopedName, info.params.isEmpty() ? "()" : info.params);
  }

  // 普通函数与成员方法: FuncName ReturnType Params (如 Wait_SDCARD_Ready int (void))
  return QString("%1 %2 %3").arg(info.scopedName, info.returnType, info.params.isEmpty() ? "(void)" : info.params);
}

// 将复杂函数形参签名清洗为纯类型签名，例如：
// "(const QColor &color = QColor(...), int size = 18)" -> "(const QColor &, int)"
// "(uint8_t *data, int len)" -> "(uint8_t *, int)"
// "(void)" -> "(void)"
static QString normalizeParamTypes(const QString &rawParamStr) {
  QString raw = rawParamStr.trimmed();
  if (raw.isEmpty()) return "(void)";
  if (!raw.startsWith('(')) raw = "(" + raw;
  if (!raw.endsWith(')')) raw = raw + ")";
  if (raw == "()") return "(void)";
  if (raw == "(void)") return raw;

  QString inner = raw.mid(1, raw.length() - 2).trimmed();
  if (inner.isEmpty()) return "(void)";
  if (inner == "void") return "(void)";

  // 按逗号分割形参（保护嵌套的括号、尖括号、方括号）
  QStringList rawParams;
  int pDepth = 0, aDepth = 0, bDepth = 0;
  int last = 0;
  for (int i = 0; i < inner.length(); ++i) {
    QChar c = inner[i];
    if (c == '(') ++pDepth;
    else if (c == ')') --pDepth;
    else if (c == '<') ++aDepth;
    else if (c == '>') --aDepth;
    else if (c == '[') ++bDepth;
    else if (c == ']') --bDepth;
    else if (c == ',' && pDepth == 0 && aDepth == 0 && bDepth == 0) {
      rawParams.append(inner.mid(last, i - last).trimmed());
      last = i + 1;
    }
  }
  if (last < inner.length()) {
    rawParams.append(inner.mid(last).trimmed());
  }

  QStringList cleanParams;
  for (QString p : rawParams) {
    p = p.trimmed();
    if (p.isEmpty()) continue;

    // 1. 去除默认参数值 (例如: = 18, = QColor("#A855F7"), = nullptr)
    int eqIndex = -1;
    pDepth = 0; aDepth = 0; bDepth = 0;
    for (int i = 0; i < p.length(); ++i) {
      QChar c = p[i];
      if (c == '(') ++pDepth;
      else if (c == ')') --pDepth;
      else if (c == '<') ++aDepth;
      else if (c == '>') --aDepth;
      else if (c == '[') ++bDepth;
      else if (c == ']') --bDepth;
      else if (c == '=' && pDepth == 0 && aDepth == 0 && bDepth == 0) {
        eqIndex = i;
        break;
      }
    }
    if (eqIndex != -1) {
      p = p.left(eqIndex).trimmed();
    }

    if (p == "void" || p == "...") {
      cleanParams.append(p);
      continue;
    }

    // 2. 去除变量名，保留纯类型 (如 "uint8_t *data" -> "uint8_t *", "int ms" -> "int")
    p.replace(QRegularExpression(R"(\s+)"), " ");
    QStringList tokens = p.split(' ', Qt::SkipEmptyParts);
    if (tokens.size() > 1) {
      QString lastTok = tokens.last();
      if (!lastTok.contains('*') && !lastTok.contains('&')) {
        if (lastTok.contains('[')) {
          int bIdx = lastTok.indexOf('[');
          QString arr = lastTok.mid(bIdx);
          tokens.removeLast();
          p = tokens.join(" ") + arr;
        } else {
          tokens.removeLast();
          p = tokens.join(" ");
        }
      } else if (lastTok.startsWith('*') || lastTok.startsWith('&')) {
        QString sym = lastTok.startsWith('*') ? "*" : "&";
        QString varName = lastTok.mid(1).trimmed();
        if (!varName.isEmpty()) {
          tokens.removeLast();
          tokens.append(sym);
          p = tokens.join(" ");
        }
      }
    }

    p = p.trimmed();
    p.replace(" *", " *");
    p.replace(" &", " &");
    p.replace(QRegularExpression(R"(\s+)"), " ");
    cleanParams.append(p);
  }

  return "(" + cleanParams.join(", ") + ")";
}

// 自定义函数与符号大纲树绘制委托 (1:1 严格对齐图 2 与图 3 风格排版与配色)
class SymbolTreeDelegate : public QStyledItemDelegate {
public:
  explicit SymbolTreeDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    bool isSelected = (option.state & QStyle::State_Selected);
    bool isHovered = (option.state & QStyle::State_MouseOver);

    // 1. 绘制项背景
    if (isSelected) {
      painter->setPen(QPen(QColor("#007ACC"), 1));
      painter->setBrush(QColor(0, 122, 204, 50));
      painter->drawRoundedRect(option.rect.adjusted(1, 1, -1, -1), 3, 3);
    } else if (isHovered) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(QColor(255, 255, 255, 12));
      painter->drawRoundedRect(option.rect.adjusted(1, 1, -1, -1), 3, 3);
    }

    // 2. 获取项属性
    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    QString name = index.data(Qt::UserRole + 1).toString();
    QString retType = index.data(Qt::UserRole + 6).toString();
    QString params = index.data(Qt::UserRole + 7).toString();
    int refCount = index.data(Qt::UserRole + 3).toInt();
    int symbolType = index.data(Qt::UserRole + 8).toInt();

    if (name.isEmpty()) {
      name = index.data(Qt::DisplayRole).toString();
    }

    bool hasChildren = index.model()->hasChildren(index);
    int curX = option.rect.left() + 4;
    int centerY = option.rect.top() + option.rect.height() / 2;

    // 3. 绘制展开/折叠矢量 Chevron 箭头 (对齐图 2 样式: 展开为向下箭头 ∨, 折叠为向右箭头 >)
    if (hasChildren) {
      const QTreeView *treeView = qobject_cast<const QTreeView *>(option.widget);
      bool isExpanded = treeView ? treeView->isExpanded(index) : (bool)(option.state & QStyle::State_Open);

      painter->setPen(QPen(isHovered ? QColor("#A855F7") : QColor("#9CA3AF"), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      painter->setBrush(Qt::NoBrush);

      if (isExpanded) {
        // 向下箭头 ∨ (图 2 展开状态)
        QPolygonF poly;
        poly << QPointF(curX + 1, centerY - 2)
             << QPointF(curX + 5.5, centerY + 2.5)
             << QPointF(curX + 10, centerY - 2);
        painter->drawPolyline(poly);
      } else {
        // 向右箭头 > (图 2 折叠状态)
        QPolygonF poly;
        poly << QPointF(curX + 3, centerY - 4.5)
             << QPointF(curX + 7.5, centerY)
             << QPointF(curX + 3, centerY + 4.5);
        painter->drawPolyline(poly);
      }
      curX += 15;
    } else {
      if (!index.parent().isValid()) {
        curX += 15;
      }
    }

    // 4. 绘制符号类型矢量图标
    int iconSize = 16;
    int iconY = option.rect.top() + (option.rect.height() - iconSize) / 2;
    QRect iconRect(curX, iconY, iconSize, iconSize);
    if (!icon.isNull()) {
      icon.paint(painter, iconRect, Qt::AlignCenter);
    }
    curX = iconRect.right() + 6;

    // 5. 绘制右侧调用/引用频次徽标 (如 1, +9, 2)
    int rightLimit = option.rect.right() - 6;
    QFont subFont("Consolas", 9);
    QFontMetrics fmSub(subFont);

    if (refCount > 0) {
      QString countStr = (refCount > 5) ? QString("+%1").arg(refCount) : QString::number(refCount);
      int countW = fmSub.horizontalAdvance(countStr) + 8;
      QRect countRect(rightLimit - countW, option.rect.top(), countW, option.rect.height());
      painter->setFont(subFont);
      painter->setPen(QColor("#E5C07B")); // 暖橙色
      painter->drawText(countRect, Qt::AlignRight | Qt::AlignVCenter, countStr);
      rightLimit -= countW + 4;
    }

    // 6. 绘制符号主体文本: 函数名 (红色/高亮色) + 返回类型 (灰白色) + 参数 (灰色)
    QFont nameFont("Consolas", 9, (symbolType == CodeEditor::SymbolClass || symbolType == CodeEditor::SymbolStruct) ? QFont::Bold : QFont::Normal);
    QFontMetrics fmName(nameFont);

    int textY = option.rect.top();
    int textH = option.rect.height();

    // 绘制名称
    painter->setFont(nameFont);
    QColor nameColor = isSelected ? QColor("#FFFFFF") : QColor("#ECEFF4");
    if (symbolType == CodeEditor::SymbolFunction) {
      nameColor = isSelected ? QColor("#FFAAAA") : QColor("#E06C75"); // 珊瑚红
    } else if (symbolType == CodeEditor::SymbolVariable) {
      nameColor = isSelected ? QColor("#B5E853") : QColor("#E06C75");
    } else if (symbolType == CodeEditor::SymbolClass) {
      nameColor = isSelected ? QColor("#FFE082") : QColor("#EAB308"); // 亮黄
    } else if (symbolType == CodeEditor::SymbolStruct) {
      nameColor = isSelected ? QColor("#A7F3D0") : QColor("#10B981"); // 翠绿
    }
    painter->setPen(nameColor);
    int nameW = fmName.horizontalAdvance(name);
    if (curX + nameW > rightLimit) nameW = qMax(0, rightLimit - curX);
    painter->drawText(QRect(curX, textY, nameW, textH), Qt::AlignLeft | Qt::AlignVCenter, name);
    curX += nameW;

    // 绘制返回类型 (例如 int, void, QString, bool)
    if (!retType.isEmpty() && curX + 10 < rightLimit) {
      curX += 6;
      painter->setFont(subFont);
      painter->setPen(isSelected ? QColor("#D1D5DB") : QColor("#94A3B8"));
      int retW = fmSub.horizontalAdvance(retType);
      if (curX + retW > rightLimit) retW = qMax(0, rightLimit - curX);
      painter->drawText(QRect(curX, textY, retW, textH), Qt::AlignLeft | Qt::AlignVCenter, retType);
      curX += retW;
    }

    // 绘制参数列表 (例如 (const QString &), (void), (int, int))
    if (!params.isEmpty() && curX + 10 < rightLimit) {
      curX += 4;
      painter->setFont(subFont);
      painter->setPen(isSelected ? QColor("#9CA3AF") : QColor("#6B7280"));
      int paramW = fmSub.horizontalAdvance(params);
      if (curX + paramW > rightLimit) paramW = qMax(0, rightLimit - curX);
      painter->drawText(QRect(curX, textY, paramW, textH), Qt::AlignLeft | Qt::AlignVCenter, params);
      curX += paramW;
    }

    painter->restore();
  }

  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
    QString name = index.data(Qt::UserRole + 1).toString();
    if (name.isEmpty()) name = index.data(Qt::DisplayRole).toString();
    QString retType = index.data(Qt::UserRole + 6).toString();
    QString params = index.data(Qt::UserRole + 7).toString();
    int refCount = index.data(Qt::UserRole + 3).toInt();

    QFont nameFont("Consolas", 9, QFont::Bold);
    QFontMetrics fmName(nameFont);
    QFont subFont("Consolas", 9);
    QFontMetrics fmSub(subFont);

    int w = 24 + 18 + 10;
    w += fmName.horizontalAdvance(name);
    if (!retType.isEmpty()) w += 6 + fmSub.horizontalAdvance(retType);
    if (!params.isEmpty()) w += 4 + fmSub.horizontalAdvance(params);
    if (refCount > 0) {
      QString countStr = (refCount > 5) ? QString("+%1").arg(refCount) : QString::number(refCount);
      w += 10 + fmSub.horizontalAdvance(countStr);
    }
    w += 36;
    return QSize(qMax(option.rect.width(), w), 24);
  }
};

// 预处理 C/C++ 源代码：将行注释、块注释、字符串字面量、非 define 预处理指令等宽掩码置空为空格，保留原始换行符、字符偏移与 #define 宏定义
static QString generateCleanCppCode(const QString &code) {
  const int codeLen = code.length();
  QString clean = code;
  bool inBlockComment = false;
  bool inLineComment = false;
  bool inString = false;
  bool inChar = false;
  bool inRawString = false;
  QString rawStringDelimiter;

  for (int i = 0; i < codeLen; ++i) {
    QChar ch = code[i];
    QChar nextCh = (i + 1 < codeLen) ? code[i + 1] : QChar('\0');

    if (inBlockComment) {
      if (ch == '*' && nextCh == '/') {
        clean[i] = ' ';
        clean[i + 1] = ' ';
        inBlockComment = false;
        ++i;
      } else if (ch != '\n') {
        clean[i] = ' ';
      }
      continue;
    }

    if (inLineComment) {
      if (ch == '\n') {
        inLineComment = false;
      } else {
        clean[i] = ' ';
      }
      continue;
    }

    if (inRawString) {
      if (ch == ')' && code.mid(i + 1).startsWith(rawStringDelimiter + "\"")) {
        int endPos = i + 1 + rawStringDelimiter.length() + 1;
        for (int k = i; k < endPos && k < codeLen; ++k) {
          if (clean[k] != '\n') clean[k] = ' ';
        }
        i = endPos - 1;
        inRawString = false;
      } else if (ch != '\n') {
        clean[i] = ' ';
      }
      continue;
    }

    if (inString) {
      if (ch == '\\' && i + 1 < codeLen) {
        clean[i] = ' ';
        if (clean[i + 1] != '\n') clean[i + 1] = ' ';
        ++i;
      } else if (ch == '"') {
        clean[i] = ' ';
        inString = false;
      } else if (ch != '\n') {
        clean[i] = ' ';
      }
      continue;
    }

    if (inChar) {
      if (ch == '\\' && i + 1 < codeLen) {
        clean[i] = ' ';
        if (clean[i + 1] != '\n') clean[i + 1] = ' ';
        ++i;
      } else if (ch == '\'') {
        clean[i] = ' ';
        inChar = false;
      } else if (ch != '\n') {
        clean[i] = ' ';
      }
      continue;
    }

    // 检查是否进入注释或字符串
    if (ch == '/' && nextCh == '*') {
      clean[i] = ' ';
      clean[i + 1] = ' ';
      inBlockComment = true;
      ++i;
    } else if (ch == '/' && nextCh == '/') {
      clean[i] = ' ';
      clean[i + 1] = ' ';
      inLineComment = true;
      ++i;
    } else if (ch == 'R' && nextCh == '"' && i + 2 < codeLen) {
      int openParen = code.indexOf('(', i + 2);
      if (openParen != -1 && openParen - (i + 2) < 16) {
        rawStringDelimiter = code.mid(i + 2, openParen - (i + 2));
        inRawString = true;
        for (int k = i; k <= openParen && k < codeLen; ++k) {
          if (clean[k] != '\n') clean[k] = ' ';
        }
        i = openParen;
      } else {
        clean[i] = ' ';
        clean[i + 1] = ' ';
        inString = true;
        ++i;
      }
    } else if (ch == '"') {
      clean[i] = ' ';
      inString = true;
    } else if (ch == '\'') {
      clean[i] = ' ';
      inChar = true;
    }
  }

  // 仅屏蔽非 define 的预处理指令 (#include, #pragma, #undef, #error 等)，完整保留 #define 宏定义及其名称
  for (int i = 0; i < codeLen; ++i) {
    if (clean[i] == '#') {
      int lineStart = clean.lastIndexOf('\n', i - 1) + 1;
      QString prefix = clean.mid(lineStart, i - lineStart).trimmed();
      if (prefix.isEmpty()) {
        int lineEnd = clean.indexOf('\n', i);
        if (lineEnd == -1) lineEnd = codeLen;
        while (lineEnd > 0 && lineEnd < codeLen && clean[lineEnd - 1] == '\\') {
          lineEnd = clean.indexOf('\n', lineEnd + 1);
          if (lineEnd == -1) { lineEnd = codeLen; break; }
        }
        QString dirLine = clean.mid(i, lineEnd - i);
        // 如果是 #define，保留 "#define MACRO_NAME"，仅将后面的展开表达式主体安全置为空格
        QRegularExpression defRe(R"(^#[ \t]*define[ \t]+([A-Za-z_]\w*)(?:\(([^)]*)\))?)");
        QRegularExpressionMatch defMatch = defRe.match(dirLine);
        if (defMatch.hasMatch()) {
          int keepEnd = i + defMatch.capturedEnd();
          for (int k = keepEnd; k < lineEnd; ++k) {
            if (clean[k] != '\n') clean[k] = ' ';
          }
        } else {
          // 其他预处理指令全部置为空格
          for (int k = i; k < lineEnd; ++k) {
            if (clean[k] != '\n') clean[k] = ' ';
          }
        }
        i = lineEnd - 1;
      }
    }
  }

  return clean;
}

// 现代 VS Code 风格右上角浮动查找与替换栏
class FindReplaceWidget : public QWidget {
public:
  explicit FindReplaceWidget(CodeEditor *editor, QWidget *parent = nullptr)
      : QWidget(parent), m_editor(editor) {
    setObjectName("FindReplaceWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(4);

    // 第 1 行：查找输入行
    QHBoxLayout *findRow = new QHBoxLayout();
    findRow->setContentsMargins(0, 0, 0, 0);
    findRow->setSpacing(4);

    m_toggleReplaceBtn = new QToolButton(this);
    m_toggleReplaceBtn->setText("▼");
    m_toggleReplaceBtn->setToolTip("展开/折叠替换 (Ctrl+H)");
    m_toggleReplaceBtn->setCheckable(true);
    m_toggleReplaceBtn->setFixedSize(20, 24);
    findRow->addWidget(m_toggleReplaceBtn);

    m_findEdit = new QLineEdit(this);
    m_findEdit->setPlaceholderText("查找 (Ctrl+F)");
    m_findEdit->setMinimumWidth(180);
    m_findEdit->setFixedHeight(24);
    findRow->addWidget(m_findEdit, 1);

    m_matchCaseBtn = new QToolButton(this);
    m_matchCaseBtn->setText("Aa");
    m_matchCaseBtn->setToolTip("区分大小写 (Alt+C)");
    m_matchCaseBtn->setCheckable(true);
    m_matchCaseBtn->setFixedSize(24, 24);
    findRow->addWidget(m_matchCaseBtn);

    m_matchWordBtn = new QToolButton(this);
    m_matchWordBtn->setText("\\b");
    m_matchWordBtn->setToolTip("全词匹配 (Alt+W)");
    m_matchWordBtn->setCheckable(true);
    m_matchWordBtn->setFixedSize(24, 24);
    findRow->addWidget(m_matchWordBtn);

    m_regexBtn = new QToolButton(this);
    m_regexBtn->setText(".*");
    m_regexBtn->setToolTip("使用正则表达式 (Alt+R)");
    m_regexBtn->setCheckable(true);
    m_regexBtn->setFixedSize(24, 24);
    findRow->addWidget(m_regexBtn);

    m_matchCountLabel = new QLabel(this);
    m_matchCountLabel->setText("无结果");
    m_matchCountLabel->setStyleSheet("color: #9CA3AF; font-size: 11px; padding: 0 4px;");
    findRow->addWidget(m_matchCountLabel);

    m_prevBtn = new QToolButton(this);
    m_prevBtn->setText("▲");
    m_prevBtn->setToolTip("上一个 (Shift+Enter / Shift+F3)");
    m_prevBtn->setFixedSize(24, 24);
    findRow->addWidget(m_prevBtn);

    m_nextBtn = new QToolButton(this);
    m_nextBtn->setText("▼");
    m_nextBtn->setToolTip("下一个 (Enter / F3)");
    m_nextBtn->setFixedSize(24, 24);
    findRow->addWidget(m_nextBtn);

    m_closeBtn = new QToolButton(this);
    m_closeBtn->setText("✕");
    m_closeBtn->setToolTip("关闭 (Esc)");
    m_closeBtn->setFixedSize(20, 24);
    findRow->addWidget(m_closeBtn);

    mainLayout->addLayout(findRow);

    // 第 2 行：替换输入行
    m_replaceRowWidget = new QWidget(this);
    QHBoxLayout *replaceRow = new QHBoxLayout(m_replaceRowWidget);
    replaceRow->setContentsMargins(0, 0, 0, 0);
    replaceRow->setSpacing(4);

    QLabel *spacer = new QLabel(this);
    spacer->setFixedWidth(20);
    replaceRow->addWidget(spacer);

    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText("替换为...");
    m_replaceEdit->setMinimumWidth(180);
    m_replaceEdit->setFixedHeight(24);
    replaceRow->addWidget(m_replaceEdit, 1);

    m_replaceBtn = new QToolButton(this);
    m_replaceBtn->setText("替换");
    m_replaceBtn->setToolTip("替换当前匹配项");
    m_replaceBtn->setFixedHeight(24);
    replaceRow->addWidget(m_replaceBtn);

    m_replaceAllBtn = new QToolButton(this);
    m_replaceAllBtn->setText("全部替换");
    m_replaceAllBtn->setToolTip("替换全部匹配项");
    m_replaceAllBtn->setFixedHeight(24);
    replaceRow->addWidget(m_replaceAllBtn);

    mainLayout->addWidget(m_replaceRowWidget);
    m_replaceRowWidget->setVisible(false);

    // 统一样式
    setStyleSheet(
        "#FindReplaceWidget {"
        "  background-color: #252526;"
        "  border: 1px solid #3E4451;"
        "  border-radius: 6px;"
        "}"
        "QLineEdit {"
        "  background-color: #1E1E1E;"
        "  color: #D4D4D4;"
        "  border: 1px solid #3E4451;"
        "  border-radius: 3px;"
        "  padding: 2px 6px;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #007ACC;"
        "}"
        "QToolButton {"
        "  background-color: transparent;"
        "  color: #CCCCCC;"
        "  border: 1px solid transparent;"
        "  border-radius: 3px;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "  padding: 1px 4px;"
        "}"
        "QToolButton:hover {"
        "  background-color: #383B40;"
        "  border: 1px solid #4B5263;"
        "}"
        "QToolButton:checked {"
        "  background-color: #0E639C;"
        "  color: #FFFFFF;"
        "  border: 1px solid #1177BB;"
        "}"
        "QToolButton:pressed {"
        "  background-color: #007ACC;"
        "}");

    // 信号连接
    connect(m_toggleReplaceBtn, &QToolButton::toggled, this, &FindReplaceWidget::setReplaceVisible);
    connect(m_findEdit, &QLineEdit::textChanged, this, &FindReplaceWidget::onSearchTextChanged);
    connect(m_findEdit, &QLineEdit::returnPressed, this, [this]() { findText(true); });
    connect(m_nextBtn, &QToolButton::clicked, this, [this]() { findText(true); });
    connect(m_prevBtn, &QToolButton::clicked, this, [this]() { findText(false); });
    connect(m_closeBtn, &QToolButton::clicked, this, &FindReplaceWidget::hide);
    connect(m_matchCaseBtn, &QToolButton::toggled, this, [this]() { onSearchTextChanged(m_findEdit->text()); });
    connect(m_matchWordBtn, &QToolButton::toggled, this, [this]() { onSearchTextChanged(m_findEdit->text()); });
    connect(m_regexBtn, &QToolButton::toggled, this, [this]() { onSearchTextChanged(m_findEdit->text()); });
    connect(m_replaceBtn, &QToolButton::clicked, this, &FindReplaceWidget::replaceSingle);
    connect(m_replaceAllBtn, &QToolButton::clicked, this, &FindReplaceWidget::replaceAll);
    connect(m_replaceEdit, &QLineEdit::returnPressed, this, &FindReplaceWidget::replaceSingle);
  }

  void setReplaceVisible(bool visible) {
    m_toggleReplaceBtn->setChecked(visible);
    m_toggleReplaceBtn->setText(visible ? "▲" : "▼");
    m_replaceRowWidget->setVisible(visible);
    adjustSize();
    updatePosition();
  }

  void showFind(const QString &prefill = QString()) {
    if (!prefill.isEmpty()) {
      m_findEdit->setText(prefill);
      m_findEdit->selectAll();
    }
    show();
    raise();
    updatePosition();
    m_findEdit->setFocus();
    onSearchTextChanged(m_findEdit->text());
  }

  void showReplace(const QString &prefill = QString()) {
    setReplaceVisible(true);
    showFind(prefill);
    if (!prefill.isEmpty()) {
      m_replaceEdit->setFocus();
      m_replaceEdit->selectAll();
    }
  }

  void updatePosition() {
    if (!m_editor) return;
    int rightMargin = 25;
    if (m_editor->m_functionListContainer && m_editor->m_functionListContainer->isVisible()) {
      rightMargin += m_editor->m_functionListContainer->width();
    }
    int x = m_editor->width() - width() - rightMargin;
    int y = (m_editor->m_breadcrumbBar && m_editor->m_breadcrumbBar->isVisible()) ?
            (m_editor->m_breadcrumbBar->geometry().bottom() + 4) : 4;
    if (x < 10) x = 10;
    move(x, y);
  }

  bool findText(bool forward) {
    if (!m_editor || !m_editor->currentEditor()) return false;
    QsciScintilla *curEditor = m_editor->currentEditor();
    QString query = m_findEdit->text();
    if (query.isEmpty()) return false;

    bool cs = m_matchCaseBtn->isChecked();
    bool wo = m_matchWordBtn->isChecked();
    bool re = m_regexBtn->isChecked();

    bool found = curEditor->findFirst(query, re, cs, wo, true, forward);
    if (!found) {
      found = curEditor->findFirst(query, re, cs, wo, true, forward, 0, 0);
    }
    updateMatchCount();
    return found;
  }

  void replaceSingle() {
    if (!m_editor || !m_editor->currentEditor()) return;
    QsciScintilla *curEditor = m_editor->currentEditor();
    if (curEditor->hasSelectedText()) {
      curEditor->replace(m_replaceEdit->text());
    }
    findText(true);
  }

  void replaceAll() {
    if (!m_editor || !m_editor->currentEditor()) return;
    QsciScintilla *curEditor = m_editor->currentEditor();
    QString query = m_findEdit->text();
    QString rep = m_replaceEdit->text();
    if (query.isEmpty()) return;

    curEditor->SendScintilla(QsciScintilla::SCI_BEGINUNDOACTION);
    bool cs = m_matchCaseBtn->isChecked();
    bool wo = m_matchWordBtn->isChecked();
    bool re = m_regexBtn->isChecked();

    if (curEditor->findFirst(query, re, cs, wo, false, true, 0, 0)) {
      do {
        curEditor->replace(rep);
      } while (curEditor->findNext());
    }
    curEditor->SendScintilla(QsciScintilla::SCI_ENDUNDOACTION);
    updateMatchCount();
  }

  void onSearchTextChanged(const QString &text) {
    if (text.isEmpty()) {
      m_matchCountLabel->setText("无结果");
      return;
    }
    updateMatchCount();
  }

  void updateMatchCount() {
    if (!m_editor || !m_editor->currentEditor()) return;
    QString query = m_findEdit->text();
    if (query.isEmpty()) {
      m_matchCountLabel->setText("无结果");
      return;
    }

    QString code = m_editor->currentEditor()->text();
    QRegularExpression::PatternOptions opt = QRegularExpression::NoPatternOption;
    if (!m_matchCaseBtn->isChecked()) {
      opt |= QRegularExpression::CaseInsensitiveOption;
    }
    QString pattern = m_regexBtn->isChecked() ? query : QRegularExpression::escape(query);
    if (m_matchWordBtn->isChecked()) {
      pattern = QString(R"(\b%1\b)").arg(pattern);
    }
    QRegularExpression regex(pattern, opt);
    if (!regex.isValid()) {
      m_matchCountLabel->setText("正则错误");
      return;
    }

    int total = 0;
    QRegularExpressionMatchIterator it = regex.globalMatch(code);
    while (it.hasNext()) {
      it.next();
      ++total;
    }

    if (total == 0) {
      m_matchCountLabel->setText("无结果");
    } else {
      m_matchCountLabel->setText(QString("%1 处匹配").arg(total));
    }
  }

protected:
  void keyPressEvent(QKeyEvent *e) override {
    if (e->key() == Qt::Key_Escape) {
      hide();
      if (m_editor && m_editor->currentEditor()) {
        m_editor->currentEditor()->setFocus();
      }
      e->accept();
      return;
    }
    QWidget::keyPressEvent(e);
  }

private:
  CodeEditor *m_editor;
  QLineEdit *m_findEdit;
  QLineEdit *m_replaceEdit;
  QLabel *m_matchCountLabel;
  QToolButton *m_toggleReplaceBtn;
  QToolButton *m_matchCaseBtn;
  QToolButton *m_matchWordBtn;
  QToolButton *m_regexBtn;
  QToolButton *m_prevBtn;
  QToolButton *m_nextBtn;
  QToolButton *m_closeBtn;
  QToolButton *m_replaceBtn;
  QToolButton *m_replaceAllBtn;
  QWidget *m_replaceRowWidget;
};

using DefinitionCandidate = CodeEditor::DefinitionCandidate;

// 转到定义多候选跳转对话框
class GoToDefinitionDialog : public QDialog {
public:
  GoToDefinitionDialog(const QString &symbolName, const QList<CodeEditor::DefinitionCandidate> &candidates,
                       CodeEditor *editor, QWidget *parent = nullptr)
      : QDialog(parent), m_editor(editor), m_candidates(candidates) {
    setWindowTitle(QString("转到定义: %1 (共发现 %2 处候选定义)").arg(symbolName).arg(candidates.size()));
    resize(760, 380);
    setStyleSheet(
        "QDialog {"
        "  background-color: #1E1E2E;"
        "  color: #D4D4D4;"
        "}"
        "QListWidget {"
        "  background-color: #181825;"
        "  border: 1px solid #313244;"
        "  border-radius: 6px;"
        "  color: #CDD6F4;"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "  font-size: 13px;"
        "  padding: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 8px 10px;"
        "  border-bottom: 1px solid #282A3A;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #313244;"
        "  color: #89B4FA;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #45475A;"
        "  color: #89B4FA;"
        "}");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    QLabel *headerLabel = new QLabel(QString("符号 <b><font color='#89B4FA'>%1</font></b> 存在多处候选定义与声明（双击或选中回车直接跳转）：").arg(symbolName), this);
    headerLabel->setStyleSheet("color: #BAC2DE; font-size: 13px; font-weight: 500;");
    layout->addWidget(headerLabel);

    m_listWidget = new QListWidget(this);
    for (int i = 0; i < candidates.size(); ++i) {
      const auto &cand = candidates[i];
      QString fileName = QFileInfo(cand.filePath).fileName();
      QString itemText = QString("[%1]  %2  (第 %3 行)\n    %4")
                             .arg(cand.type, -10)
                             .arg(fileName)
                             .arg(cand.line + 1)
                             .arg(cand.preview.trimmed());
      QListWidgetItem *item = new QListWidgetItem(itemText, m_listWidget);
      item->setData(Qt::UserRole, i);
      if (i == 0) m_listWidget->setCurrentItem(item);
    }
    layout->addWidget(m_listWidget);

    auto triggerJump = [this]() {
      QListWidgetItem *item = m_listWidget->currentItem();
      if (item && m_editor) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < m_candidates.size()) {
          m_editor->openDefinitionCandidate(m_candidates[idx]);
        }
      }
      accept();
    };

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, triggerJump);
    connect(m_listWidget, &QListWidget::itemActivated, this, triggerJump);
  }

protected:
  void keyPressEvent(QKeyEvent *event) override {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
      QListWidgetItem *item = m_listWidget->currentItem();
      if (item && m_editor) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < m_candidates.size()) {
          m_editor->openDefinitionCandidate(m_candidates[idx]);
        }
      }
      accept();
      return;
    }
    QDialog::keyPressEvent(event);
  }

private:
  CodeEditor *m_editor;
  QListWidget *m_listWidget;
  QList<CodeEditor::DefinitionCandidate> m_candidates;
};

// 局部变量重命名对话框
class RenameSymbolDialog : public QDialog {
public:
  RenameSymbolDialog(const QString &oldName, const QString &scopeDesc, int count, QWidget *parent = nullptr)
      : QDialog(parent) {
    setWindowTitle("重命名符号 (Rename Symbol)");
    setMinimumWidth(380);
    setStyleSheet(
        "QDialog {"
        "  background-color: #252526;"
        "  color: #D4D4D4;"
        "}"
        "QLabel {"
        "  color: #D4D4D4;"
        "  font-size: 12px;"
        "}"
        "QLineEdit {"
        "  background-color: #1E1E1E;"
        "  color: #FFFFFF;"
        "  border: 1px solid #007ACC;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 13px;"
        "}"
        "QPushButton {"
        "  background-color: #0E639C;"
        "  color: #FFFFFF;"
        "  border: 1px solid #1177BB;"
        "  border-radius: 4px;"
        "  padding: 4px 14px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1177BB;"
        "}"
        "QPushButton#cancelBtn {"
        "  background-color: #383B40;"
        "  border: 1px solid #4B5263;"
        "}"
        "QPushButton#cancelBtn:hover {"
        "  background-color: #4B5263;"
        "}");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(16, 16, 16, 16);

    QLabel *infoLabel = new QLabel(QString("<b>作用域：</b>%1<br><b>引用计数：</b>共发现 <span style='color:#EAB308;'>%2</span> 处引用").arg(scopeDesc).arg(count), this);
    infoLabel->setTextFormat(Qt::RichText);
    layout->addWidget(infoLabel);

    QLabel *nameLabel = new QLabel("请输入新名称：", this);
    layout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setText(oldName);
    m_nameEdit->selectAll();
    layout->addWidget(m_nameEdit);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *okBtn = new QPushButton("执行重命名 (Enter)", this);
    okBtn->setDefault(true);
    QPushButton *cancelBtn = new QPushButton("取消", this);
    cancelBtn->setObjectName("cancelBtn");

    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
  }

  QString newName() const { return m_nameEdit->text().trimmed(); }

private:
  QLineEdit *m_nameEdit;
};

// 全工程符号重命名对话框
class ProjectRenameDialog : public QDialog {
public:
  ProjectRenameDialog(const QString &oldName, const QString &scopeDesc,
                      const QMap<QString, QList<CodeEditor::SymbolOccurrence>> &fileOccurrences,
                      QWidget *parent = nullptr)
      : QDialog(parent) {
    setWindowTitle("全工程重命名符号 (Project-Wide Rename)");
    setMinimumWidth(560);
    setStyleSheet(
        "QDialog {"
        "  background-color: #252526;"
        "  color: #D4D4D4;"
        "}"
        "QLabel {"
        "  color: #D4D4D4;"
        "  font-size: 12px;"
        "}"
        "QLineEdit {"
        "  background-color: #1E1E1E;"
        "  color: #FFFFFF;"
        "  border: 1px solid #007ACC;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 13px;"
        "}"
        "QListWidget {"
        "  background-color: #1E1E1E;"
        "  border: 1px solid #3E4451;"
        "  color: #D4D4D4;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 11px;"
        "}"
        "QPushButton {"
        "  background-color: #0E639C;"
        "  color: #FFFFFF;"
        "  border: 1px solid #1177BB;"
        "  border-radius: 4px;"
        "  padding: 5px 16px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1177BB;"
        "}"
        "QPushButton#cancelBtn {"
        "  background-color: #383B40;"
        "  border: 1px solid #4B5263;"
        "}"
        "QPushButton#cancelBtn:hover {"
        "  background-color: #4B5263;"
        "}");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(16, 16, 16, 16);

    int totalOccs = 0;
    for (auto it = fileOccurrences.begin(); it != fileOccurrences.end(); ++it) {
      totalOccs += it.value().size();
    }

    QLabel *infoLabel = new QLabel(
        QString("<b>作用域：</b>%1<br><b>全工程统计：</b>将在 <b>%2</b> 个源文件中重命名，共 <b>%3</b> 处引用")
            .arg(scopeDesc).arg(fileOccurrences.size()).arg(totalOccs), this);
    infoLabel->setTextFormat(Qt::RichText);
    layout->addWidget(infoLabel);

    QLabel *fileListLabel = new QLabel("涉及文件清单：", this);
    layout->addWidget(fileListLabel);

    QListWidget *fileListWidget = new QListWidget(this);
    fileListWidget->setMaximumHeight(130);
    for (auto it = fileOccurrences.begin(); it != fileOccurrences.end(); ++it) {
      QString fName = QFileInfo(it.key()).fileName();
      fileListWidget->addItem(QString("📄 %1  (%2 处引用) - %3").arg(fName).arg(it.value().size()).arg(it.key()));
    }
    layout->addWidget(fileListWidget);

    QLabel *nameLabel = new QLabel("请输入新符号名称：", this);
    layout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setText(oldName);
    m_nameEdit->selectAll();
    layout->addWidget(m_nameEdit);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *okBtn = new QPushButton("执行全工程重命名 (Enter)", this);
    okBtn->setDefault(true);
    QPushButton *cancelBtn = new QPushButton("取消", this);
    cancelBtn->setObjectName("cancelBtn");

    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
  }

  QString newName() const { return m_nameEdit->text().trimmed(); }

private:
  QLineEdit *m_nameEdit;
};

// 查找所有引用展示弹窗 (References Dialog)
class ReferencesDialog : public QDialog {
public:
  ReferencesDialog(const QString &symbolName, const QList<CodeEditor::SymbolOccurrence> &occurrences,
                   const QString &filePath, CodeEditor *editor, QWidget *parent = nullptr)
      : QDialog(parent), m_editor(editor), m_occurrences(occurrences), m_filePath(filePath) {
    setWindowTitle(QString("查找引用: %1 (共 %2 处)").arg(symbolName).arg(occurrences.size()));
    resize(660, 380);
    setStyleSheet(
        "QDialog {"
        "  background-color: #1E1E2E;"
        "  color: #D4D4D4;"
        "}"
        "QListWidget {"
        "  background-color: #181825;"
        "  border: 1px solid #313244;"
        "  color: #CDD6F4;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 8px;"
        "  border-bottom: 1px solid #313244;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #313244;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #45475A;"
        "  color: #89B4FA;"
        "}");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QLabel *headerLabel = new QLabel(QString("符号 <b>%1</b> 的所有引用列表（双击可直接跳转）：").arg(symbolName), this);
    headerLabel->setStyleSheet("color: #BAC2DE; font-size: 13px;");
    layout->addWidget(headerLabel);

    m_listWidget = new QListWidget(this);
    for (int i = 0; i < occurrences.size(); ++i) {
      const auto &occ = occurrences[i];
      QString itemText = QString("第 %1 行 [列 %2]:  %3").arg(occ.line + 1, 4).arg(occ.col + 1, 3).arg(occ.lineContent.trimmed());
      QListWidgetItem *item = new QListWidgetItem(itemText, m_listWidget);
      item->setData(Qt::UserRole, occ.line);
      item->setData(Qt::UserRole + 1, occ.col);
      item->setData(Qt::UserRole + 2, occ.length);
    }
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
      if (item && m_editor && m_editor->currentEditor()) {
        int line = item->data(Qt::UserRole).toInt();
        int col = item->data(Qt::UserRole + 1).toInt();
        int len = item->data(Qt::UserRole + 2).toInt();
        m_editor->currentEditor()->setCursorPosition(line, col);
        m_editor->currentEditor()->ensureLineVisible(line);
        m_editor->currentEditor()->setSelection(line, col, line, col + len);
        m_editor->currentEditor()->setFocus();
      }
      accept();
    });
  }

private:
  CodeEditor *m_editor;
  QListWidget *m_listWidget;
  QList<CodeEditor::SymbolOccurrence> m_occurrences;
  QString m_filePath;
};

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

CodeEditor::CodeEditor(QWidget *parent)
    : QWidget(parent), m_currentEditor(nullptr), m_apiCPP(nullptr),
      m_breadcrumbBar(nullptr), m_bcPathContainer(nullptr), m_bcPathLayout(nullptr),
      m_bcFileButton(nullptr), m_bcFuncButton(nullptr), m_outlineToggleBtn(nullptr),
      m_functionListContainer(nullptr), m_outlineTitleLabel(nullptr),
      m_functionTree(nullptr), m_findReplaceBar(nullptr), m_stickyScrollWidget(nullptr), m_isDarkTheme(false)
{
  // 创建主布局
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  // 创建工具栏
  createToolBar();
  layout->addWidget(m_toolBar);

  // 创建 VS Code 风格面包屑导航栏
  createBreadcrumbBar();
  layout->addWidget(m_breadcrumbBar);

  // 创建主分割器
  m_mainSplitter = new QSplitter(Qt::Horizontal, this);
  layout->addWidget(m_mainSplitter);

  // 创建C++词法分析器
  m_lexerCPP = new QsciLexerCPP();

  // 创建第一个编辑器（主编辑区域位于左侧/中央）
  QsciScintilla *editor = new QsciScintilla(this);
  m_editors.append(editor);
  m_currentEditor = editor;
  m_mainSplitter->addWidget(editor);

  // 创建函数大纲列表控件（位于右侧侧边栏，默认不显示）
  createFunctionList();

  // 设置编辑器属性
  setupEditor(editor);

  // 设置自动补全
  setupAutoCompletion(editor);

  // 设置初始分割器比例 (主编辑器占比 100%，大纲默认隐藏)
  m_mainSplitter->setStretchFactor(0, 1);
  m_mainSplitter->setStretchFactor(1, 0);
  m_mainSplitter->setCollapsible(0, false);
  m_mainSplitter->setCollapsible(1, true);

  // 浮动查找与替换栏
  m_findReplaceBar = new FindReplaceWidget(this, this);
  m_findReplaceBar->hide();

  // 粘性滚动 (Sticky Scroll) / 顶部悬挂函数头控件
  m_stickyScrollWidget = new StickyScrollWidget(this, m_currentEditor);
  m_stickyScrollWidget->hide();

  // 符号同名高亮防抖定时器
  m_occurrenceTimer = new QTimer(this);
  m_occurrenceTimer->setSingleShot(true);
  m_occurrenceTimer->setInterval(150);
  connect(m_occurrenceTimer, &QTimer::timeout, this, &CodeEditor::onOccurrenceTimerTimeout);

  // 设置示例代码
  editor->setText(
      "// PhudonTools 代码编辑器\n#include <stdint.h>\n\nint main(void) {\n    // "
      "初始化代码\n    while(1) {\n        // 主循环\n    }\n    return 0;\n}");

  // 连接信号和槽
  connect(editor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateVariableList);
  connect(editor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateFunctionList);

  // 初始化函数列表
  updateFunctionList();

  // 读取系统上次保存的主题或默认使用深色暗夜主题
  QSettings settings("PhudonTools", "Settings");
  QString currentTheme = settings.value("theme", "dark").toString();
  applyTheme(currentTheme);
}

CodeEditor::~CodeEditor() {
  if (m_apiCPP) {
    delete m_apiCPP;
    m_apiCPP = nullptr;
  }

  if (m_lexerCPP) {
    delete m_lexerCPP;
    m_lexerCPP = nullptr;
  }
}

QsciScintilla *CodeEditor::currentEditor() const { return m_currentEditor; }

QList<QsciScintilla *> CodeEditor::allEditors() const { return m_editors; }

void CodeEditor::setText(const QString &text) {
  if (m_currentEditor) {
    m_currentEditor->setText(text);
  }
}

QString CodeEditor::text() const {
  if (m_currentEditor) {
    return m_currentEditor->text();
  }
  return QString();
}

bool CodeEditor::openFile(const QString &filePath) {
  QFile file(filePath);
  if (file.open(QIODevice::ReadOnly)) {
    QByteArray data = file.readAll();
    file.close();

    QString content;
    QTextCodec::ConverterState state;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QString utf8Content =
        codec->toUnicode(data.constData(), data.size(), &state);

    if (state.invalidChars > 0) {
      // Contains invalid UTF-8 chars, assume local encoding (e.g. GBK)
      content = QTextCodec::codecForLocale()->toUnicode(data);
    } else {
      content = utf8Content;
    }

    // 更新当前规范化绝对文件路径
    m_currentFilePath = QFileInfo(filePath).canonicalFilePath();
    if (m_currentFilePath.isEmpty()) {
      m_currentFilePath = QFileInfo(filePath).absoluteFilePath();
    }

    // 在当前编辑器中显示文件内容
    if (m_currentEditor) {
      m_currentEditor->setText(content);
      // Trigger highlighting manually after loading file
      updateBracketHighlighting(m_currentEditor);
      highlightFunctionNames(m_currentEditor, 20); // 20 is FUNCTION_INDICATOR
    }

    // 解析文件中的变量并更新自动补全
    updateVariableList();

    // 更新函数列表
    updateFunctionList();

    emit fileOpened(m_currentFilePath);
    return true;
  }
  return false;
}

bool CodeEditor::saveFile(const QString &filePath) {
  if (!m_currentEditor) {
    return false;
  }

  QFile file(filePath);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << m_currentEditor->text();
    file.close();

    // 更新当前文件路径
    m_currentFilePath = QFileInfo(filePath).canonicalFilePath();
    if (m_currentFilePath.isEmpty()) {
      m_currentFilePath = QFileInfo(filePath).absoluteFilePath();
    }

    emit fileSaved(m_currentFilePath);
    return true;
  }
  return false;
}

// void CodeEditor::applyTheme(const QString &themeName)
// {
//     if (themeName == "dark") {
//         // 深色主题
//         for (QsciScintilla* editor : m_editors) {
//             // 设置编辑器背景色和默认文本颜色
//             editor->setColor(QColor("#DCDCDC"));
//             editor->setPaper(QColor("#1E1E1E"));

//             // 设置行号边距颜色
//             editor->setMarginsBackgroundColor(QColor("#1E1E1E"));
//             editor->setMarginsForegroundColor(QColor("#858585"));

//             // 设置折叠边距颜色
//             editor->setFoldMarginColors(QColor("#1E1E1E"),
//             QColor("#1E1E1E"));

//             // 设置选中文本的颜色
//             editor->setSelectionBackgroundColor(QColor("#264F78"));
//             editor->setSelectionForegroundColor(QColor("#FFFFFF"));
//         }

//         // 设置语法高亮颜色
//         m_lexerCPP->setColor(QColor("#569CD6"), QsciLexerCPP::Keyword); //
//         关键字 m_lexerCPP->setColor(QColor("#CE9178"),
//         QsciLexerCPP::DoubleQuotedString); // 字符串
//         m_lexerCPP->setColor(QColor("#CE9178"),
//         QsciLexerCPP::SingleQuotedString); // 字符
//         m_lexerCPP->setColor(QColor("#B5CEA8"), QsciLexerCPP::Number); //
//         数字 m_lexerCPP->setColor(QColor("#608B4E"), QsciLexerCPP::Comment);
//         // 注释 m_lexerCPP->setColor(QColor("#608B4E"),
//         QsciLexerCPP::CommentLine); // 行注释
//         m_lexerCPP->setColor(QColor("#C586C0"), QsciLexerCPP::PreProcessor);
//         // 预处理器 m_lexerCPP->setColor(QColor("#4EC9B0"),
//         QsciLexerCPP::GlobalClass); // 类名

//         // 设置背景色
//         m_lexerCPP->setPaper(QColor("#1E1E1E"));

//         // 设置默认字体
//         QFont font("Consolas", 10);
//         m_lexerCPP->setFont(font);
//     } else if (themeName == "light") {
//         // 浅色主题
//         for (QsciScintilla* editor : m_editors) {
//             // 设置编辑器背景色和默认文本颜色
//             editor->setColor(QColor("#000000"));
//             editor->setPaper(QColor("#FFFFFF"));

//             // 设置行号边距颜色
//             editor->setMarginsBackgroundColor(QColor("#F0F0F0"));
//             editor->setMarginsForegroundColor(QColor("#2B91AF"));

//             // 设置折叠边距颜色
//             editor->setFoldMarginColors(QColor("#F0F0F0"),
//             QColor("#F0F0F0"));

//             // 设置选中文本的颜色
//             editor->setSelectionBackgroundColor(QColor("#ADD6FF"));
//             editor->setSelectionForegroundColor(QColor("#000000"));
//         }

//         // 设置语法高亮颜色
//         m_lexerCPP->setColor(QColor("#0000FF"), QsciLexerCPP::Keyword); //
//         关键字 m_lexerCPP->setColor(QColor("#A31515"),
//         QsciLexerCPP::DoubleQuotedString); // 字符串
//         m_lexerCPP->setColor(QColor("#A31515"),
//         QsciLexerCPP::SingleQuotedString); // 字符
//         m_lexerCPP->setColor(QColor("#098658"), QsciLexerCPP::Number); //
//         数字 m_lexerCPP->setColor(QColor("#008000"), QsciLexerCPP::Comment);
//         // 注释 m_lexerCPP->setColor(QColor("#008000"),
//         QsciLexerCPP::CommentLine); // 行注释
//         m_lexerCPP->setColor(QColor("#800000"), QsciLexerCPP::PreProcessor);
//         // 预处理器 m_lexerCPP->setColor(QColor("#267F99"),
//         QsciLexerCPP::GlobalClass); // 类名

//         // 设置背景色
//         m_lexerCPP->setPaper(QColor("#FFFFFF"));

//         // 设置默认字体
//         QFont font("Consolas", 10);
//         m_lexerCPP->setFont(font);
//     }
//     // 可以添加更多主题...
// }

void CodeEditor::applyTheme(const QString &themeName) {
  const IdeTheme::ThemePalette p = IdeTheme::paletteFor(themeName);
  m_isDarkTheme = p.isDark;

  // 1. 设置编辑器 QsciScintilla 核心配色 (消除纯白背景与硬编码边距色)
  for (QsciScintilla *editor : m_editors) {
    if (!editor)
      continue;

    editor->setColor(QColor(p.textMain));
    editor->setPaper(QColor(p.editorBg));

    // 行号边距颜色
    editor->setMarginsBackgroundColor(QColor(p.marginBg));
    editor->setMarginsForegroundColor(QColor(p.marginFg));

    // 折叠边距颜色
    editor->setFoldMarginColors(QColor(p.foldMarginBg), QColor(p.foldMarginBg));

    // 选中文本高亮颜色
    editor->setSelectionBackgroundColor(QColor(p.selectionBg));
    editor->setSelectionForegroundColor(QColor(p.selectionText));

    // 光标与活动行高亮
    editor->setCaretForegroundColor(p.caretColor);
    editor->setCaretLineVisible(true);
    editor->setCaretLineBackgroundColor(p.caretLineBg);

    // 设置活动行号样式
    editor->SendScintilla(QsciScintilla::SCI_STYLESETFORE, 34,
                          (p.accent.startsWith('#') ? QColor(p.accent).rgb() & 0xFFFFFF : 0x00AA00));
    editor->SendScintilla(QsciScintilla::SCI_STYLESETBACK, 34,
                          p.caretLineBg.rgb() & 0xFFFFFF);
    editor->SendScintilla(QsciScintilla::SCI_STYLESETFONT, 34, "Consolas");
    editor->SendScintilla(QsciScintilla::SCI_STYLESETSIZE, 34, 10);
    editor->SendScintilla(QsciScintilla::SCI_STYLESETBOLD, 34, 1);

    // 设置彩虹括号
    setupRainbowBrackets(editor);

    // 设置符号同名/引用高亮指示器 (OCCURRENCE_INDICATOR = 28)
    editor->indicatorDefine(QsciScintilla::RoundBoxIndicator, OCCURRENCE_INDICATOR);
    editor->setIndicatorForegroundColor(QColor(p.isDark ? "#89B4FA" : "#2563EB"), OCCURRENCE_INDICATOR);
    editor->setIndicatorDrawUnder(false, OCCURRENCE_INDICATOR);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, (unsigned long)OCCURRENCE_INDICATOR, (long)(p.isDark ? 75 : 70));
    editor->SendScintilla(QsciScintilla::SCI_INDICSETOUTLINEALPHA, (unsigned long)OCCURRENCE_INDICATOR, (long)255);
  }

  // 2. 深度定制面包屑导航栏与函数大纲列表
  if (m_breadcrumbBar) {
    m_breadcrumbBar->setStyleSheet(QString(
        "#breadcrumbBar {"
        "    background-color: %1;"
        "    border-bottom: 1px solid %2;"
        "    padding: 0 8px;"
        "}"
        "#breadcrumbBar QLabel {"
        "    color: %3;"
        "    font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "    font-size: 12px;"
        "}"
        "#breadcrumbBar QToolButton {"
        "    background: transparent;"
        "    border: 1px solid transparent;"
        "    border-radius: 4px;"
        "    color: %4;"
        "    font-family: 'Segoe UI', 'Microsoft YaHei', 'Consolas', sans-serif;"
        "    font-size: 12px;"
        "    padding: 2px 8px;"
        "}"
        "#breadcrumbBar QToolButton:hover {"
        "    background-color: %5;"
        "    border: 1px solid %2;"
        "}"
        "#breadcrumbBar QToolButton:checked {"
        "    background-color: rgba(168, 85, 247, 0.22);"
        "    border: 1px solid %6;"
        "    color: %6;"
        "    font-weight: 600;"
        "}"
        "#breadcrumbBar QToolButton:pressed {"
        "    background-color: %7;"
        "}").arg(p.panelBg, p.border, p.textSub, p.textMain, p.menuHover, p.accent, p.cardBg));
  }

  if (m_functionListContainer) {
    m_functionListContainer->setStyleSheet(QString(
        "#functionListContainer {"
        "    background-color: %1;"
        "    border-left: 1px solid %2;"
        "}"
        "#outlineHeaderWidget {"
        "    background-color: %3;"
        "    border-bottom: 1px solid %2;"
        "}"
        "#outlineHeaderWidget QLabel {"
        "    color: %4;"
        "    font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "    font-size: 11px;"
        "    font-weight: bold;"
        "}").arg(p.sidebarBg, p.border, p.panelBg, p.textMain));
  }

  if (m_functionTree) {
    m_functionTree->setStyleSheet(QString(
        "QTreeWidget {"
        "    background-color: %1;"
        "    border: none;"
        "    color: %3;"
        "    font-family: 'Consolas', 'Segoe UI', monospace;"
        "    outline: 0;"
        "}"
        "QTreeWidget::item {"
        "    padding: 3px 2px;"
        "    border-bottom: 1px solid %4;"
        "    color: %3;"
        "}"
        "QTreeWidget::item:hover {"
        "    background-color: %5;"
        "}"
        "QTreeWidget::item:selected {"
        "    background-color: %6;"
        "    color: %7;"
        "    font-weight: bold;"
        "}"
        "QTreeWidget::branch:has-children:!has-siblings:closed,"
        "QTreeWidget::branch:closed:has-children:has-siblings {"
        "    image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 24 24' fill='none' stroke='%239CA3AF' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'><polyline points='9 18 15 12 9 6'></polyline></svg>\");"
        "}"
        "QTreeWidget::branch:has-children:!has-siblings:closed:hover,"
        "QTreeWidget::branch:closed:has-children:has-siblings:hover {"
        "    image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 24 24' fill='none' stroke='%23A855F7' stroke-width='3' stroke-linecap='round' stroke-linejoin='round'><polyline points='9 18 15 12 9 6'></polyline></svg>\");"
        "}"
        "QTreeWidget::branch:open:has-children:!has-siblings,"
        "QTreeWidget::branch:open:has-children:has-siblings {"
        "    image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 24 24' fill='none' stroke='%239CA3AF' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'><polyline points='6 9 12 15 18 9'></polyline></svg>\");"
        "}"
        "QTreeWidget::branch:open:has-children:!has-siblings:hover,"
        "QTreeWidget::branch:open:has-children:has-siblings:hover {"
        "    image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 24 24' fill='none' stroke='%23A855F7' stroke-width='3' stroke-linecap='round' stroke-linejoin='round'><polyline points='6 9 12 15 18 9'></polyline></svg>\");"
        "}").arg(p.sidebarBg, p.border, p.textMain, p.borderDark, p.menuHover, p.accent, p.accentText, p.baseAltBg));
  }

  // 3. 语法高亮 C/C++ 词法分析器 (QsciLexerCPP)
  if (m_lexerCPP) {
    m_lexerCPP->setColor(p.synKeyword, QsciLexerCPP::Keyword);
    m_lexerCPP->setColor(p.synString, QsciLexerCPP::DoubleQuotedString);
    m_lexerCPP->setColor(p.synChar, QsciLexerCPP::SingleQuotedString);
    m_lexerCPP->setColor(p.synNumber, QsciLexerCPP::Number);
    m_lexerCPP->setColor(p.synComment, QsciLexerCPP::Comment);
    m_lexerCPP->setColor(p.synComment, QsciLexerCPP::CommentLine);
    m_lexerCPP->setColor(p.synComment, QsciLexerCPP::CommentDoc);
    m_lexerCPP->setColor(p.synComment, QsciLexerCPP::CommentLineDoc);
    m_lexerCPP->setColor(p.synPreprocessor, QsciLexerCPP::PreProcessor);
    m_lexerCPP->setColor(p.synGlobalClass, QsciLexerCPP::GlobalClass);
    m_lexerCPP->setColor(p.synIdentifier, QsciLexerCPP::Identifier);
    m_lexerCPP->setColor(p.synOperator, QsciLexerCPP::Operator);
    m_lexerCPP->setPaper(QColor(p.editorBg));

    QFont font("Consolas", 10);
    m_lexerCPP->setFont(font);
  }

  // 4. 自定义函数名与类名高亮设置
  setupFunctionNameHighlighting(p.isDark);

  // 5. 编辑器迷你工具栏配色
  if (m_toolBar) {
    m_toolBar->setStyleSheet(QString(
        "QToolBar {"
        "    background-color: %1;"
        "    border-bottom: 1px solid %2;"
        "    spacing: 3px;"
        "    padding: 2px 4px;"
        "}"
        "QToolButton {"
        "    background: transparent;"
        "    border: 1px solid transparent;"
        "    border-radius: 3px;"
        "    padding: 2px;"
        "}"
        "QToolButton:hover {"
        "    background-color: %3;"
        "    border: 1px solid %2;"
        "}"
        "QToolButton:pressed {"
        "    background-color: %4;"
        "}").arg(p.panelBg, p.border, p.menuHover, p.accent));
  }

  // 6. 粘性滚动 (Sticky Scroll) 主题更新
  if (m_stickyScrollWidget) {
    static_cast<StickyScrollWidget *>(m_stickyScrollWidget)->updateTheme(p);
  }
  updateStickyScroll();
}

void CodeEditor::setupFunctionNameHighlighting(bool isDarkTheme) {
  // 为每个编辑器设置函数名高亮
  for (QsciScintilla *editor : m_editors) {
    if (!editor)
      continue;

    const int FUNCTION_INDICATOR = 20;
    const int CLASS_INDICATOR = 27;

    // 设置指示器样式为文本前景色
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, FUNCTION_INDICATOR,
                          QsciScintilla::INDIC_TEXTFORE);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, CLASS_INDICATOR,
                          QsciScintilla::INDIC_TEXTFORE);

    // 根据主题设置颜色
    if (isDarkTheme) {
      // 深色主题
      editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, FUNCTION_INDICATOR,
                            0xAADCDC); // 橙色 #DCDCAA
      editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, CLASS_INDICATOR,
                            0xB0C94E); // 青绿色 #4EC9B0
    } else {
      // 浅色主题
      editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, FUNCTION_INDICATOR,
                            0x008CFF); // 深橙色
      editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, CLASS_INDICATOR,
                            0x997F26); // 深青色
    }

    // 设置指示器透明度和优先级
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, FUNCTION_INDICATOR,
                          255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, FUNCTION_INDICATOR,
                          false);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETOUTLINEALPHA,
                          FUNCTION_INDICATOR, 255);

    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, CLASS_INDICATOR,
                          255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, CLASS_INDICATOR,
                          false);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETOUTLINEALPHA,
                          CLASS_INDICATOR, 255);

    // 连接信号(如果尚未连接)
    // 注意：这里为了简单起见，可能会重复连接。
    // 理想情况下应该检查是否连接，或者在setupEditor中只连接一次。
    // 断开所有 textChanged 信号可能会破坏 CodeEditor 的其他功能
    connect(editor, &QsciScintilla::textChanged, [this, editor]() {
      QTimer::singleShot(200, [this, editor]() {
        if (editor)
          highlightFunctionNames(editor, 20);
      });
    });

    // 立即更新
    highlightFunctionNames(editor, FUNCTION_INDICATOR);
  }
}

void CodeEditor::highlightFunctionNames(QsciScintilla *editor,
                                        int indicatorId) {
  if (!editor)
    return;

  // 清除现有的函数名高亮
  // Use SCI_GETTEXTLENGTH for accurate length
  int docLen = editor->SendScintilla(QsciScintilla::SCI_GETTEXTLENGTH);
  editor->clearIndicatorRange(0, 0, editor->lines(), docLen, indicatorId);

  // Ensure document is styled to allow correct style checking
  editor->SendScintilla(QsciScintilla::SCI_COLOURISE, 0, -1);

  // 改进的正则表达式：支持 scoped names (Class::Func)
  // 匹配: (可能的前缀::)函数名 ( ...
  QRegularExpression functionNameRegex(
      R"(\b((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)\s*\()");

  // 关键字过滤列表
  QSet<QString> keywords = {
      "if",       "for",       "while",    "switch",   "catch",   "return",
      "else",     "do",        "break",    "continue", "goto",    "sizeof",
      "typedef",  "volatile",  "register", "extern",   "static",  "auto",
      "const",    "struct",    "union",    "enum",     "class",   "template",
      "typename", "namespace", "using",    "try",      "throw",   "new",
      "delete",   "case",      "default",  "public",   "private", "protected",
      "virtual",  "inline",    "explicit", "friend",   "printf",  "scanf",
      "malloc",   "free",      "strlen",   "strcpy",   "strcmp"};

  int lineCount = editor->lines();
  for (int i = 0; i < lineCount; ++i) {
    // 获取当前行的文本
    QString lineText = editor->text(i);

    // 查找匹配
    QRegularExpressionMatchIterator matches =
        functionNameRegex.globalMatch(lineText);
    while (matches.hasNext()) {
      QRegularExpressionMatch match = matches.next();
      QString functionName = match.captured(1);

      // 提取纯函数名部分用于关键字检查 (test::func -> func)
      QString baseName = functionName;
      int lastColon = functionName.lastIndexOf("::");
      if (lastColon != -1) {
        baseName = functionName.mid(lastColon + 2);
      }

      if (keywords.contains(baseName)) {
        continue;
      }

      // 获取匹配在QString中的位置（字符索引）
      int charStart = match.capturedStart(1);

      // 获取当前行在文档中的起始字节位置
      long lineStartPos =
          editor->SendScintilla(QsciScintilla::SCI_POSITIONFROMLINE, i);

      // 计算匹配项相对于行首的字节偏移量
      QByteArray prefix = lineText.left(charStart).toUtf8();
      int byteOffset = prefix.length();
      int startPos = lineStartPos + byteOffset; // 这是字节位置

      // 检查样式，确保不是在注释或字符串中
      // 注意：使用SCI_GETSTYLEAT需要字节位置
      int style =
          editor->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, startPos);
      bool isCommentOrString = (style == QsciLexerCPP::Comment ||
                                style == QsciLexerCPP::CommentLine ||
                                style == QsciLexerCPP::CommentDoc ||
                                style == QsciLexerCPP::DoubleQuotedString ||
                                style == QsciLexerCPP::SingleQuotedString);

      if (isCommentOrString) {
        continue;
      }

      // 分离类名和函数名进行分别高亮
      if (lastColon != -1) {
        // 有类名限定符 (Class::Func)

        // 1. 高亮类名部分 (Class::) - 使用新的CLASS_INDICATOR (27)
        int classPartLen = lastColon + 2; // Include "::"
        // Convert char length to byte length
        QByteArray classPartBytes = functionName.left(classPartLen).toUtf8();
        int classByteLen = classPartBytes.length();

        editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
                              27); // CLASS_INDICATOR
        editor->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, startPos,
                              classByteLen);

        // 2. 高亮函数名部分 (Func)
        int funcStartPos = startPos + classByteLen;
        QByteArray funcPartBytes = baseName.toUtf8();
        int funcByteLen = funcPartBytes.length();

        editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
                              indicatorId); // FUNCTION_INDICATOR
        editor->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE,
                              funcStartPos, funcByteLen);

      } else {
        // 普通函数名 (Func)
        QByteArray funcBytes = functionName.toUtf8();
        int funcLen = funcBytes.length();

        editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
                              indicatorId);
        editor->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, startPos,
                              funcLen);
      }
    }
  }
}

void CodeEditor::createSplitView(Qt::Orientation orientation) {
  // if (!m_currentEditor) {
  //     return;
  // }

  // // 创建新的分割器，替换当前编辑器
  // QSplitter* splitter = new QSplitter(orientation);

  // // 获取当前编辑器在主分割器中的索引
  // int index = m_mainSplitter->indexOf(m_currentEditor);

  // // 从主分割器中移除当前编辑器
  // m_currentEditor->setParent(nullptr);

  // // 将当前编辑器添加到新分割器
  // splitter->addWidget(m_currentEditor);

  // // 创建新的编辑器
  QsciScintilla *newEditor = new QsciScintilla();
  m_editors.append(newEditor);

  // 初始化函数列表控件
  if (!m_functionTree) {
    createFunctionList();
  }

  // // 设置编辑器属性
  // setupEditor(newEditor);

  // // 设置自动补全
  // setupAutoCompletion(newEditor);

  // // 复制当前编辑器的内容到新编辑器
  // newEditor->setText(m_currentEditor->text());

  // // 将新编辑器添加到分割器
  // splitter->addWidget(newEditor);

  // // 将分割器添加到主分割器
  // m_mainSplitter->insertWidget(index, splitter);

  // // 设置分割器比例
  // splitter->setSizes(QList<int>() << 1 << 1);

  // // 将焦点设置到新编辑器
  // newEditor->setFocus();
  // m_currentEditor = newEditor;

  // // 连接信号和槽
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateFunctionList);
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateVariableList);
  if (!m_currentEditor) {
    return;
  }

  // 获取当前编辑器的父分割器
  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(m_currentEditor->parent());
  if (!parentSplitter) {
    parentSplitter = m_mainSplitter;
  }

  // 创建新的分割器，替换当前编辑器
  QSplitter *splitter = new QSplitter(orientation);

  // 获取当前编辑器在父分割器中的索引
  int index = parentSplitter->indexOf(m_currentEditor);

  // 从父分割器中移除当前编辑器
  m_currentEditor->setParent(nullptr);

  // 将当前编辑器添加到新分割器
  splitter->addWidget(m_currentEditor);

  // 连接文本更新信号
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateFunctionList);

  // 设置编辑器属性
  setupEditor(newEditor);

  // 设置自动补全
  setupAutoCompletion(newEditor);

  // 复制当前编辑器的内容到新编辑器
  newEditor->setText(m_currentEditor->text());

  // 将新编辑器添加到分割器
  splitter->addWidget(newEditor);

  // 将分割器添加到父分割器
  parentSplitter->insertWidget(index, splitter);

  // 设置分割器比例
  splitter->setSizes(QList<int>() << 1 << 1);

  // 将焦点设置到新编辑器
  newEditor->setFocus();
  m_currentEditor = newEditor;

  // 连接信号和槽
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateVariableList);
  connect(newEditor, &QsciScintilla::textChanged, this,
          &CodeEditor::updateFunctionList);
}

// void CodeEditor::closeSplitView()
// {
//     if (!m_currentEditor || m_editors.size() <= 1) {
//         return;
//     }

//     // 获取当前编辑器的父分割器
//     QSplitter* parentSplitter =
//     qobject_cast<QSplitter*>(m_currentEditor->parent()); if
//     (!parentSplitter
//     || parentSplitter == m_mainSplitter) {
//         return;
//     }

//     // 获取分割器中的另一个部件
//     QWidget* otherWidget = nullptr;
//     for (int i = 0; i < parentSplitter->count(); ++i) {
//         QWidget* widget = parentSplitter->widget(i);
//         if (widget != m_currentEditor) {
//             otherWidget = widget;
//             break;
//         }
//     }

//     if (!otherWidget) {
//         return;
//     }

//     // 获取父分割器的父部件
//     QWidget* grandParent =
//     qobject_cast<QWidget*>(parentSplitter->parent()); QSplitter*
//     grandParentSplitter = qobject_cast<QSplitter*>(grandParent);

//     // 获取父分割器在其父分割器中的索引
//     int index = -1;
//     if (grandParentSplitter) {
//         index = grandParentSplitter->indexOf(parentSplitter);
//     } else if (grandParent == m_mainSplitter) {
//         index = m_mainSplitter->indexOf(parentSplitter);
//         grandParentSplitter = m_mainSplitter;
//     }

//     // 从父分割器中移除另一个部件
//     otherWidget->setParent(nullptr);

//     // 将另一个部件添加到父分割器的父部件中
//     if (index != -1 && grandParentSplitter) {
//         grandParentSplitter->insertWidget(index, otherWidget);
//     } else {
//         // 如果找不到父分割器的父部件，则添加到主分割器
//         m_mainSplitter->addWidget(otherWidget);
//     }

//     // 从编辑器列表中移除当前编辑器
//     m_editors.removeOne(m_currentEditor);

//     // 删除当前编辑器
//     delete m_currentEditor;

//     // 删除父分割器
//     delete parentSplitter;

//     // 更新当前编辑器
//     if (qobject_cast<QsciScintilla*>(otherWidget)) {
//         m_currentEditor = qobject_cast<QsciScintilla*>(otherWidget);
//     } else if (QSplitter* otherSplitter =
//     qobject_cast<QSplitter*>(otherWidget)) {
//         // 如果另一个部件是分割器，则找到其中的第一个编辑器
//         m_currentEditor = findFirstEditor(otherSplitter);
//     } else {
//         m_currentEditor = m_editors.first();
//     }

//     // 将焦点设置到当前编辑器
//     if (m_currentEditor) {
//         m_currentEditor->setFocus();
//     }
// }

// void CodeEditor::closeSplitView()
// {
//     if (!m_currentEditor || m_editors.size() <= 1) {
//         return;
//     }

//     // 获取当前编辑器的父分割器
//     QSplitter* parentSplitter =
//     qobject_cast<QSplitter*>(m_currentEditor->parent()); if
//     (!parentSplitter
//     || parentSplitter == m_mainSplitter) {
//         return;
//     }

//     // 获取分割器中的另一个部件
//     QWidget* otherWidget = nullptr;
//     for (int i = 0; i < parentSplitter->count(); ++i) {
//         QWidget* widget = parentSplitter->widget(i);
//         if (widget != m_currentEditor) {
//             otherWidget = widget;
//             break;
//         }
//     }

//     if (!otherWidget) {
//         return;
//     }

//     // 获取父分割器的父部件 - 修复类型转换问题
//     QWidget* grandParent =
//     qobject_cast<QWidget*>(parentSplitter->parent()); QSplitter*
//     grandParentSplitter = nullptr;

//     // 检查父分割器的父部件是否是分割器或主分割器
//     if (grandParent == m_mainSplitter) {
//         grandParentSplitter = m_mainSplitter;
//     } else {
//         grandParentSplitter = qobject_cast<QSplitter*>(grandParent);
//     }

//     // 获取父分割器在其父分割器中的索引
//     int index = -1;
//     if (grandParentSplitter) {
//         index = grandParentSplitter->indexOf(parentSplitter);
//     }

//     // 从父分割器中移除另一个部件
//     otherWidget->setParent(nullptr);

//     // 将另一个部件添加到父分割器的父部件中
//     if (index != -1 && grandParentSplitter) {
//         grandParentSplitter->insertWidget(index, otherWidget);
//     } else {
//         // 如果找不到父分割器的父部件，则添加到主分割器
//         m_mainSplitter->addWidget(otherWidget);
//     }

//     // 从编辑器列表中移除当前编辑器
//     m_editors.removeOne(m_currentEditor);

//     // 删除当前编辑器
//     delete m_currentEditor;

//     // 删除父分割器
//     delete parentSplitter;

//     // 更新当前编辑器
//     if (qobject_cast<QsciScintilla*>(otherWidget)) {
//         m_currentEditor = qobject_cast<QsciScintilla*>(otherWidget);
//     } else if (QSplitter* otherSplitter =
//     qobject_cast<QSplitter*>(otherWidget)) {
//         // 如果另一个部件是分割器，则找到其中的第一个编辑器
//         m_currentEditor = findFirstEditor(otherSplitter);
//     } else {
//         m_currentEditor = m_editors.first();
//     }

//     // 将焦点设置到当前编辑器
//     if (m_currentEditor) {
//         m_currentEditor->setFocus();
//     }
// }

void CodeEditor::closeSplitView() {
  if (!m_currentEditor || m_editors.size() <= 1) {
    return;
  }

  // 获取当前编辑器的父分割器
  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(m_currentEditor->parent());
  if (!parentSplitter || parentSplitter == m_mainSplitter) {
    return;
  }

  // 获取分割器中的另一个部件
  QWidget *otherWidget = nullptr;
  for (int i = 0; i < parentSplitter->count(); ++i) {
    QWidget *widget = parentSplitter->widget(i);
    if (widget != m_currentEditor) {
      otherWidget = widget;
      break;
    }
  }

  if (!otherWidget) {
    return;
  }

  // 从编辑器列表中移除当前编辑器
  m_editors.removeOne(m_currentEditor);

  // 保存当前编辑器的引用，以便后面删除
  QsciScintilla *editorToDelete = m_currentEditor;

  // 获取父分割器的父部件
  QWidget *grandParent = qobject_cast<QWidget *>(parentSplitter->parent());
  QSplitter *grandParentSplitter = nullptr;

  // 先更新当前编辑器引用，避免使用已删除的指针
  if (qobject_cast<QsciScintilla *>(otherWidget)) {
    m_currentEditor = qobject_cast<QsciScintilla *>(otherWidget);
  } else if (QSplitter *otherSplitter =
                 qobject_cast<QSplitter *>(otherWidget)) {
    // 如果另一个部件是分割器，则找到其中的第一个编辑器
    m_currentEditor = findFirstEditor(otherSplitter);
  } else {
    // 如果没有找到合适的编辑器，使用列表中的第一个
    m_currentEditor = m_editors.isEmpty() ? nullptr : m_editors.first();
  }

  // 处理特殊情况：如果父部件是主分割器
  if (grandParent == m_mainSplitter) {
    // 从父分割器中移除另一个部件（先保存引用）
    otherWidget->setParent(nullptr);

    // 获取父分割器在主分割器中的索引
    int index = m_mainSplitter->indexOf(parentSplitter);

    // 将另一个部件添加到主分割器
    m_mainSplitter->insertWidget(index, otherWidget);
  } else {
    // 处理嵌套分割器的情况
    grandParentSplitter = qobject_cast<QSplitter *>(grandParent);
    if (!grandParentSplitter) {
      // 安全检查失败，恢复编辑器列表并返回
      m_editors.append(editorToDelete);
      return;
    }

    // 获取父分割器在祖父分割器中的索引
    int index = grandParentSplitter->indexOf(parentSplitter);

    // 从父分割器中移除另一个部件
    otherWidget->setParent(nullptr);

    // 将另一个部件添加到祖父分割器
    grandParentSplitter->insertWidget(index, otherWidget);
  }

  // 删除当前编辑器和父分割器
  delete editorToDelete;
  delete parentSplitter;

  // 将焦点设置到当前编辑器
  if (m_currentEditor) {
    m_currentEditor->setFocus();
  }
}

// 添加一个辅助方法来查找分割器中的第一个编辑器
QsciScintilla *CodeEditor::findFirstEditor(QSplitter *splitter) {
  if (!splitter) {
    return nullptr;
  }

  for (int i = 0; i < splitter->count(); ++i) {
    QWidget *widget = splitter->widget(i);

    // 如果是编辑器，直接返回
    QsciScintilla *editor = qobject_cast<QsciScintilla *>(widget);
    if (editor) {
      return editor;
    }

    // 如果是分割器，递归查找
    QSplitter *childSplitter = qobject_cast<QSplitter *>(widget);
    if (childSplitter) {
      QsciScintilla *foundEditor = findFirstEditor(childSplitter);
      if (foundEditor) {
        return foundEditor;
      }
    }
  }

  return nullptr;
}

void CodeEditor::setupEditor(QsciScintilla *editor) {
  // 设置编辑器字体
  QFont font("Consolas", 10);
  font.setFixedPitch(true);
  editor->setFont(font);
  editor->setMarginsFont(font);

  // 设置行号边距
  editor->setMarginWidth(0, "0000");
  editor->setMarginLineNumbers(0, true);

  // 设置折叠边距
  editor->setMarginWidth(2, 14);
  editor->setMarginType(2, QsciScintilla::SymbolMargin);
  editor->setMarginSensitivity(2, true);
  editor->setFolding(QsciScintilla::BoxedTreeFoldStyle);

  // 设置C++语法高亮
  editor->setLexer(m_lexerCPP);

  // 设置自动缩进
  editor->setAutoIndent(true);
  editor->setIndentationWidth(4);
  editor->setTabWidth(4);
  editor->setTabIndents(true);
  editor->setBackspaceUnindents(true);

  // 设置UTF-8编码
  editor->setUtf8(true);

  // 设置括号匹配
  editor->setBraceMatching(QsciScintilla::SloppyBraceMatch);

  // ... existing code ...
  // 设置括号匹配
  editor->setBraceMatching(QsciScintilla::SloppyBraceMatch);

  // 设置不同括号对的颜色
  // // 圆括号 ()
  // editor->setMatchedBraceForegroundColor(QColor("#FF0000"));  // 红色
  // editor->setMatchedBraceBackgroundColor(QColor("#FFE4E1"));  //
  // 浅红色背景

  // // 方括号 []
  // editor->setUnmatchedBraceForegroundColor(QColor("#0000FF"));  // 蓝色
  // editor->setUnmatchedBraceBackgroundColor(QColor("#E6E6FA"));  //
  // 光标与活动行
  editor->setCaretWidth(2);
  editor->setCaretLineVisible(true);
  editor->setCaretLineBackgroundColor(QColor("#1C202B"));

  // 设置括号匹配的样式和颜色
  editor->setMatchedBraceBackgroundColor(QColor("#3B514D")); // 匹配的括号背景色
  editor->setMatchedBraceForegroundColor(
      QColor("#FFD700")); // 匹配的括号前景色 - 金色
  editor->setUnmatchedBraceBackgroundColor(
      QColor("#4B1515")); // 不匹配的括号背景色
  editor->setUnmatchedBraceForegroundColor(
      QColor("#FF0000")); // 不匹配的括号前景色 - 红色

  // 设置自动换行
  editor->setWrapMode(QsciScintilla::WrapNone);

  // 设置行尾可见
  editor->setEolVisibility(false);

  // 设置缩进指南
  editor->setIndentationGuides(true);

  // 在setupEditor中确保highlightBraces在适当的时间点调用
  connect(editor, &QsciScintilla::SCN_STYLENEEDED,
          [this, editor](int position) {
            // 在样式需要更新后调用
            updateBracketHighlighting(editor);
          });

  // 添加函数名高亮设置
  setupFunctionNameHighlighting(m_isDarkTheme);

  // 初始化Rainbow Brackets
  setupRainbowBrackets(editor);

  // Define marker 25 for current line number highlighting
  // SC_MARK_BACKGROUND makes the marker affect the line background
  const int CURRENT_LINE_MARKER = 25;
  editor->markerDefine(QsciScintilla::Background, CURRENT_LINE_MARKER);
  editor->setMarkerBackgroundColor(QColor("#E6FFCC"),
                                   CURRENT_LINE_MARKER); // Light green
  editor->setMarkerForegroundColor(QColor("#00AA00"),
                                   CURRENT_LINE_MARKER); // Green text

  // Enable the marker to appear in the line number margin (margin 0)
  editor->SendScintilla(
      QsciScintilla::SCI_SETMARGINMASKN, 0,
      editor->SendScintilla(QsciScintilla::SCI_GETMARGINMASKN, 0) |
          (1 << CURRENT_LINE_MARKER));

  // 设置符号同名/引用高亮指示器 (OCCURRENCE_INDICATOR = 28)
  editor->indicatorDefine(QsciScintilla::RoundBoxIndicator, OCCURRENCE_INDICATOR);
  editor->setIndicatorForegroundColor(QColor(m_isDarkTheme ? "#89B4FA" : "#2563EB"), OCCURRENCE_INDICATOR);
  editor->setIndicatorDrawUnder(false, OCCURRENCE_INDICATOR);
  editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, (unsigned long)OCCURRENCE_INDICATOR, (long)(m_isDarkTheme ? 75 : 70));
  editor->SendScintilla(QsciScintilla::SCI_INDICSETOUTLINEALPHA, (unsigned long)OCCURRENCE_INDICATOR, (long)255);

  // 设置上下文右键菜单
  editor->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(editor, &QsciScintilla::customContextMenuRequested, this, &CodeEditor::onCustomContextMenuRequested);

  // 连接光标移动与选区改变信号以实时更新高亮、面包屑、Sticky Scroll 与同名符号高亮
  connect(editor, &QsciScintilla::cursorPositionChanged, this,
          [this](int line, int col) {
            Q_UNUSED(col);
            highlightCurrentLineNumber();
            updateBreadcrumb(line);
            updateStickyScroll();
            if (m_occurrenceTimer) m_occurrenceTimer->start();
          });
  connect(editor, &QsciScintilla::selectionChanged, this, [this]() {
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

  // 安装快捷键事件过滤器与快捷键对象 (确保 F12, Shift+F12, F2 100% 触发)
  editor->installEventFilter(this);
  if (editor->viewport()) {
    editor->viewport()->installEventFilter(this);
  }

  new QShortcut(QKeySequence(Qt::Key_F12), editor, this, &CodeEditor::gotoDefinitionAtCursor);
  new QShortcut(QKeySequence("Shift+F12"), editor, this, &CodeEditor::findReferencesAtCursor);
  new QShortcut(QKeySequence(Qt::Key_F2), editor, this, &CodeEditor::renameSymbolAtCursor);

  // Initialize line number highlighting, breadcrumb & sticky scroll
  highlightCurrentLineNumber();
  updateBreadcrumb();
  updateStickyScroll();
}

void CodeEditor::setupRainbowBrackets(QsciScintilla *editor) {
  // Define unique indicators for varying nesting levels
  // Use indicators 21-26 (RAINBOW_LEVEL_1 to RAINBOW_LEVEL_6)

  QList<QColor> colors;
  if (m_isDarkTheme) {
    // Dark Theme Colors (Vibrant)
    colors << QColor("#FFD700")  // Level 1: Gold
           << QColor("#DA70D6")  // Level 2: Orchid
           << QColor("#179FFF")  // Level 3: SkyBlue
           << QColor("#FFB6C1")  // Level 4: LightPink
           << QColor("#00FFFF")  // Level 5: Cyan
           << QColor("#7CFC00"); // Level 6: LawnGreen
  } else {
    // Light Theme Colors (Darker for contrast)
    colors << QColor("#CC9900")  // Level 1
           << QColor("#800080")  // Level 2
           << QColor("#0000FF")  // Level 3
           << QColor("#C71585")  // Level 4
           << QColor("#008080")  // Level 5
           << QColor("#228B22"); // Level 6
  }

  for (int i = 0; i < 6; ++i) {
    int indicator = RAINBOW_LEVEL_1 + i;
    editor->indicatorDefine(QsciScintilla::PlainIndicator,
                            indicator); // Simple text coloring
    editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, indicator,
                          QsciScintilla::INDIC_TEXTFORE);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE, indicator,
                          (colors[i].blue() << 16) | (colors[i].green() << 8) |
                              colors[i].red());
    editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, indicator, 255);
    editor->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, indicator, false);
  }

  // Connect to textChanged with debounce to avoid heavy processing on
  // every keystroke
  editor->disconnect(
      SIGNAL(textChanged())); // Careful, this might disconnect others
  // Better: use a specific connection or rely on the single valid
  // connection. In `setupEditor` we already have connections. Let's add a
  // connection specifically for brackets if not present.

  // Note: We are using a unified updateBracketHighlighting now.
  connect(editor, &QsciScintilla::textChanged, [this, editor]() {
    // Debounce?
    // For now, simple direct call or short timer
    updateBracketHighlighting(editor);
  });

  // Initial update
  updateBracketHighlighting(editor);
}

// Remove duplication: empty implementation for old method if it exists or
// reuse

// 添加新方法：设置不同类型括号的颜色
// void CodeEditor::setupBraceColors(QsciScintilla* editor)
// {
//     // 定义不同类型的括号指示器
//     const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
//     const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
//     const int CURLY_BRACE_INDICATOR = 10; // 花括号 {}
//     const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

//     // 设置圆括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator,
//     ROUND_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#4EC9B0"),
//     ROUND_BRACE_INDICATOR); // 青绿色
//     editor->setIndicatorOutlineColor(QColor("#4EC9B0"),
//     ROUND_BRACE_INDICATOR);

//     // 设置方括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator,
//     SQUARE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#CE9178"),
//     SQUARE_BRACE_INDICATOR); // 橙色
//     editor->setIndicatorOutlineColor(QColor("#CE9178"),
//     SQUARE_BRACE_INDICATOR);

//     // 设置花括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator,
//     CURLY_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#569CD6"),
//     CURLY_BRACE_INDICATOR); // 蓝色
//     editor->setIndicatorOutlineColor(QColor("#569CD6"),
//     CURLY_BRACE_INDICATOR);

//     // 设置尖括号指示器样式
//     editor->indicatorDefine(QsciScintilla::FullBoxIndicator,
//     ANGLE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#C586C0"),
//     ANGLE_BRACE_INDICATOR); // 紫色
//     editor->setIndicatorOutlineColor(QColor("#C586C0"),
//     ANGLE_BRACE_INDICATOR);

//     // 连接文本变化信号，以便在文本变化时更新括号颜色
//     connect(editor, &QsciScintilla::textChanged, [this, editor]() {
//         highlightBraces(editor);
//     });

//     // 初始化时高亮括号
//     highlightBraces(editor);
// }

// void CodeEditor::setupBraceColors(QsciScintilla* editor)
// {
//     // 定义不同类型的括号指示器
//     const int ROUND_BRACE_INDICATOR = 8;  // 圆括号 ()
//     const int SQUARE_BRACE_INDICATOR = 9; // 方括号 []
//     const int CURLY_BRACE_INDICATOR = 10; // 花括号 {}
//     const int ANGLE_BRACE_INDICATOR = 11; // 尖括号 <>

//     // 提高指示器优先级
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
//     ROUND_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE,
//     ROUND_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
//     ROUND_BRACE_INDICATOR, 0x4EC9B0);

//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
//     SQUARE_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE,
//     SQUARE_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
//     SQUARE_BRACE_INDICATOR, 0xCE9178);

//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
//     CURLY_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE,
//     CURLY_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
//     CURLY_BRACE_INDICATOR, 0x569CD6);

//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
//     ANGLE_BRACE_INDICATOR);
//     editor->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, 1);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE,
//     ANGLE_BRACE_INDICATOR, QsciScintilla::INDIC_TEXTFORE);
//     editor->SendScintilla(QsciScintilla::SCI_INDICSETFORE,
//     ANGLE_BRACE_INDICATOR, 0xC586C0);

//     // 设置圆括号指示器样式 - 使用更明显的样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator,
//     ROUND_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#4EC9B0"),
//     ROUND_BRACE_INDICATOR); // 青绿色

//     // 设置方括号指示器样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator,
//     SQUARE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#CE9178"),
//     SQUARE_BRACE_INDICATOR); // 橙色

//     // 设置花括号指示器样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator,
//     CURLY_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#569CD6"),
//     CURLY_BRACE_INDICATOR); // 蓝色

//     // 设置尖括号指示器样式
//     editor->indicatorDefine(QsciScintilla::PlainIndicator,
//     ANGLE_BRACE_INDICATOR);
//     editor->setIndicatorForegroundColor(QColor("#C586C0"),
//     ANGLE_BRACE_INDICATOR); // 紫色

//     // 连接文本变化信号，以便在文本变化时更新括号颜色
//     connect(editor, &QsciScintilla::textChanged, [this, editor]() {
//         highlightBraces(editor);
//     });

//     // 初始化时高亮括号
//     highlightBraces(editor);
// }

// 1. 修复编译问题 - 更新setupBraceColors方法

void CodeEditor::updateBracketHighlighting(QsciScintilla *editor) {
  if (!editor)
    return;

  // 清除所有括号相关的指示器
  int len = editor->length();
  // 清除普通括号指示器
  // editor->clearIndicatorRange(0, 0, editor->lines(), len,
  // ROUND_BRACE_INDICATOR);

  // 清除Rainbow Brackets指示器
  for (int i = 0; i < 6; ++i) {
    editor->clearIndicatorRange(0, 0, editor->lines(), len,
                                RAINBOW_LEVEL_1 + i);
  }

  // 获取文本长度
  int length = editor->SendScintilla(QsciScintilla::SCI_GETTEXTLENGTH);
  if (length <= 0)
    return;

  // 获取文本内容 (byte buffer)
  std::vector<char> buffer(length + 1);
  editor->SendScintilla(QsciScintilla::SCI_GETTEXT, length + 1, buffer.data());

  // 使用栈来跟踪括号嵌套
  QStack<int> bracketStack;

  // 遍历文本
  for (int i = 0; i < length; ++i) {
    char c = buffer[i];

    bool isOpener = (c == '(' || c == '[' || c == '{');
    bool isCloser = (c == ')' || c == ']' || c == '}');

    if (!isOpener && !isCloser)
      continue;

    // 检查样式，跳过注释和字符串
    int style = editor->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, i);
    bool isCommentOrString =
        (style == QsciLexerCPP::Comment || style == QsciLexerCPP::CommentLine ||
         style == QsciLexerCPP::CommentDoc ||
         style == QsciLexerCPP::DoubleQuotedString ||
         style == QsciLexerCPP::SingleQuotedString);

    if (isCommentOrString)
      continue;

    int depth = bracketStack.size();
    int colorLevel = depth % 6;
    int indicator = RAINBOW_LEVEL_1 + colorLevel;

    if (isOpener) {
      // 高亮左括号
      editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, indicator);
      editor->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, i, 1);

      // 压入栈 (存储括号类型，用于匹配)
      bracketStack.push(c);
    } else if (isCloser) {
      if (bracketStack.isEmpty()) {
        // 不匹配的右括号，忽略或标记错误
        continue;
      }

      char lastOpener = bracketStack.pop();
      bool isMatch = (lastOpener == '(' && c == ')') ||
                     (lastOpener == '[' && c == ']') ||
                     (lastOpener == '{' && c == '}');

      if (isMatch) {
        // 使用弹出后的深度计算颜色（与对应的左括号一致）
        depth = bracketStack.size();
        colorLevel = depth % 6;
        indicator = RAINBOW_LEVEL_1 + colorLevel;

        // 高亮右括号
        editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT,
                              indicator);
        editor->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, i, 1);
      } else {
        // 括号不匹配，可以做特殊处理，这里暂时忽略
      }
    }
  }
}

void CodeEditor::setupAutoCompletion(QsciScintilla *editor) {
  // 如果还没有创建API对象，则创建
  if (!m_apiCPP) {
    m_apiCPP = new QsciAPIs(m_lexerCPP);

    // 添加C/C++关键字和STM32相关API
    QStringList keywords;
    // C/C++关键字
    keywords << "auto"
             << "break"
             << "case"
             << "char"
             << "const"
             << "continue"
             << "default"
             << "do"
             << "double"
             << "else"
             << "enum"
             << "extern"
             << "float"
             << "for"
             << "goto"
             << "if"
             << "int"
             << "long"
             << "register"
             << "return"
             << "short"
             << "signed"
             << "sizeof"
             << "static"
             << "struct"
             << "switch"
             << "typedef"
             << "union"
             << "unsigned"
             << "void"
             << "volatile"
             << "while";

    // STM32数据类型
    keywords << "uint8_t"
             << "uint16_t"
             << "uint32_t"
             << "int8_t"
             << "int16_t"
             << "int32_t"
             << "GPIO_InitTypeDef"
             << "USART_InitTypeDef"
             << "TIM_TimeBaseInitTypeDef";

    // STM32特定函数和宏
    keywords << "GPIO_Init"
             << "GPIO_SetBits"
             << "GPIO_ResetBits"
             << "GPIO_ReadInputDataBit"
             << "TIM_TimeBaseInit"
             << "TIM_Cmd"
             << "TIM_ITConfig"
             << "TIM_GetCounter"
             << "USART_Init"
             << "USART_Cmd"
             << "USART_SendData"
             << "USART_ReceiveData"
             << "ADC_Init"
             << "ADC_Cmd"
             << "ADC_StartConversion"
             << "ADC_GetConversionValue"
             << "RCC_APB1PeriphClockCmd"
             << "RCC_APB2PeriphClockCmd"
             << "RCC_AHB1PeriphClockCmd"
             << "NVIC_Init"
             << "NVIC_EnableIRQ"
             << "NVIC_DisableIRQ"
             << "SysTick_Config"
             << "HAL_Delay"
             << "HAL_GPIO_WritePin"
             << "HAL_GPIO_ReadPin";

    // STM32 HAL库函数
    keywords << "HAL_Init()"
             << "HAL_GPIO_Init()"
             << "HAL_GPIO_WritePin()"
             << "HAL_GPIO_ReadPin()"
             << "HAL_Delay()"
             << "HAL_UART_Init()"
             << "HAL_UART_Transmit()"
             << "HAL_UART_Receive()"
             << "HAL_TIM_Base_Init()"
             << "HAL_TIM_Base_Start()"
             << "HAL_TIM_Base_Stop()";

    // 添加到API
    for (const QString &keyword : keywords) {
      m_apiCPP->add(keyword);
    }

    // 准备API
    m_apiCPP->prepare();
  }

  // 设置自动补全
  editor->setAutoCompletionThreshold(1); // 输入1个字符后显示补全
  editor->setAutoCompletionSource(
      QsciScintilla::AcsAll); // 使用所有可用的补全源
  editor->setAutoCompletionCaseSensitivity(false); // 不区分大小写
  editor->setAutoCompletionReplaceWord(true);      // 替换当前单词
  editor->setAutoCompletionUseSingle(
      QsciScintilla::AcusNever); // 不自动选择唯一匹配项

  // 启用自动补全弹出
  editor->setAutoCompletionFillupsEnabled(true);
}

void CodeEditor::createBreadcrumbBar() {
  m_breadcrumbBar = new QWidget(this);
  m_breadcrumbBar->setObjectName("breadcrumbBar");
  m_breadcrumbBar->setFixedHeight(28);

  QHBoxLayout *layout = new QHBoxLayout(m_breadcrumbBar);
  layout->setContentsMargins(8, 0, 8, 0);
  layout->setSpacing(4);

  // 1. 路径容器 (支持多级目录: e.g. User > sdmmc >)
  m_bcPathContainer = new QWidget(m_breadcrumbBar);
  m_bcPathLayout = new QHBoxLayout(m_bcPathContainer);
  m_bcPathLayout->setContentsMargins(0, 0, 0, 0);
  m_bcPathLayout->setSpacing(4);

  // 2. 文件按钮 (显示 C / C++ / H 徽标和文件名，点击弹出同目录所有文件菜单)
  m_bcFileButton = new QToolButton(m_breadcrumbBar);
  m_bcFileButton->setObjectName("bcFileButton");
  m_bcFileButton->setIcon(createFileBadgeIcon("C++", QColor("#00E5FF"), 16));
  m_bcFileButton->setText("untitled.cpp");
  m_bcFileButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_bcFileButton->setCursor(Qt::PointingHandCursor);
  m_bcFileButton->setToolTip("点击浏览并切换同目录下的其他 C/C++ 源文件或头文件");
  connect(m_bcFileButton, &QToolButton::clicked, this, &CodeEditor::showBreadcrumbFilesMenu);

  // 3. 分隔符 2
  QLabel *sep2 = new QLabel("›", m_breadcrumbBar);
  sep2->setStyleSheet("color: #666666; font-size: 13px; font-weight: bold;");

  // 4. 当前函数符号按钮 (VS Code 风格，紫色 3D 棱箱图标)
  m_bcFuncButton = new QToolButton(m_breadcrumbBar);
  m_bcFuncButton->setObjectName("bcFuncButton");
  m_bcFuncButton->setIcon(createSymbolIcon(QColor("#A855F7"), 16));
  m_bcFuncButton->setText("...");
  m_bcFuncButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_bcFuncButton->setCursor(Qt::PointingHandCursor);
  m_bcFuncButton->setToolTip("点击浏览并跳转当前文件的所有函数 (VS Code 风格)");
  connect(m_bcFuncButton, &QToolButton::clicked, this, &CodeEditor::showBreadcrumbFunctionsMenu);

  layout->addWidget(m_bcPathContainer);
  layout->addWidget(m_bcFileButton);
  layout->addWidget(sep2);
  layout->addWidget(m_bcFuncButton);
  layout->addStretch();

  // 5. 函数大纲显隐切换按钮
  m_outlineToggleBtn = new QToolButton(m_breadcrumbBar);
  m_outlineToggleBtn->setObjectName("outlineToggleBtn");
  m_outlineToggleBtn->setIcon(createOutlineIcon(QColor("#A855F7"), 16));
  m_outlineToggleBtn->setText(" 函数大纲");
  m_outlineToggleBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_outlineToggleBtn->setCheckable(true);
  m_outlineToggleBtn->setChecked(false);
  m_outlineToggleBtn->setCursor(Qt::PointingHandCursor);
  m_outlineToggleBtn->setToolTip("开启/关闭右侧函数大纲列表 (Alt+O)");
  connect(m_outlineToggleBtn, &QToolButton::toggled, this, &CodeEditor::setFunctionOutlineVisible);

  layout->addWidget(m_outlineToggleBtn);
}

void CodeEditor::setFunctionOutlineVisible(bool visible) {
  if (m_functionListContainer) {
    m_functionListContainer->setVisible(visible);
    if (visible) {
      QList<int> sizes = m_mainSplitter->sizes();
      if (sizes.size() >= 2) {
        int total = sizes[0] + sizes[1];
        if (total <= 0) total = 1000;
        int outlineWidth = qBound(200, total / 4, 320);
        m_mainSplitter->setSizes({total - outlineWidth, outlineWidth});
      }
    }
  }
  if (m_outlineToggleBtn && m_outlineToggleBtn->isChecked() != visible) {
    m_outlineToggleBtn->blockSignals(true);
    m_outlineToggleBtn->setChecked(visible);
    m_outlineToggleBtn->blockSignals(false);
  }
}

void CodeEditor::toggleFunctionOutline() {
  bool nextVisible = m_functionListContainer ? !m_functionListContainer->isVisible() : true;
  setFunctionOutlineVisible(nextVisible);
}

static int findSymbolColumnInLine(const QString &lineText, const QString &symbolName, int expectedCol = -1) {
  if (lineText.isEmpty() || symbolName.isEmpty()) return -1;

  int symLen = symbolName.length();

  // 1. 若期望列号处恰好精确匹配该符号名，且不在行内 // 注释中，直接返回
  if (expectedCol >= 0 && expectedCol + symLen <= lineText.length()) {
    if (lineText.mid(expectedCol, symLen) == symbolName) {
      int commentIdx = lineText.indexOf("//");
      if (commentIdx == -1 || expectedCol < commentIdx) {
        return expectedCol;
      }
    }
  }

  // 2. 剥离行尾 // 注释
  QString codeOnly = lineText;
  int commentIdx = codeOnly.indexOf("//");
  if (commentIdx != -1) {
    codeOnly = codeOnly.left(commentIdx);
  }

  // 若整行代码已是注释行（/* ... */ 或 * ...），则排除
  QString trimmed = codeOnly.trimmed();
  if (trimmed.startsWith("/*") || trimmed.startsWith('*') || trimmed.startsWith("//")) {
    return -1;
  }

  // 3. 在非注释代码部分以完整词边界正则查找
  QString esc = QRegularExpression::escape(symbolName);
  QString pattern = symbolName.startsWith('~') ?
      QString(R"(~(?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(QRegularExpression::escape(symbolName.mid(1))) :
      QString(R"((?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(esc);
  QRegularExpression re(pattern);
  QRegularExpressionMatch match = re.match(codeOnly);
  if (match.hasMatch()) {
    return match.capturedStart();
  }

  // 4. 次级容错查找
  int idx = codeOnly.lastIndexOf(symbolName);
  if (idx != -1) {
    return idx;
  }

  return -1;
}

void CodeEditor::navigateToSymbol(const FunctionInfo &info) {
  if (!m_currentEditor) return;

  int targetLine = info.startLine;
  int targetCol = info.startCol;
  QString baseName = info.scopedName;
  if (baseName.contains("::")) {
    baseName = baseName.split("::").last();
  }

  int totalLines = m_currentEditor->lines();
  if (targetLine >= totalLines) targetLine = totalLines - 1;
  if (targetLine < 0) targetLine = 0;

  // 在目标行验证并校准列号 (严格排除注释干扰，优先精准匹配 baseName)
  QString lineText = m_currentEditor->text(targetLine);
  int foundCol = findSymbolColumnInLine(lineText, baseName, targetCol);
  if (foundCol == -1 && baseName.startsWith('~')) {
    foundCol = findSymbolColumnInLine(lineText, baseName.mid(1), targetCol);
  }

  if (foundCol == -1) {
    // 若因用户即时编辑发生微小偏移，在前后 12 行内智能检索该符号名 (严格排除注释行与注释内容)
    for (int delta = 1; delta <= 12; ++delta) {
      if (targetLine + delta < totalLines) {
        QString nextLine = m_currentEditor->text(targetLine + delta);
        int c = findSymbolColumnInLine(nextLine, baseName);
        if (c == -1 && baseName.startsWith('~')) c = findSymbolColumnInLine(nextLine, baseName.mid(1));
        if (c != -1) {
          targetLine = targetLine + delta;
          foundCol = c;
          break;
        }
      }
      if (targetLine - delta >= 0) {
        QString prevLine = m_currentEditor->text(targetLine - delta);
        int c = findSymbolColumnInLine(prevLine, baseName);
        if (c == -1 && baseName.startsWith('~')) c = findSymbolColumnInLine(prevLine, baseName.mid(1));
        if (c != -1) {
          targetLine = targetLine - delta;
          foundCol = c;
          break;
        }
      }
    }
  }

  if (foundCol != -1) {
    targetCol = foundCol;
  }

  int nameLen = baseName.length();

  // 1. 设置光标位置并确保行可见
  m_currentEditor->setCursorPosition(targetLine, targetCol);
  m_currentEditor->ensureLineVisible(targetLine);

  // 2. 将目标行平滑滚动至视口大约 1/3 黄金阅读位置
  int visibleLines = m_currentEditor->SendScintilla(QsciScintilla::SCI_LINESONSCREEN);
  int scrollLine = qMax(0, targetLine - visibleLines / 3);
  m_currentEditor->SendScintilla(QsciScintilla::SCI_SETFIRSTVISIBLELINE, scrollLine);

  // 3. 精准高亮/选中符号名称（函数名、构造/析构函数名、变量名或结构体名）
  m_currentEditor->setSelection(targetLine, targetCol, targetLine, targetCol + nameLen);
  m_currentEditor->setFocus();

  // 4. 实时更新面包屑导航
  updateBreadcrumb(targetLine);
}

void CodeEditor::showBreadcrumbFilesMenu() {
  QDir dir;
  if (!m_currentFilePath.isEmpty()) {
    dir = QFileInfo(m_currentFilePath).dir();
  } else {
    dir = QDir::current();
  }

  QStringList filters = {"*.c", "*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp"};
  QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);

  QMenu menu(this);
  menu.setObjectName("breadcrumbFilesMenu");
  menu.setStyleSheet(
      "QMenu {"
      "    background-color: #1E1E2E;"
      "    border: 1px solid #3B4252;"
      "    border-radius: 6px;"
      "    padding: 4px;"
      "    font-family: 'Consolas', 'Segoe UI', monospace;"
      "    font-size: 12px;"
      "}"
      "QMenu::item {"
      "    padding: 6px 24px 6px 8px;"
      "    border-radius: 4px;"
      "    color: #ECEFF4;"
      "}"
      "QMenu::item:selected {"
      "    background-color: rgba(0, 229, 255, 0.18);"
      "    border: 1px solid #00E5FF;"
      "    color: #FFFFFF;"
      "}");

  if (fileList.isEmpty()) {
    QAction *emptyAct = menu.addAction("(同目录下未检测到其他源文件)");
    emptyAct->setEnabled(false);
  } else {
    for (const QFileInfo &fInfo : fileList) {
      QString ext = fInfo.suffix().toLower();
      QString badge = "C";
      if (ext == "cpp" || ext == "cc" || ext == "cxx") badge = "C++";
      else if (ext == "h" || ext == "hpp") badge = "H";

      QIcon fileIcon = createFileBadgeIcon(badge, QColor("#00E5FF"), 16);
      QAction *act = menu.addAction(fileIcon, fInfo.fileName());

      if (!m_currentFilePath.isEmpty() && fInfo.absoluteFilePath() == QFileInfo(m_currentFilePath).absoluteFilePath()) {
        QFont f = act->font();
        f.setBold(true);
        act->setFont(f);
      }

      connect(act, &QAction::triggered, this, [this, fInfo]() {
        openFile(fInfo.absoluteFilePath());
      });
    }
  }

  QPoint globalPos = m_bcFileButton->mapToGlobal(QPoint(0, m_bcFileButton->height() + 2));
  menu.exec(globalPos);
}

void CodeEditor::showBreadcrumbFunctionsMenu() {
  if (m_functions.isEmpty() && m_currentEditor) {
    parseFunctions(m_currentEditor->text());
  }

  // 创建悬浮弹出窗口 (VS Code 风格树状大纲弹窗，支持一级类/结构体展开与收缩)
  QFrame *popup = new QFrame(this, Qt::Popup | Qt::FramelessWindowHint);
  popup->setAttribute(Qt::WA_DeleteOnClose);
  popup->setObjectName("breadcrumbTreePopup");
  popup->setStyleSheet(
      "#breadcrumbTreePopup {"
      "    background-color: #1E1E2E;"
      "    border: 1px solid #3B4252;"
      "    border-radius: 6px;"
      "}"
      "QTreeWidget {"
      "    background-color: transparent;"
      "    border: none;"
      "    color: #ECEFF4;"
      "    font-family: 'Consolas', 'Segoe UI', monospace;"
      "    outline: 0;"
      "}"
      "QTreeWidget::item {"
      "    padding: 3px 2px;"
      "    border-bottom: 1px solid rgba(255, 255, 255, 0.05);"
      "    color: #ECEFF4;"
      "}"
      "QTreeWidget::item:hover {"
      "    background-color: rgba(168, 85, 247, 0.18);"
      "}"
      "QTreeWidget::item:selected {"
      "    background-color: rgba(168, 85, 247, 0.35);"
      "    color: #FFFFFF;"
      "    font-weight: bold;"
      "}"
      "QTreeWidget::branch:has-children:!has-siblings:closed,"
      "QTreeWidget::branch:closed:has-children:has-siblings {"
      "    image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 24 24' fill='none' stroke='%239CA3AF' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'><polyline points='9 18 15 12 9 6'></polyline></svg>\");"
      "}"
      "QTreeWidget::branch:has-children:!has-siblings:closed:hover,"
      "QTreeWidget::branch:closed:has-children:has-siblings:hover {"
      "    image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 24 24' fill='none' stroke='%23A855F7' stroke-width='3' stroke-linecap='round' stroke-linejoin='round'><polyline points='9 18 15 12 9 6'></polyline></svg>\");"
      "}"
      "QTreeWidget::branch:open:has-children:!has-siblings,"
      "QTreeWidget::branch:open:has-children:has-siblings {"
      "    image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 24 24' fill='none' stroke='%239CA3AF' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'><polyline points='6 9 12 15 18 9'></polyline></svg>\");"
      "}"
      "QTreeWidget::branch:open:has-children:!has-siblings:hover,"
      "QTreeWidget::branch:open:has-children:has-siblings:hover {"
      "    image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 24 24' fill='none' stroke='%23A855F7' stroke-width='3' stroke-linecap='round' stroke-linejoin='round'><polyline points='6 9 12 15 18 9'></polyline></svg>\");"
      "}");

  QVBoxLayout *popupLayout = new QVBoxLayout(popup);
  popupLayout->setContentsMargins(4, 4, 4, 4);
  popupLayout->setSpacing(0);

  QTreeWidget *tree = new QTreeWidget(popup);
  tree->setObjectName("breadcrumbOutlineTree");
  tree->setHeaderHidden(true);
  tree->setRootIsDecorated(true);
  tree->setIndentation(18);
  tree->setAnimated(true);
  tree->setExpandsOnDoubleClick(true);
  tree->setItemDelegate(new SymbolTreeDelegate(tree));
  tree->setFont(QFont("Consolas", 10));
  tree->setSelectionMode(QAbstractItemView::SingleSelection);
  tree->setSortingEnabled(false);
  popupLayout->addWidget(tree);

  int currentCursorLine = 0;
  if (m_currentEditor) {
    int col = 0;
    m_currentEditor->getCursorPosition(&currentCursorLine, &col);
  }

  QTreeWidgetItem *activeItem = nullptr;

  if (m_functions.isEmpty()) {
    QTreeWidgetItem *emptyItem = new QTreeWidgetItem(tree);
    emptyItem->setText(0, " (未检测到符号定义)");
    emptyItem->setIcon(0, createSymbolIcon(QColor("#A855F7"), 16));
  } else {
    // 1. 搜集所有类 / 结构体顶级节点
    QList<FunctionInfo> containerList;
    for (const FunctionInfo &info : m_functions) {
      if (info.type == SymbolClass || info.type == SymbolStruct) {
        containerList.append(info);
      }
    }

    // 2. 将类/结构体及其挂载的二级成员加入树状结构
    QSet<QString> addedContainers;
    for (const FunctionInfo &cls : containerList) {
      if (addedContainers.contains(cls.scopedName)) continue;
      addedContainers.insert(cls.scopedName);

      QTreeWidgetItem *classItem = new QTreeWidgetItem(tree);
      int clsIdx = m_functions.indexOf(cls);
      classItem->setData(0, Qt::UserRole, clsIdx);
      classItem->setData(0, Qt::UserRole + 1, cls.scopedName);
      classItem->setData(0, Qt::UserRole + 8, (int)cls.type);
      classItem->setIcon(0, getIconForSymbol(cls.type));

      if (currentCursorLine >= cls.startLine && currentCursorLine <= cls.endLine) {
        activeItem = classItem;
      }

      // 挂载该类/结构体的所有二级成员
      for (int i = 0; i < m_functions.size(); ++i) {
        const FunctionInfo &mem = m_functions[i];
        if (mem.parentClass == cls.scopedName) {
          QTreeWidgetItem *memItem = new QTreeWidgetItem(classItem);
          memItem->setData(0, Qt::UserRole, i);
          memItem->setData(0, Qt::UserRole + 1, mem.scopedName);
          memItem->setData(0, Qt::UserRole + 6, mem.returnType);
          memItem->setData(0, Qt::UserRole + 7, mem.params);
          memItem->setData(0, Qt::UserRole + 3, mem.refCount);
          memItem->setData(0, Qt::UserRole + 8, (int)mem.type);
          memItem->setIcon(0, getIconForSymbol(mem.type));

          if (currentCursorLine >= mem.startLine && currentCursorLine <= mem.endLine) {
            activeItem = memItem;
          }
        }
      }

      // 默认展开一级目录
      classItem->setExpanded(true);
    }

    // 3. 将未挂载在任何类/结构体下面的全局函数、全局变量、宏定义等作为顶级项加入
    for (int i = 0; i < m_functions.size(); ++i) {
      const FunctionInfo &info = m_functions[i];
      if (info.parentClass.isEmpty() && info.type != SymbolClass && info.type != SymbolStruct) {
        QTreeWidgetItem *item = new QTreeWidgetItem(tree);
        item->setData(0, Qt::UserRole, i);
        item->setData(0, Qt::UserRole + 1, info.scopedName);
        item->setData(0, Qt::UserRole + 6, info.returnType);
        item->setData(0, Qt::UserRole + 7, info.params);
        item->setData(0, Qt::UserRole + 3, info.refCount);
        item->setData(0, Qt::UserRole + 8, (int)info.type);
        item->setIcon(0, getIconForSymbol(info.type));

        if (currentCursorLine >= info.startLine && currentCursorLine <= info.endLine) {
          activeItem = item;
        }
      }
    }
  }

  if (activeItem) {
    tree->setCurrentItem(activeItem);
    tree->scrollToItem(activeItem);
  }

  connect(tree, &QTreeWidget::itemClicked, popup, [this, popup](QTreeWidgetItem *item) {
    if (!item) return;
    if (item->childCount() > 0) {
      item->setExpanded(!item->isExpanded());
      return;
    }
    int index = item->data(0, Qt::UserRole).toInt();
    if (index >= 0 && index < m_functions.size()) {
      navigateToSymbol(m_functions[index]);
    }
    popup->close();
  });

  // 计算弹窗高度与宽度 (确保完整显示长参数签名，对齐图 2 样式)
  int totalVisibleItems = 0;
  for (int i = 0; i < tree->topLevelItemCount(); ++i) {
    ++totalVisibleItems;
    QTreeWidgetItem *top = tree->topLevelItem(i);
    if (top && top->isExpanded()) {
      totalVisibleItems += top->childCount();
    }
  }
  int calculatedHeight = qBound(140, totalVisibleItems * 26 + 18, 450);

  int maxContentWidth = 380;
  QFont nameFont("Consolas", 9, QFont::Bold);
  QFontMetrics fmName(nameFont);
  QFont subFont("Consolas", 9);
  QFontMetrics fmSub(subFont);

  for (int i = 0; i < m_functions.size(); ++i) {
    const FunctionInfo &info = m_functions[i];
    int itemW = 55 + fmName.horizontalAdvance(info.scopedName);
    if (!info.returnType.isEmpty()) itemW += 6 + fmSub.horizontalAdvance(info.returnType);
    if (!info.params.isEmpty()) itemW += 6 + fmSub.horizontalAdvance(info.params);
    if (info.refCount > 0) itemW += 25;
    if (itemW > maxContentWidth) maxContentWidth = itemW;
  }
  int calculatedWidth = qBound(380, maxContentWidth + 40, 700);
  popup->resize(calculatedWidth, calculatedHeight);

  QPoint globalPos = m_bcFuncButton->mapToGlobal(QPoint(0, m_bcFuncButton->height() + 2));
  popup->move(globalPos);
  popup->show();
}

void CodeEditor::updateBreadcrumb(int cursorLine) {
  if (!m_breadcrumbBar || !m_currentEditor) return;

  // 1. 更新多级路径组件 (e.g. User > sdmmc > 或 PhudonTools >)
  if (m_bcPathLayout) {
    QLayoutItem *child;
    while ((child = m_bcPathLayout->takeAt(0)) != nullptr) {
      if (child->widget()) child->widget()->deleteLater();
      delete child;
    }

    QStringList pathSegments;
    if (!m_currentFilePath.isEmpty()) {
      QDir dir = QFileInfo(m_currentFilePath).dir();
      QString dirPath = dir.absolutePath();
      dirPath.replace('\\', '/');
      QStringList parts = dirPath.split('/', Qt::SkipEmptyParts);
      if (parts.size() >= 2) {
        pathSegments.append(parts[parts.size() - 2]);
        pathSegments.append(parts[parts.size() - 1]);
      } else if (parts.size() == 1) {
        pathSegments.append(parts.first());
      } else {
        pathSegments.append("PhudonTools");
      }
    } else {
      pathSegments.append("PhudonTools");
    }

    for (int i = 0; i < pathSegments.size(); ++i) {
      QLabel *pLabel = new QLabel(pathSegments[i], m_bcPathContainer);
      pLabel->setStyleSheet("color: #888888; font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; font-size: 12px;");
      m_bcPathLayout->addWidget(pLabel);

      QLabel *pSep = new QLabel("›", m_bcPathContainer);
      pSep->setStyleSheet("color: #666666; font-size: 13px; font-weight: bold;");
      m_bcPathLayout->addWidget(pSep);
    }
  }

  // 2. 更新文件按钮 (显示 C / C++ 徽标与文件名)
  QString fileName = m_currentFilePath.isEmpty() ? "untitled.cpp" : QFileInfo(m_currentFilePath).fileName();
  QString ext = QFileInfo(fileName).suffix().toLower();
  QString badge = "C++";
  if (ext == "c") badge = "C";
  else if (ext == "h" || ext == "hpp") badge = "H";

  if (m_bcFileButton) {
    m_bcFileButton->setIcon(createFileBadgeIcon(badge, QColor("#00E5FF"), 16));
    m_bcFileButton->setText(fileName);
  }

  // 3. 当前光标所在函数/符号
  if (cursorLine < 0) {
    int col = 0;
    m_currentEditor->getCursorPosition(&cursorLine, &col);
  }

  QString currentFuncName = "...";
  QIcon currentFuncIcon = createSymbolIcon(QColor("#A855F7"), 16);

  for (const FunctionInfo &info : m_functions) {
    if (cursorLine >= info.startLine && cursorLine <= info.endLine) {
      currentFuncName = info.scopedName;
      currentFuncIcon = getIconForSymbol(info.type);
      break;
    }
  }

  if (currentFuncName == "..." && !m_functions.isEmpty()) {
    for (int i = m_functions.size() - 1; i >= 0; --i) {
      if (m_functions[i].startLine <= cursorLine) {
        currentFuncName = m_functions[i].scopedName;
        currentFuncIcon = getIconForSymbol(m_functions[i].type);
        break;
      }
    }
  }

  if (currentFuncName == "..." && !m_functions.isEmpty()) {
    currentFuncName = m_functions.first().scopedName;
    currentFuncIcon = getIconForSymbol(m_functions.first().type);
  }

  if (m_bcFuncButton) {
    m_bcFuncButton->setIcon(currentFuncIcon);
    m_bcFuncButton->setText(currentFuncName);
  }
}

void CodeEditor::createFunctionList() {
  m_functionListContainer = new QWidget(this);
  m_functionListContainer->setObjectName("functionListContainer");
  QVBoxLayout *containerLayout = new QVBoxLayout(m_functionListContainer);
  containerLayout->setContentsMargins(0, 0, 0, 0);
  containerLayout->setSpacing(0);

  // 顶部标题栏 + 关闭按钮
  QWidget *headerWidget = new QWidget(m_functionListContainer);
  headerWidget->setObjectName("outlineHeaderWidget");
  QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
  headerLayout->setContentsMargins(8, 4, 6, 4);
  headerLayout->setSpacing(4);

  m_outlineTitleLabel = new QLabel("大纲 (0)", headerWidget);
  m_outlineTitleLabel->setStyleSheet("font-weight: 600; font-size: 11px;");

  QToolButton *closeBtn = new QToolButton(headerWidget);
  closeBtn->setText("✕");
  closeBtn->setCursor(Qt::PointingHandCursor);
  closeBtn->setToolTip("关闭函数大纲");
  closeBtn->setStyleSheet(
      "QToolButton { background: transparent; border: none; font-size: 12px; font-weight: bold; color: #888888; padding: 2px 4px; border-radius: 3px; }"
      "QToolButton:hover { background-color: rgba(239, 68, 68, 0.2); color: #EF4444; }");
  connect(closeBtn, &QToolButton::clicked, this, [this]() {
    setFunctionOutlineVisible(false);
  });

  headerLayout->addWidget(m_outlineTitleLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(closeBtn);

  containerLayout->addWidget(headerWidget);

  // 创建函数与类大纲树状控件 (QTreeWidget 支持一级类/结构体、二级成员自由收缩/展开)
  m_functionTree = new QTreeWidget(m_functionListContainer);
  m_functionTree->setObjectName("outlineTreeWidget");
  m_functionTree->setHeaderHidden(true);
  m_functionTree->setRootIsDecorated(true);
  m_functionTree->setIndentation(18);
  m_functionTree->setAnimated(true);
  m_functionTree->setExpandsOnDoubleClick(true);
  m_functionTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_functionTree->setTextElideMode(Qt::ElideNone);
  m_functionTree->setItemDelegate(new SymbolTreeDelegate(m_functionTree));
  containerLayout->addWidget(m_functionTree);

  // 设置属性
  m_functionTree->setFont(QFont("Consolas", 10));
  m_functionTree->setSelectionMode(QAbstractItemView::SingleSelection);
  m_functionTree->setSortingEnabled(false);

  // 将大纲容器添加到主分割器（默认隐藏）
  m_mainSplitter->addWidget(m_functionListContainer);
  m_functionListContainer->setVisible(false);

  if (m_functionTree) {
    m_functionTree->setMaximumWidth(550);
    m_functionTree->setMinimumWidth(220);
  }

  // 点击树节点跳转
  connect(m_functionTree, &QTreeWidget::itemClicked, this,
          [this](QTreeWidgetItem *item) {
            if (!m_currentEditor || !item) return;
            if (item->childCount() > 0) {
              item->setExpanded(!item->isExpanded());
            }
            int index = item->data(0, Qt::UserRole).toInt();
            if (index >= 0 && index < m_functions.size()) {
              navigateToSymbol(m_functions[index]);
            }
          });
}

// 在已屏蔽注释与字符串的 clean 文本切片 [startLimit, endLimit] 中，精准定位符号名称在整篇代码中的绝对字符索引
static int findExactSymbolOffsetInClean(const QString &cleanText, int startLimit, int endLimit,
                                        const QString &baseName, const QString &scopedName = QString(),
                                        int parenPos = -1) {
  if (startLimit < 0) startLimit = 0;
  if (endLimit > cleanText.length()) endLimit = cleanText.length();
  if (startLimit >= endLimit || baseName.isEmpty()) return startLimit;

  // 策略 1: 若提供了形参左括号 parenPos（函数/方法），从 '(' 往前跳过空白字符，其前面紧邻的必定是函数名末尾
  if (parenPos != -1 && parenPos <= endLimit && parenPos > startLimit) {
    int p = parenPos - 1;
    while (p >= startLimit && cleanText[p].isSpace()) {
      --p;
    }
    if (p >= startLimit) {
      int bLen = baseName.length();
      if (p - bLen + 1 >= startLimit) {
        if (cleanText.mid(p - bLen + 1, bLen) == baseName) {
          return p - bLen + 1;
        }
      }
    }
  }

  // 策略 2: 若带有类作用域 (如 ClassName::FuncName)，先精确寻找完整作用域名
  if (!scopedName.isEmpty() && scopedName.contains("::")) {
    int scIdx = cleanText.lastIndexOf(scopedName, endLimit);
    if (scIdx >= startLimit && scIdx <= endLimit) {
      int colonIdx = scopedName.lastIndexOf("::");
      return scIdx + colonIdx + 2;
    }
    // 带有空格的作用域 (ClassName :: FuncName) 正则匹配
    QString escapedCls = QRegularExpression::escape(scopedName.split("::").first());
    QString escapedBase = QRegularExpression::escape(baseName);
    QRegularExpression scopeRe(QString(R"(\b%1\s*::\s*(%2)\b)").arg(escapedCls, escapedBase));
    QRegularExpressionMatchIterator iter = scopeRe.globalMatch(cleanText.mid(startLimit, endLimit - startLimit));
    int lastMatch = -1;
    while (iter.hasNext()) {
      QRegularExpressionMatch m = iter.next();
      lastMatch = startLimit + m.capturedStart(1);
    }
    if (lastMatch != -1) {
      return lastMatch;
    }
  }

  // 策略 3: 在 [startLimit, endLimit] 范围内通过正则完整词边界查找 baseName
  QString esc = QRegularExpression::escape(baseName);
  QString pattern = baseName.startsWith('~') ?
      QString(R"(~(?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(QRegularExpression::escape(baseName.mid(1))) :
      QString(R"((?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(esc);
  QRegularExpression re(pattern);
  QRegularExpressionMatchIterator iter = re.globalMatch(cleanText.mid(startLimit, endLimit - startLimit));
  int bestPos = -1;
  while (iter.hasNext()) {
    QRegularExpressionMatch m = iter.next();
    int candidate = startLimit + m.capturedStart();
    if (parenPos != -1) {
      // 优先选择最靠近 '(' 的那个匹配项（避免匹配到返回类型中的同名符号）
      if (candidate < parenPos) {
        bestPos = candidate;
      }
    } else {
      bestPos = candidate;
    }
  }
  if (bestPos != -1) {
    return bestPos;
  }

  // 策略 4: 兜底最后一次出现
  int idx = cleanText.lastIndexOf(baseName, endLimit);
  if (idx >= startLimit) {
    return idx;
  }
  idx = cleanText.indexOf(baseName, startLimit);
  if (idx != -1 && idx <= endLimit) {
    return idx;
  }

  // 策略 5: 寻找 startLimit 之后第一个非空白字符
  int p = startLimit;
  while (p < endLimit && cleanText[p].isSpace()) ++p;
  return p < endLimit ? p : startLimit;
}

void CodeEditor::parseFunctions(const QString &code) {
  m_functions.clear();
  if (code.isEmpty()) return;

  const int codeLen = code.length();
  bool isHeader = m_currentFilePath.endsWith(".h", Qt::CaseInsensitive) ||
                  m_currentFilePath.endsWith(".hpp", Qt::CaseInsensitive);

  // 步骤 1：精确屏蔽注释与字符串字面量，并严格隔离预处理指令 (#include, #define 等)
  QString clean = code;
  bool inBlockComment = false;
  bool inLineComment = false;
  bool inString = false;
  bool inChar = false;
  bool inRawString = false;
  QString rawStringDelimiter;

  for (int i = 0; i < codeLen; ++i) {
    QChar ch = code[i];
    QChar nextCh = (i + 1 < codeLen) ? code[i + 1] : QChar('\0');

    if (inBlockComment) {
      if (ch == '*' && nextCh == '/') {
        clean[i] = ' ';
        clean[i + 1] = ' ';
        inBlockComment = false;
        ++i;
      } else if (ch != '\n') {
        clean[i] = ' ';
      }
      continue;
    }

    if (inLineComment) {
      if (ch == '\n') {
        inLineComment = false;
      } else {
        clean[i] = ' ';
      }
      continue;
    }

    if (inRawString) {
      if (ch == ')' && code.mid(i + 1).startsWith(rawStringDelimiter + "\"")) {
        int endPos = i + 1 + rawStringDelimiter.length() + 1;
        for (int k = i; k < endPos && k < codeLen; ++k) {
          if (clean[k] != '\n') clean[k] = ' ';
        }
        i = endPos - 1;
        inRawString = false;
      } else if (ch != '\n') {
        clean[i] = ' ';
      }
      continue;
    }

    if (inString) {
      if (ch == '\\' && i + 1 < codeLen) {
        clean[i] = ' ';
        if (clean[i + 1] != '\n') clean[i + 1] = ' ';
        ++i;
      } else if (ch == '"') {
        clean[i] = ' ';
        inString = false;
      } else if (ch != '\n') {
        clean[i] = ' ';
      }
      continue;
    }

    if (inChar) {
      if (ch == '\\' && i + 1 < codeLen) {
        clean[i] = ' ';
        if (clean[i + 1] != '\n') clean[i + 1] = ' ';
        ++i;
      } else if (ch == '\'') {
        clean[i] = ' ';
        inChar = false;
      } else if (ch != '\n') {
        clean[i] = ' ';
      }
      continue;
    }

    // 检查是否进入注释或字符串
    if (ch == '/' && nextCh == '*') {
      clean[i] = ' ';
      clean[i + 1] = ' ';
      inBlockComment = true;
      ++i;
    } else if (ch == '/' && nextCh == '/') {
      clean[i] = ' ';
      clean[i + 1] = ' ';
      inLineComment = true;
      ++i;
    } else if (ch == 'R' && nextCh == '"' && i + 2 < codeLen) {
      int openParen = code.indexOf('(', i + 2);
      if (openParen != -1 && openParen - (i + 2) < 16) {
        rawStringDelimiter = code.mid(i + 2, openParen - (i + 2));
        inRawString = true;
        for (int k = i; k <= openParen && k < codeLen; ++k) {
          if (clean[k] != '\n') clean[k] = ' ';
        }
        i = openParen;
      } else {
        clean[i] = ' ';
        clean[i + 1] = ' ';
        inString = true;
        ++i;
      }
    } else if (ch == '"') {
      clean[i] = ' ';
      inString = true;
    } else if (ch == '\'') {
      clean[i] = ' ';
      inChar = true;
    }
  }

  // 预处理指令行处理：提取 #define 宏定义至符号列表，并屏蔽其他无关预处理指令
  for (int i = 0; i < codeLen; ++i) {
    if (clean[i] == '#') {
      int lineStart = clean.lastIndexOf('\n', i - 1) + 1;
      QString prefix = clean.mid(lineStart, i - lineStart).trimmed();
      if (prefix.isEmpty()) {
        int lineEnd = clean.indexOf('\n', i);
        if (lineEnd == -1) lineEnd = codeLen;
        while (lineEnd > 0 && lineEnd < codeLen && clean[lineEnd - 1] == '\\') {
          lineEnd = clean.indexOf('\n', lineEnd + 1);
          if (lineEnd == -1) { lineEnd = codeLen; break; }
        }
        QString dirLine = clean.mid(i, lineEnd - i);
        QRegularExpression defRe(R"(^#[ \t]*define[ \t]+([A-Za-z_]\w*)(?:\(([^)]*)\))?)");
        QRegularExpressionMatch defMatch = defRe.match(dirLine);
        if (defMatch.hasMatch()) {
          QString macroName = defMatch.captured(1).trimmed();
          int macroStart = i + defMatch.capturedStart(1);
          int line = code.left(macroStart).count('\n');
          int lStart = code.lastIndexOf('\n', macroStart - 1) + 1;
          int col = macroStart - lStart;

          FunctionInfo mInfo;
          mInfo.scopedName = macroName;
          mInfo.type = SymbolMacro;
          mInfo.params = defMatch.captured(2).isNull() ? "" : QString("(%1)").arg(defMatch.captured(2).trimmed());
          mInfo.indentLevel = 0;
          mInfo.startLine = line;
          mInfo.startCol = col;
          mInfo.endLine = line;
          m_functions.append(mInfo);

          int keepEnd = i + defMatch.capturedEnd();
          for (int k = keepEnd; k < lineEnd; ++k) {
            if (clean[k] != '\n') clean[k] = ' ';
          }
        } else {
          for (int k = i; k < lineEnd; ++k) {
            if (clean[k] != '\n') clean[k] = ' ';
          }
        }
        i = lineEnd - 1;
      }
    }
  }

  // 关键字黑名单
  static const QSet<QString> keywords = {
      "if", "for", "while", "switch", "catch", "return", "else", "do",
      "case", "default", "sizeof", "decltype", "typedef", "struct", "union",
      "enum", "class", "template", "typename", "namespace", "using", "try",
      "throw", "public", "protected", "private", "signals", "slots", "Q_OBJECT",
      "Q_PROPERTY", "Q_DISABLE_COPY", "Q_DECLARE_METATYPE", "emit", "new", "delete",
      "static_assert", "alignas", "alignof", "constexpr", "consteval", "static_cast",
      "dynamic_cast", "reinterpret_cast", "const_cast", "typeid", "noexcept",
      "connect", "disconnect", "goto"
  };

  // 步骤 2：扫描类与结构体定义 (class, struct, typedef struct) 以及其内部所有成员 (方法、成员变量/字段、内嵌结构体)
  QRegularExpression containerDefRegex(R"(\b(?:(?:typedef\s+)?(class|struct)\s+([A-Za-z_]\w*)?|typedef\s+struct)\s*(?::[^{;]*)?\{)");
  QRegularExpressionMatchIterator containerIter = containerDefRegex.globalMatch(clean);
  QList<QPair<int, int>> containerRanges; // 记录每个类/结构体的 [startPos, endPos] 范围

  while (containerIter.hasNext()) {
    QRegularExpressionMatch containerMatch = containerIter.next();
    QString keyword = containerMatch.captured(1);
    QString tagName = containerMatch.captured(2).trimmed();
    int containerOpenBrace = containerMatch.capturedEnd() - 1; // '{'

    // 找到结束大括号 '}'
    int bCount = 1;
    int containerEndBrace = containerOpenBrace + 1;
    while (containerEndBrace < codeLen && bCount > 0) {
      if (clean[containerEndBrace] == '{') ++bCount;
      else if (clean[containerEndBrace] == '}') --bCount;
      if (bCount == 0) break;
      ++containerEndBrace;
    }

    // 检查 typedef struct / struct 结尾处的别名 (如 } StructName;)
    QString containerName = tagName;
    int endStmtPos = containerEndBrace;
    if (containerEndBrace < codeLen) {
      int semiPos = clean.indexOf(';', containerEndBrace);
      if (semiPos != -1 && semiPos - containerEndBrace < 100) {
        QString afterBrace = clean.mid(containerEndBrace + 1, semiPos - containerEndBrace - 1).trimmed();
        if (!afterBrace.isEmpty()) {
          QRegularExpression aliasRegex(R"(^([A-Za-z_]\w*)$)");
          QRegularExpressionMatch aliasMatch = aliasRegex.match(afterBrace);
          if (aliasMatch.hasMatch()) {
            containerName = aliasMatch.captured(1);
            endStmtPos = semiPos;
          }
        }
      }
    }

    if (containerName.isEmpty()) {
      if (!tagName.isEmpty()) containerName = tagName;
      else containerName = "Anonymous";
    }

    int namePos = containerMatch.capturedStart();
    int line = code.left(namePos).count('\n');
    int lineStart = code.lastIndexOf('\n', namePos - 1) + 1;
    int col = namePos - lineStart;

    containerRanges.append(qMakePair(containerMatch.capturedStart(), endStmtPos));

    bool isClass = (keyword == "class");
    FunctionInfo containerInfo;
    containerInfo.scopedName = containerName;
    containerInfo.returnType = "";
    containerInfo.params = "";
    containerInfo.type = isClass ? SymbolClass : SymbolStruct;
    containerInfo.indentLevel = 0;
    containerInfo.startLine = line;
    containerInfo.startCol = col;
    containerInfo.endLine = code.left(endStmtPos < codeLen ? endStmtPos : codeLen).count('\n');
    m_functions.append(containerInfo);

    // 扫描类/结构体内部成员 (方法、字段/成员变量、内嵌结构体)
    QList<FunctionInfo> containerMembers;
    int memPos = containerOpenBrace + 1;
    int memStmtStart = memPos;
    int memBraceDepth = 0;

    while (memPos < containerEndBrace && memPos < codeLen) {
      QChar ch = clean[memPos];

      if (ch == '{') {
        QString stmt = clean.mid(memStmtStart, memPos - memStmtStart).trimmed();
        QRegularExpression structRegex(R"(\b(?:struct|class)\s+([A-Za-z_]\w*)\s*$)");
        QRegularExpressionMatch structMatch = structRegex.match(stmt);
        if (structMatch.hasMatch() && memBraceDepth == 0) {
          QString structName = structMatch.captured(1);
          int structNamePos = clean.lastIndexOf(structName, memPos);
          int sLine = code.left(structNamePos).count('\n');
          int sLineStart = code.lastIndexOf('\n', structNamePos - 1) + 1;
          int sCol = structNamePos - sLineStart;

          int sb = 1;
          int sEnd = memPos + 1;
          while (sEnd < containerEndBrace && sb > 0) {
            if (clean[sEnd] == '{') ++sb;
            else if (clean[sEnd] == '}') --sb;
            if (sb == 0) break;
            ++sEnd;
          }

          FunctionInfo structInfo;
          structInfo.scopedName = structName;
          structInfo.type = stmt.contains("class") ? SymbolClass : SymbolStruct;
          structInfo.indentLevel = 1;
          structInfo.parentClass = containerName;
          structInfo.startLine = sLine;
          structInfo.startCol = sCol;
          structInfo.endLine = code.left(sEnd).count('\n');
          containerMembers.append(structInfo);

          memPos = sEnd;
          memStmtStart = memPos + 1;
          continue;
        }

        // 内联成员函数/构造函数定义 (剥离初始化列表)
        if (!stmt.isEmpty() && memBraceDepth == 0) {
          QString cleanStmt = stmt;
          int pD = 0, aD = 0;
          for (int k = 0; k < cleanStmt.length(); ++k) {
            QChar c = cleanStmt[k];
            if (c == '(') ++pD;
            else if (c == ')') --pD;
            else if (c == '<') ++aD;
            else if (c == '>') --aD;
            else if (c == ':' && pD == 0 && aD == 0) {
              bool isScope = (k > 0 && cleanStmt[k - 1] == ':') || (k + 1 < cleanStmt.length() && cleanStmt[k + 1] == ':');
              if (!isScope) {
                int parenIdx = cleanStmt.lastIndexOf(')', k);
                if (parenIdx != -1) {
                  cleanStmt = cleanStmt.left(k).trimmed();
                  break;
                }
              }
            }
          }

          int openParen = cleanStmt.lastIndexOf('(');
          if (openParen != -1) {
            int closeParen = cleanStmt.indexOf(')', openParen);
            if (closeParen == -1) closeParen = cleanStmt.length() - 1;
            QString beforeParen = cleanStmt.left(openParen).trimmed();
            if (beforeParen.contains('\n')) {
              beforeParen = beforeParen.split('\n').last().trimmed();
            }
            beforeParen.remove(QRegularExpression(R"(\b(public|protected|private|signals|slots|Q_SIGNALS|Q_SLOTS)\s*:\s*)"));
            beforeParen.remove(QRegularExpression(R"(\b(Q_OBJECT|Q_DISABLE_COPY|explicit|virtual|inline|static|constexpr|friend)\b)"));
            beforeParen = beforeParen.trimmed();

            QRegularExpression nameRegex(R"((?:(operator\s*(?:[^\s\w]+|\(\)|\[\]|\w+))|(~?[A-Za-z_]\w*))\s*$)");
            QRegularExpressionMatch nameMatch = nameRegex.match(beforeParen);
            if (nameMatch.hasMatch()) {
              QString funcName = nameMatch.captured(0).trimmed();
              QString retType = beforeParen.left(nameMatch.capturedStart()).trimmed();

              if (!keywords.contains(funcName) && funcName != "Q_OBJECT") {
                int absOpenParen = clean.lastIndexOf('(', memPos);
                if (absOpenParen < memStmtStart) absOpenParen = memPos;
                int funcNamePosInClean = findExactSymbolOffsetInClean(clean, memStmtStart, memPos, funcName, QString(), absOpenParen);

                int absCloseParen = clean.indexOf(')', absOpenParen);
                QString rawParams = (absOpenParen != -1 && absCloseParen != -1) ? clean.mid(absOpenParen, absCloseParen - absOpenParen + 1) : "()";
                QString normParams = normalizeParamTypes(rawParams);

                QString afterParen = cleanStmt.mid(closeParen + 1).trimmed();
                if (afterParen.contains(QRegularExpression(R"(\bconst\b)"))) {
                  normParams += " const";
                }

                int inBraces = 1;
                int inBodyEnd = memPos + 1;
                while (inBodyEnd < containerEndBrace && inBraces > 0) {
                  if (clean[inBodyEnd] == '{') ++inBraces;
                  else if (clean[inBodyEnd] == '}') --inBraces;
                  if (inBraces == 0) break;
                  ++inBodyEnd;
                }

                int memLine = code.left(funcNamePosInClean).count('\n');
                int memLineStart = code.lastIndexOf('\n', funcNamePosInClean - 1) + 1;
                int memCol = funcNamePosInClean - memLineStart;

                FunctionInfo memInfo;
                memInfo.scopedName = funcName;
                memInfo.returnType = (funcName == containerName || funcName == "~" + containerName || funcName.startsWith('~')) ? "" : (retType.isEmpty() ? "void" : retType);
                memInfo.params = normParams;
                memInfo.type = SymbolFunction;
                memInfo.indentLevel = 1;
                memInfo.parentClass = containerName;
                memInfo.startLine = memLine;
                memInfo.startCol = memCol;
                memInfo.endLine = code.left(inBodyEnd).count('\n');
                containerMembers.append(memInfo);

                memPos = inBodyEnd;
                memStmtStart = memPos + 1;
                continue;
              }
            }
          }
        }

        ++memBraceDepth;
        memStmtStart = memPos + 1;
        ++memPos;
        continue;
      }

      if (ch == '}') {
        if (memBraceDepth > 0) --memBraceDepth;
        memStmtStart = memPos + 1;
        ++memPos;
        continue;
      }

      if (ch == ';') {
        QString stmt = clean.mid(memStmtStart, memPos - memStmtStart).trimmed();
        if (!stmt.isEmpty() && memBraceDepth == 0) {
          int openParen = stmt.lastIndexOf('(');
          if (openParen != -1) {
            // 成员函数原型声明
            int closeParen = stmt.indexOf(')', openParen);
            if (closeParen != -1) {
              QString beforeParen = stmt.left(openParen).trimmed();
              if (beforeParen.contains('\n')) {
                beforeParen = beforeParen.split('\n').last().trimmed();
              }
              beforeParen.remove(QRegularExpression(R"(\b(public|protected|private|signals|slots|Q_SIGNALS|Q_SLOTS)\s*:\s*)"));
              beforeParen.remove(QRegularExpression(R"(\b(Q_OBJECT|Q_DISABLE_COPY|explicit|virtual|inline|static|constexpr|friend)\b)"));
              beforeParen = beforeParen.trimmed();

              QRegularExpression nameRegex(R"((?:(operator\s*(?:[^\s\w]+|\(\)|\[\]|\w+))|(~?[A-Za-z_]\w*))\s*$)");
              QRegularExpressionMatch nameMatch = nameRegex.match(beforeParen);
              if (nameMatch.hasMatch()) {
                QString funcName = nameMatch.captured(0).trimmed();
                QString retType = beforeParen.left(nameMatch.capturedStart()).trimmed();

                if (!keywords.contains(funcName) && funcName != "Q_OBJECT") {
                  int absOpenParen = clean.lastIndexOf('(', memPos);
                  if (absOpenParen < memStmtStart) absOpenParen = memPos;
                  int funcNamePosInClean = findExactSymbolOffsetInClean(clean, memStmtStart, memPos, funcName, QString(), absOpenParen);

                  int absCloseParen = clean.indexOf(')', absOpenParen);
                  QString rawParams = (absOpenParen != -1 && absCloseParen != -1) ? clean.mid(absOpenParen, absCloseParen - absOpenParen + 1) : "()";
                  QString normParams = normalizeParamTypes(rawParams);

                  QString afterParen = stmt.mid(closeParen + 1).trimmed();
                  if (afterParen.contains(QRegularExpression(R"(\bconst\b)"))) {
                    normParams += " const";
                  }

                  int memLine = code.left(funcNamePosInClean).count('\n');
                  int memLineStart = code.lastIndexOf('\n', funcNamePosInClean - 1) + 1;
                  int memCol = funcNamePosInClean - memLineStart;

                  FunctionInfo memInfo;
                  memInfo.scopedName = funcName;
                  memInfo.returnType = (funcName == containerName || funcName == "~" + containerName || funcName.startsWith('~')) ? "" : (retType.isEmpty() ? "void" : retType);
                  memInfo.params = normParams;
                  memInfo.type = SymbolFunction;
                  memInfo.indentLevel = 1;
                  memInfo.parentClass = containerName;
                  memInfo.startLine = memLine;
                  memInfo.startCol = memCol;
                  memInfo.endLine = memLine;
                  containerMembers.append(memInfo);
                }
              }
            }
          } else {
            // 类 / 结构体内成员变量与字段 (例如 int count;, char buf[64];, SD_HandleTypeDef uSdHandle;)
            QString varDecl = stmt;
            varDecl.remove(QRegularExpression(R"(\b(public|protected|private|signals|slots|Q_SIGNALS|Q_SLOTS)\s*:\s*)"));
            varDecl.remove(QRegularExpression(R"(\b(static|constexpr|const|volatile|mutable)\b)"));
            varDecl = varDecl.trimmed();

            if (!varDecl.isEmpty() && !varDecl.startsWith('#') && !varDecl.startsWith("class ") && !varDecl.startsWith("struct ")) {
              int eqIdx = varDecl.indexOf('=');
              if (eqIdx != -1) varDecl = varDecl.left(eqIdx).trimmed();

              int lastCloseAngle = varDecl.lastIndexOf('>');
              QString typePart;
              QString namesPart;

              if (lastCloseAngle != -1) {
                int angleStart = varDecl.indexOf('<');
                typePart = varDecl.left(angleStart).trimmed();
                namesPart = varDecl.mid(lastCloseAngle + 1).trimmed();
              } else {
                int lastSpace = varDecl.lastIndexOf(' ');
                int lastStar = varDecl.lastIndexOf('*');
                int splitIdx = qMax(lastSpace, lastStar);
                if (splitIdx != -1) {
                  typePart = varDecl.left(splitIdx + 1).trimmed();
                  namesPart = varDecl.mid(splitIdx + 1).trimmed();
                }
              }

              if (!typePart.isEmpty() && !namesPart.isEmpty()) {
                QStringList varNames = namesPart.split(',', Qt::SkipEmptyParts);
                for (QString vName : varNames) {
                  vName = vName.trimmed();
                  QString finalType = typePart;
                  if (vName.startsWith('*')) {
                    finalType += " *";
                    vName = vName.mid(1).trimmed();
                  }
                  if (vName.contains('[')) {
                    vName = vName.left(vName.indexOf('[')).trimmed();
                  }

                  if (!keywords.contains(vName) && !keywords.contains(finalType) && vName != "Q_OBJECT") {
                    int namePosInClean = findExactSymbolOffsetInClean(clean, memStmtStart, memPos, vName, QString(), -1);
                    int memLine = code.left(namePosInClean).count('\n');
                    int memLineStart = code.lastIndexOf('\n', namePosInClean - 1) + 1;
                    int memCol = namePosInClean - memLineStart;

                    FunctionInfo vInfo;
                    vInfo.scopedName = vName;
                    vInfo.returnType = finalType;
                    vInfo.params = "";
                    vInfo.type = SymbolVariable;
                    vInfo.indentLevel = 1;
                    vInfo.parentClass = containerName;
                    vInfo.startLine = memLine;
                    vInfo.startCol = memCol;
                    vInfo.endLine = memLine;
                    containerMembers.append(vInfo);
                  }
                }
              }
            }
          }
        }

        memStmtStart = memPos + 1;
        ++memPos;
        continue;
      }

      ++memPos;
    }

    // 内部成员按名称字母升序排序
    std::sort(containerMembers.begin(), containerMembers.end(), [](const FunctionInfo &a, const FunctionInfo &b) {
      return a.scopedName.compare(b.scopedName, Qt::CaseInsensitive) < 0;
    });

    for (const FunctionInfo &mem : containerMembers) {
      m_functions.append(mem);
    }
  }

  // 步骤 3：扫描类与结构体外部的函数/方法定义与声明 (包含全局函数与在 .cpp 中定义的 ClassName::MethodName)
  int pos = 0;
  int stmtStart = 0;
  int braceDepth = 0;

  auto isInContainerRange = [&](int p) -> bool {
    for (const auto &rng : containerRanges) {
      if (p >= rng.first && p <= rng.second) return true;
    }
    return false;
  };

  QList<FunctionInfo> globalSymbols;
  QMap<QString, FunctionInfo> outOfClassClasses; // 在 .cpp 中发现的类分组

  while (pos < codeLen) {
    if (isInContainerRange(pos)) {
      for (const auto &rng : containerRanges) {
        if (pos >= rng.first && pos <= rng.second) {
          pos = rng.second;
        }
      }
      stmtStart = pos + 1;
      ++pos;
      continue;
    }

    QChar ch = clean[pos];

    if (ch == '{') {
      QString stmt = clean.mid(stmtStart, pos - stmtStart).trimmed();
      if (!stmt.isEmpty() && braceDepth == 0) {
        // 关键修复 (图 2 Bug): 剥离 C++ 构造函数初始化列表 (如 ClassName(...) : BaseClass(...), m_var(nullptr))
        QString cleanStmt = stmt;
        int pD = 0, aD = 0;
        for (int k = 0; k < cleanStmt.length(); ++k) {
          QChar c = cleanStmt[k];
          if (c == '(') ++pD;
          else if (c == ')') --pD;
          else if (c == '<') ++aD;
          else if (c == '>') --aD;
          else if (c == ':' && pD == 0 && aD == 0) {
            bool isScope = (k > 0 && cleanStmt[k - 1] == ':') || (k + 1 < cleanStmt.length() && cleanStmt[k + 1] == ':');
            if (!isScope) {
              int parenIdx = cleanStmt.lastIndexOf(')', k);
              if (parenIdx != -1) {
                cleanStmt = cleanStmt.left(k).trimmed();
                break;
              }
            }
          }
        }

        int openParen = cleanStmt.lastIndexOf('(');
        if (openParen != -1) {
          int closeParen = cleanStmt.indexOf(')', openParen);
          if (closeParen == -1) closeParen = cleanStmt.length() - 1;

          QString beforeParen = cleanStmt.left(openParen).trimmed();
          if (beforeParen.contains('\n')) {
            beforeParen = beforeParen.split('\n').last().trimmed();
          }

          QRegularExpression nameRegex(R"((?:(operator\s*(?:[^\s\w]+|\(\)|\[\]|\w+))|((?:~?[A-Za-z_]\w*(?:\s*<[^>]*>)?::)*~?[A-Za-z_]\w*(?:\s*<[^>]*>)?))\s*$)");
          QRegularExpressionMatch nameMatch = nameRegex.match(beforeParen);
          if (nameMatch.hasMatch()) {
            QString rawFuncName = nameMatch.captured(0).trimmed();
            QString baseName = rawFuncName;
            QString parentCls = "";

            if (rawFuncName.contains("::")) {
              QStringList parts = rawFuncName.split("::");
              baseName = parts.last().trimmed();
              parentCls = parts[parts.size() - 2].trimmed();
              if (parentCls.contains('<')) parentCls = parentCls.left(parentCls.indexOf('<')).trimmed();
            }

            QString cleanCheckName = baseName;
            if (cleanCheckName.startsWith('~')) cleanCheckName = cleanCheckName.mid(1);

            if (!keywords.contains(cleanCheckName) && !keywords.contains(rawFuncName)) {
              QString retType = beforeParen.left(nameMatch.capturedStart()).trimmed();
              if (retType.contains('\n')) {
                retType = retType.split('\n').last().trimmed();
              }
              bool isControl = false;
              for (const char *kw : {"if", "for", "while", "switch", "catch", "return", "else", "case"}) {
                if (retType.contains(QRegularExpression(QString(R"(\b%1\b)").arg(kw)))) {
                  isControl = true;
                  break;
                }
              }

              if (!isControl) {
                retType.remove(QRegularExpression(R"(\b(inline|virtual|static|explicit|friend|constexpr|consteval|Q_INVOKABLE|Q_SLOT|Q_SIGNAL|extern)\b)"));
                retType = retType.trimmed();

                if (retType.isEmpty()) {
                  if (!parentCls.isEmpty() && (baseName == parentCls || baseName == "~" + parentCls)) {
                    retType = "";
                  } else if (baseName.startsWith('~')) {
                    retType = "";
                  } else if (rawFuncName == "main") {
                    retType = "int";
                  }
                }

                int absOpenParen = clean.lastIndexOf('(', pos);
                if (absOpenParen < stmtStart) absOpenParen = pos;

                int funcNamePosInClean = findExactSymbolOffsetInClean(clean, stmtStart, pos, baseName, rawFuncName, absOpenParen);

                int absCloseParen = clean.indexOf(')', absOpenParen);
                QString rawParams = (absOpenParen != -1 && absCloseParen != -1) ? clean.mid(absOpenParen, absCloseParen - absOpenParen + 1) : "()";
                QString normParams = normalizeParamTypes(rawParams);

                QString afterParen = cleanStmt.mid(closeParen + 1).trimmed();
                if (afterParen.contains(QRegularExpression(R"(\bconst\b)"))) {
                  normParams += " const";
                }

                int bodyBraces = 1;
                int bodyEnd = pos + 1;
                while (bodyEnd < codeLen && bodyBraces > 0) {
                  if (clean[bodyEnd] == '{') ++bodyBraces;
                  else if (clean[bodyEnd] == '}') --bodyBraces;
                  if (bodyBraces == 0) break;
                  ++bodyEnd;
                }

                int line = code.left(funcNamePosInClean).count('\n');
                int lineStart = code.lastIndexOf('\n', funcNamePosInClean - 1) + 1;
                int col = funcNamePosInClean - lineStart;
                int endLine = code.left(bodyEnd < codeLen ? bodyEnd : codeLen).count('\n');

                FunctionInfo info;
                info.scopedName = baseName;
                info.returnType = (baseName.contains("~") || (!parentCls.isEmpty() && baseName == parentCls)) ? "" : (retType.isEmpty() ? "void" : retType);
                info.params = normParams;
                info.type = SymbolFunction;
                info.indentLevel = parentCls.isEmpty() ? 0 : 1;
                info.parentClass = parentCls;
                info.startLine = line;
                info.startCol = col;
                info.endLine = endLine;

                if (!parentCls.isEmpty() && !outOfClassClasses.contains(parentCls)) {
                  FunctionInfo clsInfo;
                  clsInfo.scopedName = parentCls;
                  clsInfo.returnType = "";
                  clsInfo.params = "";
                  clsInfo.type = SymbolClass;
                  clsInfo.indentLevel = 0;
                  clsInfo.startLine = line;
                  clsInfo.startCol = 0;
                  clsInfo.endLine = endLine;
                  outOfClassClasses.insert(parentCls, clsInfo);
                }

                globalSymbols.append(info);

                pos = bodyEnd;
                stmtStart = pos + 1;
                continue;
              }
            }
          }
        }
      }

      ++braceDepth;
      stmtStart = pos + 1;
      ++pos;
      continue;
    }

    if (ch == ';') {
      QString stmt = clean.mid(stmtStart, pos - stmtStart).trimmed();
      if (!stmt.isEmpty() && (braceDepth == 0 || isHeader) && !stmt.startsWith('#')) {
        int openParen = stmt.lastIndexOf('(');
        if (openParen != -1) {
          int closeParen = stmt.indexOf(')', openParen);
          if (closeParen != -1) {
            QString beforeParen = stmt.left(openParen).trimmed();
            if (beforeParen.contains('\n')) {
              beforeParen = beforeParen.split('\n').last().trimmed();
            }

            QRegularExpression nameRegex(R"((?:(operator\s*(?:[^\s\w]+|\(\)|\[\]|\w+))|((?:~?[A-Za-z_]\w*(?:\s*<[^>]*>)?::)*~?[A-Za-z_]\w*(?:\s*<[^>]*>)?))\s*$)");
            QRegularExpressionMatch nameMatch = nameRegex.match(beforeParen);
            if (nameMatch.hasMatch()) {
              QString rawFuncName = nameMatch.captured(0).trimmed();
              QString baseName = rawFuncName;
              QString parentCls = "";

              if (rawFuncName.contains("::")) {
                QStringList parts = rawFuncName.split("::");
                baseName = parts.last().trimmed();
                parentCls = parts[parts.size() - 2].trimmed();
                if (parentCls.contains('<')) parentCls = parentCls.left(parentCls.indexOf('<')).trimmed();
              }

              QString cleanCheckName = baseName;
              if (cleanCheckName.startsWith('~')) cleanCheckName = cleanCheckName.mid(1);

              if (!keywords.contains(cleanCheckName) && !keywords.contains(rawFuncName)) {
                QString retType = beforeParen.left(nameMatch.capturedStart()).trimmed();
                if (retType.contains('\n')) {
                  retType = retType.split('\n').last().trimmed();
                }
                retType.remove(QRegularExpression(R"(\b(inline|virtual|static|explicit|friend|constexpr|consteval|Q_INVOKABLE|Q_SLOT|Q_SIGNAL|extern)\b)"));
                retType = retType.trimmed();

                int absOpenParen = clean.lastIndexOf('(', pos);
                if (absOpenParen < stmtStart) absOpenParen = pos;

                int funcNamePosInClean = findExactSymbolOffsetInClean(clean, stmtStart, pos, baseName, rawFuncName, absOpenParen);

                int absCloseParen = clean.indexOf(')', absOpenParen);
                QString rawParams = (absOpenParen != -1 && absCloseParen != -1) ? clean.mid(absOpenParen, absCloseParen - absOpenParen + 1) : "()";
                QString normParams = normalizeParamTypes(rawParams);

                int line = code.left(funcNamePosInClean).count('\n');
                int lineStart = code.lastIndexOf('\n', funcNamePosInClean - 1) + 1;
                int col = funcNamePosInClean - lineStart;

                FunctionInfo info;
                info.scopedName = baseName;
                info.returnType = retType.isEmpty() ? (baseName.startsWith('~') ? "" : "void") : retType;
                info.params = normParams;
                info.type = SymbolFunction;
                info.indentLevel = parentCls.isEmpty() ? 0 : 1;
                info.parentClass = parentCls;
                info.startLine = line;
                info.startCol = col;
                info.endLine = line;

                if (!parentCls.isEmpty() && !outOfClassClasses.contains(parentCls)) {
                  FunctionInfo clsInfo;
                  clsInfo.scopedName = parentCls;
                  clsInfo.returnType = "";
                  clsInfo.params = "";
                  clsInfo.type = SymbolClass;
                  clsInfo.indentLevel = 0;
                  clsInfo.startLine = line;
                  clsInfo.startCol = 0;
                  clsInfo.endLine = line;
                  outOfClassClasses.insert(parentCls, clsInfo);
                }

                globalSymbols.append(info);
              }
            }
          }
        } else {
          // 全局变量声明 (例如: SD_HandleTypeDef uSdHandle; int count = 0;)
          QString varDecl = stmt;
          varDecl.remove(QRegularExpression(R"(\b(static|constexpr|const|volatile|extern)\b)"));
          varDecl = varDecl.trimmed();

          if (!varDecl.isEmpty() && !varDecl.startsWith("class ") && !varDecl.startsWith("struct ") && !varDecl.startsWith("typedef ")) {
            int eqIdx = varDecl.indexOf('=');
            if (eqIdx != -1) varDecl = varDecl.left(eqIdx).trimmed();

            int lastCloseAngle = varDecl.lastIndexOf('>');
            QString typePart;
            QString namesPart;

            if (lastCloseAngle != -1) {
              int angleStart = varDecl.indexOf('<');
              typePart = varDecl.left(angleStart).trimmed();
              namesPart = varDecl.mid(lastCloseAngle + 1).trimmed();
            } else {
              int lastSpace = varDecl.lastIndexOf(' ');
              int lastStar = varDecl.lastIndexOf('*');
              int splitIdx = qMax(lastSpace, lastStar);
              if (splitIdx != -1) {
                typePart = varDecl.left(splitIdx + 1).trimmed();
                namesPart = varDecl.mid(splitIdx + 1).trimmed();
              }
            }

            if (!typePart.isEmpty() && !namesPart.isEmpty()) {
              QStringList varNames = namesPart.split(',', Qt::SkipEmptyParts);
              for (QString vName : varNames) {
                vName = vName.trimmed();
                QString finalType = typePart;
                if (vName.startsWith('*')) {
                  finalType += " *";
                  vName = vName.mid(1).trimmed();
                }
                if (vName.contains('[')) {
                  vName = vName.left(vName.indexOf('[')).trimmed();
                }

                if (!keywords.contains(vName) && !keywords.contains(finalType)) {
                  int namePosInClean = findExactSymbolOffsetInClean(clean, stmtStart, pos, vName, QString(), -1);
                  int memLine = code.left(namePosInClean).count('\n');
                  int memLineStart = code.lastIndexOf('\n', namePosInClean - 1) + 1;
                  int memCol = namePosInClean - memLineStart;

                  FunctionInfo vInfo;
                  vInfo.scopedName = vName;
                  vInfo.returnType = finalType;
                  vInfo.params = "";
                  vInfo.type = SymbolVariable;
                  vInfo.indentLevel = 0;
                  vInfo.startLine = memLine;
                  vInfo.startCol = memCol;
                  vInfo.endLine = memLine;
                  globalSymbols.append(vInfo);
                }
              }
            }
          }
        }
      }

      stmtStart = pos + 1;
      ++pos;
      continue;
    }

    ++pos;
  }

  // 将 .cpp 中发现的类加入顶层
  for (const FunctionInfo &cls : outOfClassClasses.values()) {
    bool alreadyExists = false;
    for (const FunctionInfo &f : m_functions) {
      if (f.scopedName == cls.scopedName && (f.type == SymbolClass || f.type == SymbolStruct)) {
        alreadyExists = true;
        break;
      }
    }
    if (!alreadyExists) {
      m_functions.append(cls);
    }
  }

  // 追加全局符号
  m_functions.append(globalSymbols);

  // 计算每个符号在当前文件内的引用/调用计数 (对齐图 3 右侧 +9, 1, 2 显示)
  for (int i = 0; i < m_functions.size(); ++i) {
    QString name = m_functions[i].scopedName;
    if (!name.isEmpty() && name != "main") {
      QRegularExpression refRegex(QString(R"(\b%1\b)").arg(QRegularExpression::escape(name)));
      int count = 0;
      auto matchIter = refRegex.globalMatch(code);
      while (matchIter.hasNext()) {
        matchIter.next();
        ++count;
      }
      m_functions[i].refCount = count;
    } else {
      m_functions[i].refCount = 0;
    }
  }

  // 去重 (相同名称且相同行号与父类只保留一条)
  QList<FunctionInfo> uniqueList;
  for (const FunctionInfo &info : m_functions) {
    bool dup = false;
    for (const FunctionInfo &u : uniqueList) {
      if (u.scopedName == info.scopedName && u.startLine == info.startLine && u.parentClass == info.parentClass) {
        dup = true;
        break;
      }
    }
    if (!dup) {
      uniqueList.append(info);
    }
  }
  m_functions = uniqueList;
}

void CodeEditor::updateFunctionList() {
  if (!m_currentEditor) return;

  parseFunctions(m_currentEditor->text());

  if (m_functionTree) {
    m_functionTree->clear();

    // 1. 搜集所有类 / 结构体顶级节点
    QList<FunctionInfo> classList;
    for (const FunctionInfo &info : m_functions) {
      if (info.type == SymbolClass || info.type == SymbolStruct) {
        classList.append(info);
      }
    }

    // 2. 将类及其挂载的二级成员加入树状结构
    QSet<QString> addedClasses;
    for (const FunctionInfo &cls : classList) {
      if (addedClasses.contains(cls.scopedName)) continue;
      addedClasses.insert(cls.scopedName);

      QTreeWidgetItem *classItem = new QTreeWidgetItem(m_functionTree);
      int clsIdx = m_functions.indexOf(cls);
      classItem->setData(0, Qt::UserRole, clsIdx);
      classItem->setData(0, Qt::UserRole + 1, cls.scopedName);
      classItem->setData(0, Qt::UserRole + 8, (int)cls.type);
      classItem->setIcon(0, getIconForSymbol(cls.type));
      classItem->setToolTip(0, QString("【%1】行 %2: %3").arg(cls.type == SymbolClass ? "类" : "结构体").arg(cls.startLine + 1).arg(cls.scopedName));

      // 挂载该类的所有二级成员
      for (int i = 0; i < m_functions.size(); ++i) {
        const FunctionInfo &mem = m_functions[i];
        if (mem.parentClass == cls.scopedName) {
          QTreeWidgetItem *memItem = new QTreeWidgetItem(classItem);
          memItem->setData(0, Qt::UserRole, i);
          memItem->setData(0, Qt::UserRole + 1, mem.scopedName);
          memItem->setData(0, Qt::UserRole + 6, mem.returnType);
          memItem->setData(0, Qt::UserRole + 7, mem.params);
          memItem->setData(0, Qt::UserRole + 3, mem.refCount);
          memItem->setData(0, Qt::UserRole + 8, (int)mem.type);
          memItem->setIcon(0, getIconForSymbol(mem.type));
          QString typeStr = (mem.type == SymbolVariable) ? "变量" : "函数";
          memItem->setToolTip(0, QString("【%1】行 %2: %3").arg(typeStr).arg(mem.startLine + 1).arg(formatSymbolDisplay(mem)));
        }
      }

      // 默认展开一级类目录
      classItem->setExpanded(true);
    }

    // 3. 将未挂载在任何类下面的全局函数、全局变量、宏定义等作为顶级项加入
    for (int i = 0; i < m_functions.size(); ++i) {
      const FunctionInfo &info = m_functions[i];
      if (info.parentClass.isEmpty() && info.type != SymbolClass && info.type != SymbolStruct) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_functionTree);
        item->setData(0, Qt::UserRole, i);
        item->setData(0, Qt::UserRole + 1, info.scopedName);
        item->setData(0, Qt::UserRole + 6, info.returnType);
        item->setData(0, Qt::UserRole + 7, info.params);
        item->setData(0, Qt::UserRole + 3, info.refCount);
        item->setData(0, Qt::UserRole + 8, (int)info.type);
        item->setIcon(0, getIconForSymbol(info.type));

        QString typeStr = "函数";
        if (info.type == SymbolVariable) typeStr = "变量";
        else if (info.type == SymbolMacro) typeStr = "宏定义";
        item->setToolTip(0, QString("【%1】行 %2: %3").arg(typeStr).arg(info.startLine + 1).arg(formatSymbolDisplay(info)));
      }
    }
  }

  if (m_outlineTitleLabel) {
    m_outlineTitleLabel->setText(QString("大纲 (%1)").arg(m_functions.size()));
  }

  updateBreadcrumb();

  if (m_currentEditor) {
    highlightFunctionNames(m_currentEditor, FUNCTION_INDICATOR);
  }
  updateStickyScroll();
}

// void
// CodeEditor::updateFunctionList()//该版本需要保留，解决了跨行函数问题
// {
//     if (!m_currentEditor || !m_functionList) {
//         return;
//     }

//     // 清空函数列表
//     m_functionList->clear();

//     // 获取编辑器文本
//     QString text = m_currentEditor->text();
//     QStringList lines = text.split('\n');

//     // 存储函数名和行号的映射
//     QMap<QString, int> functionLineMap;

//     // 改进的多行函数识别
//     // 先预处理，将跨行的函数定义合并为单行
//     QStringList processedLines;
//     QList<int> originalLineNumbers;

//     for (int i = 0; i < lines.size(); i++) {
//         QString line = lines[i];
//         QString trimmed = line.trimmed();

//         // 跳过空行和注释
//         if (trimmed.isEmpty() || trimmed.startsWith("//") ||
//         trimmed.startsWith("/*")) {
//             processedLines.append(line);
//             originalLineNumbers.append(i);
//             continue;
//         }
//         // 检查是否是函数定义的开始（包含函数名和左括号）
//         if (trimmed.contains("(") && !trimmed.contains(";") &&
//             !trimmed.startsWith("#") && !trimmed.startsWith("if") &&
//             !trimmed.startsWith("for") && !trimmed.startsWith("while")
//             && !trimmed.startsWith("switch")) {

//             QString multiLineFunction = line;
//             int startLineNum = i;
//             bool foundClosingParen = line.contains(")");
//             bool foundOpenBrace = line.contains("{");
//             bool isComplete = false;

//             //
//             第一步：如果当前行没有右括号，继续合并后续行直到找到右括号
//             if (!foundClosingParen) {
//                 for (int j = i + 1; j < lines.size(); j++) {
//                     QString nextLine = lines[j];
//                     multiLineFunction += " " + nextLine.trimmed();

//                     if (nextLine.contains(")")) {
//                         foundClosingParen = true;
//                         if (nextLine.contains("{")) {
//                             foundOpenBrace = true;
//                         }
//                         i = j; // 更新外层循环的索引
//                         break;
//                     }

//                     // 防止无限循环，最多合并15行
//                     if (j - i > 15) {
//                         break;
//                     }
//                 }
//             }

//             // 第二步：如果找到了右括号但还没找到开括号，继续搜索
//             if (foundClosingParen && !foundOpenBrace) {
//                 // 继续搜索函数修饰符和函数体
//                 for (int j = i + 1; j < lines.size(); j++) {
//                     QString nextLine = lines[j].trimmed();

//                     // 跳过空行
//                     if (nextLine.isEmpty()) {
//                         continue;
//                     }

//                     // 合并这一行
//                     multiLineFunction += " " + nextLine;

//                     // 检查各种可能的情况
//                     if (nextLine.startsWith(":")) {
//                         // 构造函数初始化列表
//                         continue;
//                     }
//                     else if (nextLine.contains("{")) {
//                         // 找到函数体开始
//                         i = j;
//                         foundOpenBrace = true;
//                         isComplete = true;
//                         break;
//                     }
//                     else if (nextLine.contains(";")) {
//                         // 函数声明，不是定义
//                         isComplete = false;
//                         break;
//                     }
//                     else if (nextLine.startsWith("const") ||
//                              nextLine.startsWith("override") ||
//                              nextLine.startsWith("final") ||
//                              nextLine.startsWith("noexcept") ||
//                              nextLine.startsWith("->")) {
//                         // 函数修饰符，继续合并
//                         continue;
//                     }
//                     else {
//                         // 其他情况，可能是多行初始化列表或其他内容
//                         continue;
//                     }

//                     // 防止无限循环
//                     if (j - i > 10) {
//                         break;
//                     }
//                 }
//             } else if (foundClosingParen && foundOpenBrace) {
//                 // 单行函数定义
//                 isComplete = true;
//             }

//             // 只有完整的函数定义才添加到处理列表
//             if (foundClosingParen && (foundOpenBrace || isComplete)) {
//                 processedLines.append(multiLineFunction);
//                 originalLineNumbers.append(startLineNum);
//             } else {
//                 // 不完整的函数定义，按原样处理
//                 processedLines.append(line);
//                 originalLineNumbers.append(i);
//             }
//         } else {
//             processedLines.append(line);
//             originalLineNumbers.append(i);
//         }
//     }

//     //改进的C++函数识别正则表达式
//     QRegularExpression functionRegex(
//         R"(^\s*(?:(?:static|inline|virtual|extern|const|explicit|friend|template\s*<[^>]*>)\s+)*(?:[\w:]+(?:\s*[*&]+)?\s+)+([A-Za-z_]\w*(?:::\w+)*)\s*\([^;]*\)\s*(?:const\s*)?(?:override\s*)?(?:final\s*)?(?:noexcept\s*)?(?:->\s*[\w:]+\s*)?)"
//         );

//     // 构造函数识别正则表达式 - 支持类前缀
//     QRegularExpression constructorRegex(
//         R"(^\s*(?:explicit\s+)?(?:([A-Za-z_]\w*)::)?([A-Za-z_]\w*)\s*\([^;]*\)\s*(?::\s*[^{;]*)?\s*(?:\{|$))"
//         );

//     // 析构函数识别正则表达式 - 支持类前缀
//     QRegularExpression destructorRegex(
//         R"(^\s*(?:virtual\s+)?(?:([A-Za-z_]\w*)::)?~([A-Za-z_]\w*)\s*\(\s*\)\s*(?:override\s*)?(?:final\s*)?(?:noexcept\s*)?)"
//         );

//     // 类定义正则表达式
//     QRegularExpression classRegex(
//         R"(^\s*(?:class|struct|enum)\s+(\w+)(?:\s*:[^{]*)?)"
//         );

//     // 宏定义正则表达式
//     QRegularExpression macroRegex(
//         R"(^\s*#define\s+(\w+)(?:\([^)]*\))?)"
//         );

//     // 存储已识别的类名，用于构造函数识别
//     QSet<QString> classNames;

//     // 关键字过滤列表
//     QSet<QString> keywords = {
//         "if", "for", "while", "switch", "catch", "return", "else",
//         "do", "break", "continue", "goto", "sizeof", "typedef",
//         "volatile", "register", "extern", "static", "auto", "const",
//         "struct", "union", "enum", "class", "template", "typename",
//         "namespace", "using", "try", "throw", "new", "delete", "case",
//         "default"
//     };

//     // // 逐行分析
//     // for (int lineNum = 0; lineNum < lines.size(); lineNum++) {
//     //     QString line = lines[lineNum];
//     //     QString trimmedLine = line.trimmed();

//     //     // 跳过空行和注释行
//     //     if (trimmedLine.isEmpty() || trimmedLine.startsWith("//") ||
//     trimmedLine.startsWith("/*")) {
//     //         continue;
//     //     }

//     //     // 跳过多行注释块
//     //     if (trimmedLine.contains("/*") &&
//     !trimmedLine.contains("*/")) {
//     //         while (lineNum < lines.size() - 1) {
//     //             lineNum++;
//     //             if (lines[lineNum].contains("*/")) {
//     //                 break;
//     //             }
//     //         }
//     //         continue;
//     //     }

//     //     // 跳过预处理指令（除了#define）
//     //     if (trimmedLine.startsWith("#") &&
//     !trimmedLine.startsWith("#define")) {
//     //         continue;
//     //     }

//     // 逐行分析处理后的行
//     for (int lineNum = 0; lineNum < processedLines.size(); lineNum++) {
//         QString line = processedLines[lineNum];
//         QString trimmedLine = line.trimmed();
//         int originalLineNum = originalLineNumbers[lineNum];

//         // 跳过空行和注释行
//         if (trimmedLine.isEmpty() || trimmedLine.startsWith("//") ||
//         trimmedLine.startsWith("/*")) {
//             continue;
//         }

//         // 跳过多行注释块
//         if (trimmedLine.contains("/*") && !trimmedLine.contains("*/"))
//         {
//             while (lineNum < processedLines.size() - 1) {
//                 lineNum++;
//                 if (processedLines[lineNum].contains("*/")) {
//                     break;
//                 }
//             }
//             continue;
//         }

//         // 跳过预处理指令（除了#define）
//         if (trimmedLine.startsWith("#") &&
//         !trimmedLine.startsWith("#define")) {
//             continue;
//         }

//         // 1. 识别类定义
//         QRegularExpressionMatch classMatch = classRegex.match(line);
//         if (classMatch.hasMatch()) {
//             QString className = classMatch.captured(1);
//             if (!keywords.contains(className)) {
//                 functionLineMap["📦🏗️ class " + className] =
//                 originalLineNum; classNames.insert(className);  //
//                 记录类名
//             }
//             continue;
//         }

//         // 首先匹配析构函数
//         QRegularExpressionMatch destructorMatch =
//         destructorRegex.match(line); if (destructorMatch.hasMatch()) {
//             QString classPrefix = destructorMatch.captured(1);  //
//             类前缀 QString className = destructorMatch.captured(2); //
//             类名

//             if (!keywords.contains(className) &&
//             isValidFunctionDefinition(line, lineNum, processedLines)) {
//                 QString displayName;
//                 if (!classPrefix.isEmpty()) {
//                     displayName = classPrefix + "::~" + className +
//                     "()";
//                 } else {
//                     displayName = "~" + className + "()";
//                 }
//                 functionLineMap["🔧💥 " + displayName] =
//                 originalLineNum;
//             }
//         }
//         // 然后匹配构造函数
//         else {
//             QRegularExpressionMatch constructorMatch =
//             constructorRegex.match(line); if
//             (constructorMatch.hasMatch()) {
//                 QString classPrefix = constructorMatch.captured(1);  //
//                 类前缀 QString functionName =
//                 constructorMatch.captured(2);
//                 // 函数名

//                 if (!keywords.contains(functionName) &&
//                     isValidFunctionDefinition(line, lineNum,
//                     processedLines)
//                     && isLikelyConstructor(functionName, line)) {

//                     QString displayName;
//                     if (!classPrefix.isEmpty()) {
//                         displayName = classPrefix + "::" + functionName
//                         +
//                         "()";
//                     } else {
//                         displayName = functionName + "()";
//                     }
//                     functionLineMap["🏗️🔨 " + displayName] =
//                     originalLineNum;
//                 }
//             }
//             // 最后匹配普通C++函数定义
//             else {
//                 QRegularExpressionMatch funcMatch =
//                 functionRegex.match(line); if (funcMatch.hasMatch()) {
//                     QString functionName = funcMatch.captured(1);

//                     // 移除命名空间前缀以获取纯函数名
//                     QString pureFunctionName = functionName;
//                     if (functionName.contains("::")) {
//                         pureFunctionName =
//                         functionName.split("::").last();
//                     }

//                     // 过滤关键字和已识别的构造函数
//                     if (!keywords.contains(functionName) &&
//                         !classNames.contains(functionName) &&
//                         isValidFunctionDefinition(line, lineNum,
//                         processedLines)) {

//                         // 检查是否为main函数
//                         if (functionName == "main") {
//                             //functionLineMap["🚀🎯 " + functionName +
//                             "()"] = originalLineNum; functionLineMap["ⓜ
//                             " + functionName + "()"] = originalLineNum;

//                         } else {
//                             //functionLineMap["⚡⚙️ " + functionName +
//                             "()"] = originalLineNum; functionLineMap["ƒ
//                             " + functionName + "()"] = originalLineNum;
//                         }
//                     }
//                     continue;
//                 }
//             }
//         }

//         // 匹配宏定义
//         QRegularExpressionMatch macroMatch = macroRegex.match(line);
//         if (macroMatch.hasMatch()) {
//             QString macroName = macroMatch.captured(1);
//             if (!keywords.contains(macroName)) {
//                 functionLineMap["🔧📝 #define " + macroName] =
//                 originalLineNum;
//             }
//         }
//     }

//     // 按函数名排序并添加到列表
//     QStringList functionNames = functionLineMap.keys();
//     functionNames.sort(Qt::CaseInsensitive);

//     for (const QString& functionName : functionNames) {
//         QListWidgetItem* item = new QListWidgetItem(functionName);
//         item->setData(Qt::UserRole, functionLineMap[functionName]);
//         m_functionList->addItem(item);
//     }

//     // 同时更新函数名高亮
//     if (m_currentEditor) {
//         highlightFunctionNames(m_currentEditor, FUNCTION_INDICATOR);
//     }

// }



void CodeEditor::updateVariableList() {
  // 检查编辑器是否存在
  if (!m_currentEditor || !m_lexerCPP) {
    return;
  }

  try {
    // 获取当前文本
    QString text = m_currentEditor->text();

    // 创建一个新的API对象
    QsciAPIs *newApi = new QsciAPIs(m_lexerCPP);
    if (!newApi) {
      qDebug() << "创建API对象失败";
      return;
    }

    // 添加C/C++关键字和STM32相关API (重新添加所有标准关键字)
    QStringList keywords;
    // C/C++关键字
    keywords << "auto"
             << "break"
             << "case"
             << "char"
             << "const"
             << "continue"
             << "default"
             << "do"
             << "double"
             << "else"
             << "enum"
             << "extern"
             << "float"
             << "for"
             << "goto"
             << "if"
             << "int"
             << "long"
             << "register"
             << "return"
             << "short"
             << "signed"
             << "sizeof"
             << "static"
             << "struct"
             << "switch"
             << "typedef"
             << "union"
             << "unsigned"
             << "void"
             << "volatile"
             << "while";

    // STM32数据类型
    keywords << "uint8_t"
             << "uint16_t"
             << "uint32_t"
             << "int8_t"
             << "int16_t"
             << "int32_t"
             << "GPIO_InitTypeDef"
             << "USART_InitTypeDef"
             << "TIM_TimeBaseInitTypeDef";

    // STM32特定函数和宏
    keywords << "GPIO_Init"
             << "GPIO_SetBits"
             << "GPIO_ResetBits"
             << "GPIO_ReadInputDataBit"
             << "TIM_TimeBaseInit"
             << "TIM_Cmd"
             << "TIM_ITConfig"
             << "TIM_GetCounter"
             << "USART_Init"
             << "USART_Cmd"
             << "USART_SendData"
             << "USART_ReceiveData"
             << "ADC_Init"
             << "ADC_Cmd"
             << "ADC_StartConversion"
             << "ADC_GetConversionValue"
             << "RCC_APB1PeriphClockCmd"
             << "RCC_APB2PeriphClockCmd"
             << "RCC_AHB1PeriphClockCmd"
             << "NVIC_Init"
             << "NVIC_EnableIRQ"
             << "NVIC_DisableIRQ"
             << "SysTick_Config"
             << "HAL_Delay"
             << "HAL_GPIO_WritePin"
             << "HAL_GPIO_ReadPin";

    // STM32 HAL库函数
    keywords << "HAL_Init()"
             << "HAL_GPIO_Init()"
             << "HAL_GPIO_WritePin()"
             << "HAL_GPIO_ReadPin()"
             << "HAL_Delay()"
             << "HAL_UART_Init()"
             << "HAL_UART_Transmit()"
             << "HAL_UART_Receive()"
             << "HAL_TIM_Base_Init()"
             << "HAL_TIM_Base_Start()"
             << "HAL_TIM_Base_Stop()";

    // 添加关键字到新API
    for (const QString &keyword : keywords) {
      newApi->add(keyword);
    }

    // 使用正则表达式查找变量定义 - 改进的正则表达式
    QRegExp varRegex("\\b(int|char|float|double|uint8_t|uint16_t|uint32_"
                     "t|int8_t|int16_t|"
                     "int32_t|bool|void|unsigned|long|short|signed|"
                     "struct|enum|union)\\s+(["
                     "a-zA-Z_][a-zA-Z0-9_]*)\\s*[;\\[=,)]");

    int pos = 0;
    QSet<QString> variables; // 使用集合避免重复

    while ((pos = varRegex.indexIn(text, pos)) != -1) {
      QString varName = varRegex.cap(2);
      if (!varName.isEmpty() && !keywords.contains(varName)) {
        variables.insert(varName);
        qDebug() << "找到变量: " << varName;
      }
      pos += varRegex.matchedLength();
    }

    // 查找函数参数 - 改进的正则表达式
    QRegExp funcRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^\\)]*)\\)");
    pos = 0;

    while ((pos = funcRegex.indexIn(text, pos)) != -1) {
      QString params = funcRegex.cap(2);
      QStringList paramList = params.split(',');

      for (const QString &param : paramList) {
        // 解析参数定义，例如 "int value" 或 "char* buffer"
        QRegExp paramRegex(
            "\\b(\\w+)\\s+([\\*&]*)\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\b");
        if (paramRegex.indexIn(param) != -1) {
          QString varName = paramRegex.cap(3);
          if (!varName.isEmpty() && !keywords.contains(varName)) {
            variables.insert(varName);
            qDebug() << "找到参数: " << varName;
          }
        }
      }

      pos += funcRegex.matchedLength();
    }

    // 查找for循环中的变量 - 改进的正则表达式
    QRegExp forRegex("for\\s*\\(\\s*(?:int|char|float|double|uint8_t|uint16_t|"
                     "uint32_t)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=");
    pos = 0;

    while ((pos = forRegex.indexIn(text, pos)) != -1) {
      QString varName = forRegex.cap(1);
      if (!varName.isEmpty() && !keywords.contains(varName)) {
        variables.insert(varName);
        qDebug() << "找到for循环变量: " << varName;
      }
      pos += forRegex.matchedLength();
    }

    // 查找结构体和类定义中的成员变量
    QRegExp structRegex("struct\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\{([^}]*)\\}");
    pos = 0;

    while ((pos = structRegex.indexIn(text, pos)) != -1) {
      QString structBody = structRegex.cap(2);
      QRegExp memberRegex("\\b(\\w+)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*[;\\[]");
      int memberPos = 0;

      while ((memberPos = memberRegex.indexIn(structBody, memberPos)) != -1) {
        QString memberName = memberRegex.cap(2);
        if (!memberName.isEmpty() && !keywords.contains(memberName)) {
          variables.insert(memberName);
          qDebug() << "找到结构体成员: " << memberName;
        }
        memberPos += memberRegex.matchedLength();
      }

      pos += structRegex.matchedLength();
    }

    // 将找到的变量添加到API
    for (const QString &var : variables) {
      newApi->add(var);
    }

    // 准备新API
    newApi->prepare();

    // 替换旧的API
    if (m_apiCPP) {
      delete m_apiCPP;
    }
    m_apiCPP = newApi;

    // 设置新的API到词法分析器
    m_lexerCPP->setAPIs(m_apiCPP);

    // 设置自动补全模式，确保变量补全生效
    for (QsciScintilla *editor : m_editors) {
      editor->setAutoCompletionSource(
          QsciScintilla::AcsAll); // 使用所有可用的补全源
    }
  } catch (const std::exception &e) {
    qDebug() << "更新变量列表时发生异常: " << e.what();
    // 确保在异常情况下释放资源
  } catch (...) {
    qDebug() << "更新变量列表时发生未知异常";
    // 确保在异常情况下释放资源
  }
}

void CodeEditor::onEditorChanged(QsciScintilla *editor) {
  if (!editor) {
    qDebug() << "编辑器切换失败: 编辑器指针为空";
    return;
  }

  try {
    if (m_editors.contains(editor)) {
      m_currentEditor = editor;
      if (m_stickyScrollWidget) {
        m_stickyScrollWidget->setParent(m_currentEditor);
      }
      updateStickyScroll();
    }
  } catch (const std::exception &e) {
    qDebug() << "更新变量列表时发生异常: " << e.what();
  } catch (...) {
    qDebug() << "更新变量列表时发生未知异常";
  }
}

// 添加创建工具栏的方法
void CodeEditor::createToolBar() {
  m_toolBar = new QToolBar("编辑器工具栏", this);
  m_toolBar->setMovable(true);
  m_toolBar->setIconSize(QSize(16, 16));

  // 创建动作

  // 使用Qt内置图标
  QStyle *style = QApplication::style();

  // 文件操作
  // QAction* newFileAction = new QAction("新建文件", this);
  // newFileAction->setIcon(style->standardIcon(QStyle::SP_FileIcon));
  // newFileAction->setToolTip("创建新文件");
  // connect(newFileAction, &QAction::triggered, [this]() {
  //     // 实现新建文件功能
  //     emit newFileRequested();
  // });

  QAction *newFileAction = new QAction("新建文件", this);
  newFileAction->setIcon(style->standardIcon(QStyle::SP_FileIcon));
  newFileAction->setToolTip("创建新文件");
  connect(newFileAction, &QAction::triggered, this, &CodeEditor::createNewFile);
  m_toolBar->addAction(newFileAction);

  QAction *openFileAction = new QAction("打开文件", this);
  openFileAction->setIcon(style->standardIcon(QStyle::SP_DialogOpenButton));
  openFileAction->setToolTip("打开文件");
  connect(openFileAction, &QAction::triggered, [this]() {
    // 实现打开文件功能
    emit openFileRequested();
  });
  m_toolBar->addAction(openFileAction);

  QAction *saveFileAction = new QAction("保存文件", this);
  saveFileAction->setIcon(style->standardIcon(QStyle::SP_DialogSaveButton));
  saveFileAction->setToolTip("保存文件");
  connect(saveFileAction, &QAction::triggered, [this]() {
    // 实现保存文件功能
    emit saveFileRequested();
  });
  m_toolBar->addAction(saveFileAction);

  m_toolBar->addSeparator();

  // 水平分栏动作
  QAction *horizontalSplitAction =
      new QAction(QIcon(":/icons/horizontal_split.png"), "水平分栏", this);
  horizontalSplitAction->setToolTip("创建水平分栏");
  connect(horizontalSplitAction, &QAction::triggered,
          [this]() { createSplitView(Qt::Horizontal); });
  m_toolBar->addAction(horizontalSplitAction);

  // 垂直分栏动作
  QAction *verticalSplitAction =
      new QAction(QIcon(":/icons/vertical_split.png"), "垂直分栏", this);
  verticalSplitAction->setToolTip("创建垂直分栏");
  connect(verticalSplitAction, &QAction::triggered,
          [this]() { createSplitView(Qt::Vertical); });
  m_toolBar->addAction(verticalSplitAction);

  // 关闭分栏动作
  QAction *closeSplitAction =
      new QAction(QIcon(":/icons/close_split.png"), "关闭分栏", this);
  closeSplitAction->setToolTip("关闭当前分栏");
  connect(closeSplitAction, &QAction::triggered,
          [this]() { closeSplitView(); });
  m_toolBar->addAction(closeSplitAction);

  m_toolBar->addSeparator();

  // 撤销动作
  QAction *undoAction = new QAction(QIcon(":/icons/undo.png"), "撤销", this);
  undoAction->setToolTip("撤销上一步操作");
  connect(undoAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->undo();
    }
  });
  m_toolBar->addAction(undoAction);

  // 重做动作
  QAction *redoAction = new QAction(QIcon(":/icons/redo.png"), "重做", this);
  redoAction->setToolTip("重做上一步操作");
  connect(redoAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->redo();
    }
  });
  m_toolBar->addAction(redoAction);

  m_toolBar->addSeparator();

  // 剪切动作
  QAction *cutAction = new QAction(QIcon(":/icons/cut.png"), "剪切", this);
  cutAction->setToolTip("剪切选中的文本");
  connect(cutAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->cut();
    }
  });
  m_toolBar->addAction(cutAction);

  // 复制动作
  QAction *copyAction = new QAction(QIcon(":/icons/copy.png"), "复制", this);
  copyAction->setToolTip("复制选中的文本");
  connect(copyAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->copy();
    }
  });
  m_toolBar->addAction(copyAction);

  // 粘贴动作
  QAction *pasteAction = new QAction(QIcon(":/icons/paste.png"), "粘贴", this);
  pasteAction->setToolTip("粘贴文本");
  connect(pasteAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->paste();
    }
  });
  m_toolBar->addAction(pasteAction);

  m_toolBar->addSeparator();

  // 查找动作
  QAction *findAction = new QAction(QIcon(":/icons/find.png"), "查找", this);
  findAction->setToolTip("查找文本");
  connect(findAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // 这里可以实现查找功能，或者调用已有的查找方法
      // 暂时使用简单的实现
      m_currentEditor->findFirst("", false, false, false, true);
    }
  });
  m_toolBar->addAction(findAction);

  // 替换动作
  QAction *replaceAction =
      new QAction(QIcon(":/icons/replace.png"), "替换", this);
  replaceAction->setToolTip("替换文本");
  connect(replaceAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // 这里可以实现替换功能，或者调用已有的替换方法
      // 暂时使用简单的实现
      // 需要更复杂的实现可以添加一个替换对话框
    }
  });
  m_toolBar->addAction(replaceAction);

  m_toolBar->addSeparator();

  // 缩进动作
  QAction *indentAction =
      new QAction(QIcon(":/icons/indent.png"), "增加缩进", this);
  indentAction->setToolTip("增加选中文本的缩进");
  connect(indentAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // QsciScintilla doesn't have an indent() method
      // Instead, we need to manually insert spaces or tabs at the
      // beginning of selected lines
      int lineFrom, indexFrom, lineTo, indexTo;
      m_currentEditor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

      // If no selection, use current line
      if (lineFrom == -1) {
        // Fix: getCursorPosition requires two parameters
        m_currentEditor->getCursorPosition(&lineFrom, &indexFrom);
        lineTo = lineFrom;
      }

      // Begin undo action
      m_currentEditor->beginUndoAction();

      // Add indentation to each line
      for (int line = lineFrom; line <= lineTo; ++line) {
        m_currentEditor->insertAt("    ", line, 0); // Insert 4 spaces
      }

      // End undo action
      m_currentEditor->endUndoAction();
    }
  });
  m_toolBar->addAction(indentAction);

  // 取消缩进动作
  QAction *unindentAction =
      new QAction(QIcon(":/icons/unindent.png"), "减少缩进", this);
  unindentAction->setToolTip("减少选中文本的缩进");
  connect(unindentAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // QsciScintilla doesn't have an unindent() method
      // Instead, we need to manually remove spaces or tabs from the
      // beginning of selected lines
      int lineFrom, indexFrom, lineTo, indexTo;
      m_currentEditor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

      // If no selection, use current line
      if (lineFrom == -1) {
        // Fix: getCursorPosition requires two parameters
        m_currentEditor->getCursorPosition(&lineFrom, &indexFrom);
        lineTo = lineFrom;
      }

      // Begin undo action
      m_currentEditor->beginUndoAction();

      // Remove indentation from each line
      for (int line = lineFrom; line <= lineTo; ++line) {
        QString lineText = m_currentEditor->text(line);
        if (lineText.startsWith("    ")) {
          // Remove 4 spaces
          m_currentEditor->setSelection(line, 0, line, 4);
          m_currentEditor->removeSelectedText();
        } else if (lineText.startsWith("\t")) {
          // Remove tab
          m_currentEditor->setSelection(line, 0, line, 1);
          m_currentEditor->removeSelectedText();
        } else if (lineText.startsWith("  ")) {
          // Remove 2 spaces
          m_currentEditor->setSelection(line, 0, line, 2);
          m_currentEditor->removeSelectedText();
        } else if (lineText.startsWith(" ")) {
          // Remove 1 space
          m_currentEditor->setSelection(line, 0, line, 1);
          m_currentEditor->removeSelectedText();
        }
      }

      // End undo action
      m_currentEditor->endUndoAction();
    }
  });
  m_toolBar->addAction(unindentAction);

  m_toolBar->addSeparator();

  // 注释动作
  QAction *commentAction =
      new QAction(QIcon(":/icons/comment.png"), "注释", this);
  commentAction->setToolTip("注释选中的代码");
  connect(commentAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // 获取选中的文本范围
      int lineFrom, indexFrom, lineTo, indexTo;
      m_currentEditor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

      // 如果没有选中文本，则使用当前行
      if (lineFrom == -1) {
        // Fix: getCursorPosition requires two parameters
        m_currentEditor->getCursorPosition(&lineFrom, &indexFrom);
        lineTo = lineFrom;
        indexTo = indexFrom;
      }

      // 为每一行添加注释
      for (int line = lineFrom; line <= lineTo; ++line) {
        m_currentEditor->insertAt("// ", line, 0);
      }
    }
  });
  m_toolBar->addAction(commentAction);
  // 取消注释动作
  QAction *uncommentAction =
      new QAction(QIcon(":/icons/uncomment.png"), "取消注释", this);
  uncommentAction->setToolTip("取消选中代码的注释");
  connect(uncommentAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      // 获取选中的文本范围
      int lineFrom, indexFrom, lineTo, indexTo;
      m_currentEditor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

      // 如果没有选中文本，则使用当前行
      if (lineFrom == -1) {
        // Fix: getCursorPosition requires two parameters
        m_currentEditor->getCursorPosition(&lineFrom, &indexFrom);
        lineTo = lineFrom;
        indexTo = indexFrom;
      }

      // 为每一行移除注释
      for (int line = lineFrom; line <= lineTo; ++line) {
        QString lineText = m_currentEditor->text(line);
        if (lineText.startsWith("// ")) {
          m_currentEditor->setSelection(line, 0, line, 3);
          m_currentEditor->removeSelectedText();
        } else if (lineText.startsWith("//")) {
          m_currentEditor->setSelection(line, 0, line, 2);
          m_currentEditor->removeSelectedText();
        }
      }
    }
  });
  m_toolBar->addAction(uncommentAction);

  // 构建操作
  QAction *buildAction = new QAction("构建", this);
  buildAction->setIcon(style->standardIcon(QStyle::SP_ComputerIcon));
  buildAction->setToolTip("构建项目");
  connect(buildAction, &QAction::triggered, [this]() {
    // 实现构建功能
    emit buildRequested();
  });
  m_toolBar->addAction(buildAction);

  QAction *cleanAction = new QAction("清理", this);
  cleanAction->setIcon(style->standardIcon(QStyle::SP_TrashIcon));
  cleanAction->setToolTip("清理构建文件");
  connect(cleanAction, &QAction::triggered, [this]() {
    // 实现清理功能
    emit cleanRequested();
  });
  m_toolBar->addAction(cleanAction);

  m_toolBar->addSeparator();

  // 调试操作
  QAction *debugAction = new QAction("调试", this);
  debugAction->setIcon(style->standardIcon(QStyle::SP_BrowserReload));
  debugAction->setToolTip("开始调试");
  connect(debugAction, &QAction::triggered, [this]() {
    // 实现调试功能
    emit debugRequested();
  });
  m_toolBar->addAction(debugAction);

  QAction *runAction = new QAction("运行", this);
  runAction->setIcon(style->standardIcon(QStyle::SP_MediaPlay));
  runAction->setToolTip("运行程序");
  connect(runAction, &QAction::triggered, [this]() {
    // 实现运行功能
    emit runRequested();
  });
  m_toolBar->addAction(runAction);

  QAction *stopAction = new QAction("停止", this);
  stopAction->setIcon(style->standardIcon(QStyle::SP_MediaStop));
  stopAction->setToolTip("停止运行");
  connect(stopAction, &QAction::triggered, [this]() {
    // 实现停止功能
    emit stopRequested();
  });
  m_toolBar->addAction(stopAction);

  m_toolBar->addSeparator();

  // 工具操作
  QAction *serialMonitorAction = new QAction("串口监视器", this);
  serialMonitorAction->setIcon(style->standardIcon(QStyle::SP_ComputerIcon));
  serialMonitorAction->setToolTip("打开串口监视器");
  connect(serialMonitorAction, &QAction::triggered, [this]() {
    // 实现串口监视器功能
    emit serialMonitorRequested();
  });
  m_toolBar->addAction(serialMonitorAction);

  QAction *settingsAction = new QAction("设置", this);
  settingsAction->setIcon(
      style->standardIcon(QStyle::SP_FileDialogDetailedView));
  settingsAction->setToolTip("打开设置");
  connect(settingsAction, &QAction::triggered, [this]() {
    // 实现设置功能
    emit settingsRequested();
  });
  m_toolBar->addAction(settingsAction);

  m_toolBar->addSeparator();

  // 视图操作
  QAction *zoomInAction = new QAction("放大", this);
  zoomInAction->setIcon(style->standardIcon(QStyle::SP_TitleBarMaxButton));
  zoomInAction->setToolTip("放大视图");
  connect(zoomInAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->zoomIn();
    }
  });
  m_toolBar->addAction(zoomInAction);

  QAction *zoomOutAction = new QAction("缩小", this);
  zoomOutAction->setIcon(style->standardIcon(QStyle::SP_TitleBarMinButton));
  zoomOutAction->setToolTip("缩小视图");
  connect(zoomOutAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->zoomOut();
    }
  });
  m_toolBar->addAction(zoomOutAction);

  QAction *resetZoomAction = new QAction("重置缩放", this);
  resetZoomAction->setIcon(
      style->standardIcon(QStyle::SP_TitleBarNormalButton));
  resetZoomAction->setToolTip("重置缩放级别");
  connect(resetZoomAction, &QAction::triggered, [this]() {
    if (m_currentEditor) {
      m_currentEditor->zoomTo(0);
    }
  });
  m_toolBar->addAction(resetZoomAction);

  m_toolBar->addSeparator();

  // 帮助操作
  QAction *helpAction = new QAction("帮助", this);
  helpAction->setIcon(style->standardIcon(QStyle::SP_MessageBoxQuestion));
  helpAction->setToolTip("查看帮助");
  connect(helpAction, &QAction::triggered, [this]() {
    // 实现帮助功能
    emit helpRequested();
  });
  m_toolBar->addAction(helpAction);

  QAction *aboutAction = new QAction("关于", this);
  aboutAction->setIcon(style->standardIcon(QStyle::SP_MessageBoxInformation));
  aboutAction->setToolTip("关于PhudonTools");
  connect(aboutAction, &QAction::triggered, [this]() {
    // 实现关于功能
    emit aboutRequested();
  });
  m_toolBar->addAction(aboutAction);
}

void CodeEditor::createNewFile() {
  // 清空当前编辑器内容
  if (m_currentEditor) {
    m_currentEditor->clear();
  }

  // 重置当前文件路径
  m_currentFilePath = "";

  // 发出信号通知主窗口
  emit newFileRequested();
}

// 添加辅助函数来判断是否为构造函数
bool CodeEditor::isLikelyConstructor(const QString &functionName,
                                     const QString &line) {
  // 构造函数通常满足以下条件：
  // 1. 函数名首字母大写（类名规范）
  // 2. 不包含返回类型
  // 3. 可能包含初始化列表

  if (functionName.isEmpty()) {
    return false;
  }

  // 检查首字母是否大写
  if (!functionName[0].isUpper()) {
    return false;
  }

  // 检查行中是否没有明显的返回类型
  QString trimmed = line.trimmed();

  // 如果行以常见的返回类型开始，则不太可能是构造函数
  QStringList returnTypes = {
      "int",     "void",     "char",     "float",  "double",  "bool",
      "uint8_t", "uint16_t", "uint32_t", "int8_t", "int16_t", "int32_t",
      "std::",   "const",    "static",   "inline", "virtual"};

  for (const QString &type : returnTypes) {
    if (trimmed.startsWith(type + " ")) {
      return false;
    }
  }

  // 如果包含初始化列表（冒号），更可能是构造函数
  if (line.contains(":") && !line.contains("?")) { // 排除三元运算符
    return true;
  }

  return true; // 默认认为可能是构造函数
}

// bool CodeEditor::isValidFunctionDefinition(const QString& line, int
// lineNum, const QStringList& allLines) {
//     QString trimmed = line.trimmed();

//     // 排除预处理指令（除了函数式宏）
//     if (trimmed.startsWith("#") && !trimmed.startsWith("#define")) {
//         return false;
//     }

//     // 排除明显的控制结构
//     if (trimmed.startsWith("if") || trimmed.startsWith("for") ||
//         trimmed.startsWith("while") || trimmed.startsWith("switch") ||
//         trimmed.startsWith("else") || trimmed.startsWith("do")) {
//         return false;
//     }

//     // 排除变量声明（包含分号但不包含括号的行）
//     if (trimmed.endsWith(";") && !trimmed.contains("(")) {
//         return false;
//     }

//     // 排除函数声明（以分号结尾）
//     if (trimmed.endsWith(";")) {
//         return false;
//     }

//     // 检查是否有函数体（大括号）
//     if (line.contains("{")) {
//         return true;
//     }

//     // 检查接下来的更多行是否有开括号（扩展搜索范围）
//     for (int i = 1; i <= 10 && (lineNum + i) < allLines.size(); i++) {
//         QString nextLine = allLines[lineNum + i].trimmed();
//         if (nextLine.startsWith("{")) {
//             return true;
//         }
//         // 如果遇到其他函数定义或类定义，停止搜索
//         if (nextLine.contains("(") && nextLine.contains(")") &&
//         !nextLine.startsWith("//")) {
//             break;
//         }
//         // 如果遇到分号，可能是函数声明，停止搜索
//         if (nextLine.endsWith(";")) {
//             break;
//         }
//     }

//     return false;
// }

bool CodeEditor::isValidFunctionDefinition(const QString &line, int lineNum,
                                           const QStringList &allLines) {
  QString trimmed = line.trimmed();

  // 排除预处理指令（除了函数式宏）
  if (trimmed.startsWith("#") && !trimmed.startsWith("#define")) {
    return false;
  }

  // 排除明显的控制结构
  if (trimmed.startsWith("if") || trimmed.startsWith("for") ||
      trimmed.startsWith("while") || trimmed.startsWith("switch") ||
      trimmed.startsWith("else") || trimmed.startsWith("do")) {
    return false;
  }

  // 排除变量声明（包含分号但不包含括号的行）
  if (trimmed.endsWith(";") && !trimmed.contains("(")) {
    return false;
  }

  // 排除函数声明（以分号结尾）
  if (trimmed.endsWith(";")) {
    return false;
  }

  // 检查是否有函数体（大括号）
  if (line.contains("{")) {
    return true;
  }

  // 扩展搜索范围到20行，并改进搜索逻辑
  for (int i = 1; i <= 20 && (lineNum + i) < allLines.size(); i++) {
    QString nextLine = allLines[lineNum + i].trimmed();

    // 跳过空行和注释
    if (nextLine.isEmpty() || nextLine.startsWith("//") ||
        nextLine.startsWith("/*")) {
      continue;
    }

    if (nextLine.startsWith("{")) {
      return true;
    }

    // 如果遇到明显的新函数定义，停止搜索
    if (nextLine.contains("{") &&
        (nextLine.contains("class") || nextLine.contains("struct"))) {
      break;
    }

    // 如果遇到分号且不是在括号内，可能是函数声明
    if (nextLine.endsWith(";") && !nextLine.contains("(")) {
      break;
    }

    // 如果遇到另一个函数定义的开始（包含返回类型和函数名模式）
    if (i > 5 && nextLine.contains("(") && nextLine.contains(")") &&
        !nextLine.startsWith("if") && !nextLine.startsWith("for") &&
        !nextLine.startsWith("while") && !nextLine.startsWith("switch")) {
      // 检查是否像函数定义
      QRegularExpression funcPattern(R"(\b\w+\s*\([^)]*\))");
      if (funcPattern.match(nextLine).hasMatch()) {
        break;
      }
    }
  }

  return false;
}

void CodeEditor::highlightCurrentLineNumber() {
  // Determine which editor sent the signal or use current
  QsciScintilla *editor = qobject_cast<QsciScintilla *>(sender());
  if (!editor)
    editor = m_currentEditor;
  if (!editor)
    return;

  int currentLine, currentIndex;
  editor->getCursorPosition(&currentLine, &currentIndex);

  if (currentLine != m_previousLine) {
    // Define marker 25 for current line highlight
    // This marker will show in the line number margin area
    const int CURRENT_LINE_MARKER = 25;

    // Remove marker from previous line
    if (m_previousLine >= 0) {
      editor->markerDelete(m_previousLine, CURRENT_LINE_MARKER);
    }

    // Add marker to current line
    editor->markerAdd(currentLine, CURRENT_LINE_MARKER);

    m_previousLine = currentLine;
  }
}

void CodeEditor::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  if (m_findReplaceBar && m_findReplaceBar->isVisible()) {
    static_cast<FindReplaceWidget *>(m_findReplaceBar)->updatePosition();
  }
  updateStickyScroll();
}

bool CodeEditor::eventFilter(QObject *watched, QEvent *event) {
  if (m_currentEditor && (watched == m_currentEditor || watched == m_currentEditor->viewport())) {
    if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
      updateStickyScroll();
    }
  }

  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->matches(QKeySequence::Find) ||
        (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_F)) {
      showFindReplaceBar(false);
      return true;
    }
    if (keyEvent->matches(QKeySequence::Replace) ||
        (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_H)) {
      showFindReplaceBar(true);
      return true;
    }
    if (keyEvent->key() == Qt::Key_F12 && keyEvent->modifiers() == Qt::NoModifier) {
      gotoDefinitionAtCursor();
      return true;
    }
    if (keyEvent->key() == Qt::Key_F12 && keyEvent->modifiers() == Qt::ShiftModifier) {
      findReferencesAtCursor();
      return true;
    }
    if (keyEvent->key() == Qt::Key_F2 && keyEvent->modifiers() == Qt::NoModifier) {
      renameSymbolAtCursor();
      return true;
    }
    if (keyEvent->key() == Qt::Key_F3) {
      if (m_findReplaceBar) {
        if (!m_findReplaceBar->isVisible()) showFindReplaceBar(false);
        static_cast<FindReplaceWidget *>(m_findReplaceBar)->findText(!(keyEvent->modifiers() & Qt::ShiftModifier));
        return true;
      }
    }
    if (keyEvent->key() == Qt::Key_Escape) {
      if (m_findReplaceBar && m_findReplaceBar->isVisible()) {
        hideFindReplaceBar();
        return true;
      }
    }
  } else if (event->type() == QEvent::MouseButtonRelease) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if (mouseEvent->button() == Qt::LeftButton && (mouseEvent->modifiers() & Qt::ControlModifier)) {
      if (m_currentEditor) {
        int pos = m_currentEditor->SendScintilla(QsciScintilla::SCI_CHARPOSITIONFROMPOINTCLOSE, mouseEvent->pos().x(), mouseEvent->pos().y());
        if (pos >= 0) {
          int line = m_currentEditor->SendScintilla(QsciScintilla::SCI_LINEFROMPOSITION, pos);
          int lineStart = m_currentEditor->SendScintilla(QsciScintilla::SCI_POSITIONFROMLINE, line);
          int col = pos - lineStart;
          m_currentEditor->setCursorPosition(line, col);
        }
      }
      gotoDefinitionAtCursor();
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

void CodeEditor::showFindReplaceBar(bool showReplace) {
  if (!m_findReplaceBar) return;
  QString selText = m_currentEditor ? m_currentEditor->selectedText().trimmed() : QString();
  if (selText.isEmpty() && m_currentEditor) {
    int line, col;
    m_currentEditor->getCursorPosition(&line, &col);
    selText = m_currentEditor->wordAtLineIndex(line, col);
  }
  if (showReplace) {
    static_cast<FindReplaceWidget *>(m_findReplaceBar)->showReplace(selText);
  } else {
    static_cast<FindReplaceWidget *>(m_findReplaceBar)->showFind(selText);
  }
}

void CodeEditor::hideFindReplaceBar() {
  if (m_findReplaceBar) {
    m_findReplaceBar->hide();
    if (m_currentEditor) m_currentEditor->setFocus();
  }
}

QString CodeEditor::projectRootPath() const {
  if (!m_projectRootPath.isEmpty() && QDir(m_projectRootPath).exists()) {
    return m_projectRootPath;
  }
  if (!m_currentFilePath.isEmpty()) {
    QDir dir = QFileInfo(m_currentFilePath).dir();
    QDir searchDir = dir;
    for (int i = 0; i < 5; ++i) {
      if (!searchDir.entryList(QStringList() << "*.pro" << "CMakeLists.txt" << "Makefile", QDir::Files).isEmpty()) {
        return searchDir.absolutePath();
      }
      if (searchDir.exists(".git") || searchDir.exists(".svn")) {
        return searchDir.absolutePath();
      }
      if (!searchDir.cdUp()) break;
    }
    return dir.absolutePath();
  }
  return QDir::currentPath();
}

QStringList CodeEditor::getAllProjectSourceFiles() const {
  QSet<QString> fileSet;
  QStringList roots;
  if (!m_projectRootPath.isEmpty() && QDir(m_projectRootPath).exists()) {
    roots.append(m_projectRootPath);
  }
  if (!m_currentFilePath.isEmpty()) {
    QString curDir = QFileInfo(m_currentFilePath).dir().absolutePath();
    if (!roots.contains(curDir)) roots.append(curDir);
  }
  QString pRoot = projectRootPath();
  if (!roots.contains(pRoot)) roots.append(pRoot);

  for (const QString &r : roots) {
    QDirIterator it(r, QStringList() << "*.c" << "*.cpp" << "*.cxx" << "*.cc" << "*.h" << "*.hpp" << "*.hxx",
                    QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
      QString filePath = it.next();
      // 排除 build, release, debug, .git 目录
      if (filePath.contains("/build/") || filePath.contains("\\build\\") ||
          filePath.contains("/release/") || filePath.contains("\\release\\") ||
          filePath.contains("/debug/") || filePath.contains("\\debug\\") ||
          filePath.contains("/.git/") || filePath.contains("\\.git\\")) {
        continue;
      }
      fileSet.insert(QDir::cleanPath(filePath));
    }
  }
  return fileSet.values();
}

QString CodeEditor::getIdentifierAt(QsciScintilla *editor, int line, int col) {
  if (!editor) return QString();
  if (editor->hasSelectedText()) {
    QString sel = editor->selectedText().trimmed();
    QRegularExpression idRe(R"(^[~]?[A-Za-z_]\w*$)");
    if (idRe.match(sel).hasMatch()) {
      return sel;
    }
  }

  QString lineText = editor->text(line);
  if (lineText.isEmpty()) return QString();

  int len = lineText.length();
  if (col < 0) col = 0;
  if (col > len) col = len;

  int idx = col;
  // 若光标位于非标识符字符处，但前一个字符是标识符，则回退 1 位
  if (idx >= len || (!lineText[idx].isLetterOrNumber() && lineText[idx] != '_' && lineText[idx] != '~')) {
    if (idx > 0 && (lineText[idx - 1].isLetterOrNumber() || lineText[idx - 1] == '_' || lineText[idx - 1] == '~')) {
      idx = col - 1;
    }
  }

  if (idx < 0 || idx >= len) return QString();
  if (!lineText[idx].isLetterOrNumber() && lineText[idx] != '_' && lineText[idx] != '~') {
    return QString();
  }

  int start = idx;
  while (start > 0 && (lineText[start - 1].isLetterOrNumber() || lineText[start - 1] == '_' || lineText[start - 1] == '~')) {
    --start;
  }
  int end = idx;
  while (end < len && (lineText[end].isLetterOrNumber() || lineText[end] == '_')) {
    ++end;
  }

  QString sym = lineText.mid(start, end - start).trimmed();
  QRegularExpression idRe(R"(^[~]?[A-Za-z_]\w*$)");
  if (idRe.match(sym).hasMatch()) {
    return sym;
  }
  return QString();
}

QString CodeEditor::extractEnclosingClassName(int line, QsciScintilla *editor) const {
  if (!editor) editor = m_currentEditor;
  if (!editor) return QString();

  // 1. 首先检查当前行所属函数在 m_functions 中记录的 parentClass
  for (const FunctionInfo &fn : m_functions) {
    if (line >= fn.startLine && line <= fn.endLine && !fn.parentClass.isEmpty()) {
      return fn.parentClass;
    }
  }

  // 2. 向上扫描是否包含 ClassName::Method 形式的函数签名
  QRegularExpression methodHeaderRe(R"(\b([A-Za-z_]\w*)\s*::\s*~?[A-Za-z_]\w*\s*\()");
  for (int l = line; l >= 0; --l) {
    QString raw = editor->text(l);
    QRegularExpressionMatch mm = methodHeaderRe.match(raw);
    if (mm.hasMatch()) {
      QString cls = mm.captured(1).trimmed();
      if (!cls.isEmpty()) return cls;
    }
    // 3. 向上扫描是否在 class/struct 定义块内部
    QRegularExpression classDefRe(R"(\b(?:class|struct)\s+(?:[A-Za-z_]\w*_EXPORT\s+|Q_DECL_EXPORT\s+)?([A-Za-z_]\w*)\b\s*(?::\s*[^{;]*)?\{?)");
    QRegularExpressionMatch m = classDefRe.match(raw);
    if (m.hasMatch()) {
      QString clsName = m.captured(1).trimmed();
      if (!clsName.isEmpty() && clsName != "struct" && clsName != "class") {
        return clsName;
      }
    }
  }
  return QString();
}

CodeEditor::SymbolScopeResult CodeEditor::analyzeSymbolScopeAt(int line, int col, QsciScintilla *editor) {
  if (!editor) editor = m_currentEditor;
  if (!editor) return SymbolScopeResult();

  // 确保 Scintilla 样式已着色至当前行
  editor->SendScintilla(QsciScintilla::SCI_COLOURISE, 0, -1);

  // 若光标（或选区起点）位于注释或字符串中，则不进行同名高亮
  int pos = editor->SendScintilla(QsciScintilla::SCI_GETCURRENTPOS);
  if (editor->hasSelectedText()) {
    pos = editor->SendScintilla(QsciScintilla::SCI_GETSELECTIONSTART);
  } else if (pos > 0) {
    // 若光标停在词尾，回退探测前一个字符的样式
    int prevStyle = editor->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, pos - 1) & 0x3F;
    int curStyle = editor->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, pos) & 0x3F;
    if (curStyle == 0 && prevStyle != 0) pos = pos - 1;
  }
  int style = editor->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, pos) & 0x3F;
  bool isCommentOrStr = (
      style == QsciLexerCPP::Comment ||
      style == QsciLexerCPP::CommentLine ||
      style == QsciLexerCPP::CommentDoc ||
      style == QsciLexerCPP::DoubleQuotedString ||
      style == QsciLexerCPP::SingleQuotedString ||
      style == 12 || // StringEOL
      style == 13 || // VerbatimString
      style == 20 || // RawString
      style == 22    // HashQuotedString
  );
  if (isCommentOrStr) {
    return SymbolScopeResult();
  }

  QString sym = getIdentifierAt(editor, line, col);
  if (sym.isEmpty()) return SymbolScopeResult();

  // 未手动选中文本时，过滤常见流程控制关键字与基础语法修饰词
  if (!editor->hasSelectedText()) {
    static const QSet<QString> flowKeywords = {
        "if", "else", "for", "while", "do", "switch", "case", "default", "break",
        "continue", "return", "goto", "try", "catch", "throw", "sizeof", "decltype",
        "typeid", "new", "delete", "nullptr", "true", "false", "this", "public",
        "protected", "private", "signals", "slots", "emit", "class", "struct", "enum", "union"
    };
    if (flowKeywords.contains(sym)) return SymbolScopeResult();
  }

  QString code = editor->text();
  QString clean = generateCleanCppCode(code);
  const int codeLen = code.length();

  // 极速构建行起始偏移索引表，以 O(log L) 毫秒级换算行号与列号，彻底消除大文件 O(N^2) 内存切片与卡顿
  QVector<int> lineStarts;
  lineStarts.reserve(qMax(64, editor->lines() + 10));
  lineStarts.append(0);
  for (int i = 0; i < codeLen; ++i) {
    if (code[i] == '\n') lineStarts.append(i + 1);
  }

  auto getLineAndColFromOffset = [&](int absOffset, int &outLine, int &outCol, QString &outLineContent) {
    if (absOffset < 0 || absOffset >= codeLen) {
      outLine = 0; outCol = 0; outLineContent.clear();
      return;
    }
    auto it = std::upper_bound(lineStarts.begin(), lineStarts.end(), absOffset);
    outLine = (it - lineStarts.begin()) - 1;
    int lineStart = lineStarts[outLine];
    outCol = absOffset - lineStart;
    int lineEnd = (outLine + 1 < lineStarts.size()) ? lineStarts[outLine + 1] - 1 : codeLen;
    outLineContent = code.mid(lineStart, lineEnd - lineStart);
  };

  SymbolScopeResult result;
  result.symbolName = sym;
  result.definitionFilePath = m_currentFilePath;

  // 判断光标当前是否位于某个函数体内
  FunctionInfo enclosingFunc;
  bool inFunc = false;
  for (const FunctionInfo &fn : m_functions) {
    if (fn.type == SymbolFunction && line >= fn.startLine && line <= fn.endLine) {
      enclosingFunc = fn;
      inFunc = true;
      break;
    }
  }

  QString escSym = QRegularExpression::escape(sym);

  // 若为函数体内局部变量或形参判定
  if (inFunc) {
    int funcStartOffset = (enclosingFunc.startLine < lineStarts.size()) ? lineStarts[enclosingFunc.startLine] : 0;
    int funcEndOffset = (enclosingFunc.endLine + 1 < lineStarts.size()) ? lineStarts[enclosingFunc.endLine + 1] : codeLen;
    funcStartOffset = qBound(0, funcStartOffset, codeLen);
    funcEndOffset = qBound(funcStartOffset, funcEndOffset, codeLen);

    QString funcClean = clean.mid(funcStartOffset, funcEndOffset - funcStartOffset);

    int openBrace = funcClean.indexOf('{');
    QString headerPart = openBrace != -1 ? funcClean.left(openBrace) : funcClean;
    QRegularExpression paramRe(QString(R"(\b(?:const\s+)?([A-Za-z_]\w*(?:\s*<[^>]*>)?(?:\s*::\s*[A-Za-z_]\w*)*)(?:\s+|\s*[*&]+\s*)(%1)\b)").arg(escSym));
    QRegularExpressionMatch pMatch = paramRe.match(headerPart);

    QRegularExpression localRe(QString(R"((?<![.\->\w])(?:const\s+|static\s+|volatile\s+|auto\s+|unsigned\s+|signed\s+|struct\s+|enum\s+)*([A-Za-z_]\w*(?:\s*<[^>]*>)?(?:\s*::\s*[A-Za-z_]\w*)*)(?:\s+|\s*[*&]+\s*)(%1)\s*(?:[;=,\[\(\{]))").arg(escSym));
    QRegularExpressionMatch lMatch;
    if (openBrace != -1) {
      QString bodyClean = funcClean.mid(openBrace);
      QRegularExpressionMatchIterator iter = localRe.globalMatch(bodyClean);
      while (iter.hasNext()) {
        QRegularExpressionMatch m = iter.next();
        QString typeName = m.captured(1).trimmed();
        static const QSet<QString> nonTypes = {"return", "connect", "disconnect", "emit", "case", "sizeof", "new", "delete", "throw", "goto", "qobject_cast", "static_cast", "reinterpret_cast", "dynamic_cast", "SIGNAL", "SLOT"};
        if (!nonTypes.contains(typeName)) {
          lMatch = m;
          break;
        }
      }
    }

    if (pMatch.hasMatch() || lMatch.hasMatch()) {
      result.kind = pMatch.hasMatch() ? ScopeFunctionParam : ScopeLocalVariable;
      result.scopeOwner = enclosingFunc.scopedName;
      result.scopeStartLine = enclosingFunc.startLine;
      result.scopeEndLine = enclosingFunc.endLine;

      int declOffsetInFunc = pMatch.hasMatch() ? pMatch.capturedStart(2) : (openBrace + lMatch.capturedStart(2));
      int absDeclOffset = funcStartOffset + declOffsetInFunc;
      QString dummy;
      getLineAndColFromOffset(absDeclOffset, result.definitionLine, result.definitionCol, dummy);

      // 局部变量：精准高亮本函数体内的所有引用项
      QString localPattern = sym.startsWith('~') ?
          QString(R"(~(?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(QRegularExpression::escape(sym.mid(1))) :
          QString(R"((?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(escSym);
      QRegularExpression symRe(localPattern);
      QRegularExpressionMatchIterator iter = symRe.globalMatch(funcClean);
      while (iter.hasNext()) {
        QRegularExpressionMatch m = iter.next();
        int absOffset = funcStartOffset + m.capturedStart();
        SymbolOccurrence occ;
        occ.startOffset = absOffset;
        occ.endOffset = absOffset + sym.length();
        occ.length = sym.length();
        getLineAndColFromOffset(absOffset, occ.line, occ.col, occ.lineContent);
        result.occurrences.append(occ);
      }
      result.isValid = !result.occurrences.isEmpty();
      return result;
    }
  }

  // 非局部变量（全局变量/类成员/方法名/类型/宏/Qt关键字）：全文件匹配
  result.kind = ScopeGlobalVariable;
  for (const FunctionInfo &fn : m_functions) {
    if (fn.scopedName == sym) {
      if (fn.type == SymbolFunction) result.kind = ScopeFunction;
      else if (fn.type == SymbolVariable) result.kind = fn.indentLevel > 0 ? ScopeMemberVariable : ScopeGlobalVariable;
      else if (fn.type == SymbolClass || fn.type == SymbolStruct || fn.type == SymbolEnum || fn.type == SymbolUnion) result.kind = ScopeType;
      else if (fn.type == SymbolMacro) result.kind = ScopeMacro;
      result.definitionLine = fn.startLine;
      result.definitionCol = fn.startCol;
      result.scopeOwner = fn.parentClass.isEmpty() ? "全局作用域" : fn.parentClass;
      break;
    }
  }

  if (result.definitionLine == -1) {
    result.scopeOwner = "全局作用域";
    result.scopeStartLine = 0;
    result.scopeEndLine = editor->lines() - 1;
  }

  QString symPattern = sym.startsWith('~') ?
      QString(R"(~(?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(QRegularExpression::escape(sym.mid(1))) :
      QString(R"((?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(escSym);
  QRegularExpression globalSymRe(symPattern);
  QRegularExpressionMatchIterator iter = globalSymRe.globalMatch(clean);

  while (iter.hasNext()) {
    QRegularExpressionMatch m = iter.next();
    int absOffset = m.capturedStart();

    SymbolOccurrence occ;
    occ.startOffset = absOffset;
    occ.endOffset = absOffset + sym.length();
    occ.length = sym.length();
    getLineAndColFromOffset(absOffset, occ.line, occ.col, occ.lineContent);
    result.occurrences.append(occ);

    if (result.definitionLine == -1) {
      result.definitionLine = occ.line;
      result.definitionCol = occ.col;
    }
  }

  result.isValid = !result.occurrences.isEmpty();
  return result;
}

void CodeEditor::clearOccurrenceHighlights(QsciScintilla *editor) {
  if (!editor) editor = m_currentEditor;
  if (!editor) return;
  int docLen = editor->SendScintilla(QsciScintilla::SCI_GETTEXTLENGTH);
  if (docLen <= 0) return;
  editor->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, OCCURRENCE_INDICATOR);
  editor->SendScintilla(QsciScintilla::SCI_INDICATORCLEARRANGE, 0, docLen);
}

void CodeEditor::highlightOccurrences(const SymbolScopeResult &scopeResult, QsciScintilla *editor) {
  if (!editor) editor = m_currentEditor;
  if (!editor) return;

  clearOccurrenceHighlights(editor);
  if (!scopeResult.isValid || scopeResult.occurrences.isEmpty()) return;

  // 1:1 对标 Qt Creator / VS Code 现代圆角矩形指示器渲染
  editor->indicatorDefine(QsciScintilla::RoundBoxIndicator, OCCURRENCE_INDICATOR);
  editor->setIndicatorForegroundColor(QColor(m_isDarkTheme ? "#89B4FA" : "#2563EB"), OCCURRENCE_INDICATOR);
  editor->setIndicatorDrawUnder(false, OCCURRENCE_INDICATOR);

  // 配置半透明背景填充 (alpha: 75/70) 与清晰圆角轮廓描边 (outline alpha: 255)
  editor->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, (unsigned long)OCCURRENCE_INDICATOR, (long)(m_isDarkTheme ? 75 : 70));
  editor->SendScintilla(QsciScintilla::SCI_INDICSETOUTLINEALPHA, (unsigned long)OCCURRENCE_INDICATOR, (long)255);

  for (const SymbolOccurrence &occ : scopeResult.occurrences) {
    editor->fillIndicatorRange(occ.line, occ.col, occ.line, occ.col + occ.length, OCCURRENCE_INDICATOR);
  }
}

void CodeEditor::onOccurrenceTimerTimeout() {
  if (!m_currentEditor) return;
  int line, col;
  m_currentEditor->getCursorPosition(&line, &col);
  SymbolScopeResult res = analyzeSymbolScopeAt(line, col, m_currentEditor);
  if (res.isValid && !res.occurrences.isEmpty()) {
    highlightOccurrences(res, m_currentEditor);
  } else {
    clearOccurrenceHighlights(m_currentEditor);
  }
}

QList<CodeEditor::DefinitionCandidate> CodeEditor::findSymbolDefinitionsInProject(const QString &symbolName, const QString &enclosingClass) {
  QList<DefinitionCandidate> candidates;
  if (symbolName.isEmpty()) return candidates;

  QString escSym = QRegularExpression::escape(symbolName);
  QString currentPath = m_currentFilePath;
  bool isCurrentHeader = currentPath.endsWith(".h", Qt::CaseInsensitive) || currentPath.endsWith(".hpp", Qt::CaseInsensitive);

  // 规则 1: 类方法跨行签名实现 (例如: ReturnType ClassName::Method(...) { 或 ClassName::~ClassName() {)
  QRegularExpression methodImplMultiRe(QString(R"(\b([A-Za-z_]\w*)\s*::\s*(~?%1)\s*\()").arg(escSym));

  // 规则 2: 普通/全局/内联/静态函数实现带大括号
  QRegularExpression funcImplRe(QString(R"((?:^|[^\w:.])(?<!class\s)(?<!struct\s)(?<!enum\s)([A-Za-z_]\w*(?:\s*<[^>]*>)?(?:\s*::\s*[A-Za-z_]\w*)*\s+)?(?<!::)\b(%1)\s*\()").arg(escSym));

  // 规则 3: 类 / 结构体 / 枚举 / 类型定义
  QRegularExpression typeDeclRe(QString(R"(\b(?:class|struct|union|enum(?:\s+class|\s+struct)?)\s+(?:[A-Za-z_]\w*_EXPORT\s+|Q_DECL_EXPORT\s+)?(%1)\b\s*[:{])").arg(escSym));
  QRegularExpression typedefRe(QString(R"(\btypedef\s+.*\b(%1)\s*;)").arg(escSym));
  QRegularExpression usingRe(QString(R"(\busing\s+(%1)\s*=)").arg(escSym));

  // 规则 4: 宏定义
  QRegularExpression macroDefRe(QString(R"(^[ \t]*#[ \t]*define[ \t]+(%1)\b)").arg(escSym), QRegularExpression::MultilineOption);

  // 规则 5: 全局变量 / 静态变量 / 成员定义
  QRegularExpression globalVarRe(QString(R"(\b(?:[A-Za-z_]\w*(?:\s*<[^>]*>)?(?:\s*::\s*[A-Za-z_]\w*)*)\s+[*&]*\s*(%1)\s*(?:[;=,\[\)]))").arg(escSym));

  // 规则 6: 函数声明 (头文件中)
  QRegularExpression funcDeclRe(QString(R"(\b(%1)\s*\()").arg(escSym));
  QRegularExpression signalDeclRe(QString(R"(signals:\s*void\s+(%1)\s*\()").arg(escSym));

  // 收集工程文件并按最高优先级排序
  QStringList projectFiles = getAllProjectSourceFiles();
  QStringList prioritizedFiles;

  // 优先级 1: 寻找配对文件 (如 foo.h -> foo.cpp / ../src/foo.cpp, foo.cpp -> foo.h / ../inc/foo.h)
  if (!currentPath.isEmpty()) {
    QFileInfo fi(currentPath);
    QString base = fi.baseName();

    QStringList pairedExtensions;
    if (isCurrentHeader) {
      pairedExtensions << ".cpp" << ".c" << ".cc" << ".cxx";
    } else {
      pairedExtensions << ".h" << ".hpp" << ".hxx" << ".inl";
    }

    // 同目录配对
    for (const QString &ext : pairedExtensions) {
      QString paired = fi.dir().filePath(base + ext);
      if (QFile::exists(paired)) {
        prioritizedFiles.append(QDir::cleanPath(paired));
      }
    }

    // 兄弟 src/ 或 inc/ 目录配对
    QStringList siblingDirs = {
        fi.dir().filePath("../src"), fi.dir().filePath("../Src"), fi.dir().filePath("../source"),
        fi.dir().filePath("../inc"), fi.dir().filePath("../Inc"), fi.dir().filePath("../include"),
        fi.dir().filePath("../../Src"), fi.dir().filePath("../../Inc")
    };
    for (const QString &sDir : siblingDirs) {
      for (const QString &ext : pairedExtensions) {
        QString paired = QDir(sDir).filePath(base + ext);
        if (QFile::exists(paired)) {
          QString cleanPaired = QDir::cleanPath(paired);
          if (!prioritizedFiles.contains(cleanPaired)) {
            prioritizedFiles.append(cleanPaired);
          }
        }
      }
    }
  }

  // 优先级 2: 当前文件
  if (!currentPath.isEmpty()) {
    QString cleanCur = QDir::cleanPath(currentPath);
    if (!prioritizedFiles.contains(cleanCur)) {
      prioritizedFiles.append(cleanCur);
    }
  }

  // 优先级 3: 其他所有工程源码文件
  for (const QString &fPath : projectFiles) {
    QString cleanP = QDir::cleanPath(fPath);
    if (!prioritizedFiles.contains(cleanP)) {
      prioritizedFiles.append(cleanP);
    }
  }

  QSet<QString> visitedLocations;

  for (const QString &fPath : prioritizedFiles) {
    QString fCode;
    if (fPath == currentPath && m_currentEditor) {
      fCode = m_currentEditor->text();
    } else {
      QFile file(fPath);
      if (!file.open(QIODevice::ReadOnly)) continue;
      QByteArray data = file.readAll();
      file.close();
      QTextCodec::ConverterState state;
      QTextCodec *codec = QTextCodec::codecForName("UTF-8");
      fCode = codec->toUnicode(data.constData(), data.size(), &state);
      if (state.invalidChars > 0) {
        fCode = QTextCodec::codecForLocale()->toUnicode(data);
      }
    }

    if (fCode.isEmpty()) continue;
    QString cleanCode = generateCleanCppCode(fCode);
    bool isHeader = fPath.endsWith(".h", Qt::CaseInsensitive) || fPath.endsWith(".hpp", Qt::CaseInsensitive);

    // 建立快速行索引表
    QVector<int> fLineStarts;
    fLineStarts.reserve(500);
    fLineStarts.append(0);
    for (int i = 0; i < fCode.length(); ++i) {
      if (fCode[i] == '\n') fLineStarts.append(i + 1);
    }

    auto addCandidate = [&](int symStartOffset, const QString &typeLabel, int baseScore) {
      if (symStartOffset < 0 || symStartOffset >= fCode.length()) return;
      int mLine = std::upper_bound(fLineStarts.begin(), fLineStarts.end(), symStartOffset) - fLineStarts.begin() - 1;
      int lStart = fLineStarts[mLine];
      int mCol = symStartOffset - lStart;
      int lEnd = (mLine + 1 < fLineStarts.size()) ? fLineStarts[mLine + 1] - 1 : fCode.length();
      QString preview = fCode.mid(lStart, lEnd - lStart).trimmed();

      QString locKey = QString("%1:%2").arg(fPath).arg(mLine);
      if (visitedLocations.contains(locKey)) return;
      visitedLocations.insert(locKey);

      DefinitionCandidate cand;
      cand.filePath = fPath;
      cand.line = mLine;
      cand.col = mCol;
      cand.length = symbolName.length();
      cand.preview = preview;
      cand.type = typeLabel;
      cand.score = baseScore;

      // 评分微调权重
      if (isCurrentHeader && !isHeader) {
        cand.score += 200; // 头文件查源文件实现极大加分
      }
      if (!isCurrentHeader && isHeader) {
        cand.score += 100; // 源文件查头文件声明加分
      }
      if (!prioritizedFiles.isEmpty() && fPath == prioritizedFiles.first()) {
        cand.score += 150; // 直系配对文件极大加分
      }

      candidates.append(cand);
    };

    // 扫描规则 1: 类方法实现
    QRegularExpressionMatchIterator mIter = methodImplMultiRe.globalMatch(cleanCode);
    while (mIter.hasNext()) {
      QRegularExpressionMatch m = mIter.next();
      QString clsName = m.captured(1).trimmed();
      int matchStart = m.capturedStart();
      int symOffset = m.capturedStart(2);

      int searchLimit = qMin(cleanCode.length(), matchStart + 600);
      int openBrace = cleanCode.indexOf('{', matchStart);
      int semiColon = cleanCode.indexOf(';', matchStart);

      if (openBrace != -1 && openBrace < searchLimit && (semiColon == -1 || openBrace < semiColon)) {
        if (symOffset != -1) {
          int score = 900;
          QString label = "类方法实现";
          if (!enclosingClass.isEmpty() && clsName.compare(enclosingClass, Qt::CaseInsensitive) == 0) {
            score = 1000;
            label = "目标类方法实现";
          }
          if (symbolName.startsWith('~') || clsName == symbolName) {
            label = "构造/析构函数";
          }
          addCandidate(symOffset, label, score);
        }
      }
    }

    // 扫描规则 2: 普通/全局/内联/静态函数实现 (无论 .cpp 还是 .h 均支持)
    QRegularExpressionMatchIterator fIter = funcImplRe.globalMatch(cleanCode);
    while (fIter.hasNext()) {
      QRegularExpressionMatch m = fIter.next();
      int symOffset = m.capturedStart(2);
      if (symOffset == -1) symOffset = m.capturedStart(1);
      int openParen = cleanCode.indexOf('(', m.capturedStart());
      if (openParen != -1) {
        int searchLimit = qMin(cleanCode.length(), openParen + 600);
        int openBrace = cleanCode.indexOf('{', openParen);
        int semiColon = cleanCode.indexOf(';', openParen);
        if (openBrace != -1 && openBrace < searchLimit && (semiColon == -1 || openBrace < semiColon)) {
          if (symOffset != -1) {
            addCandidate(symOffset, isHeader ? "头文件内联实现" : "函数实现", isHeader ? 860 : 880);
          }
        }
      }
    }

    // 扫描规则 3: 类型定义 (class, struct, enum, typedef, using)
    QRegularExpressionMatchIterator tIter = typeDeclRe.globalMatch(cleanCode);
    while (tIter.hasNext()) {
      QRegularExpressionMatch m = tIter.next();
      int symOffset = m.capturedStart(1);
      if (symOffset != -1) {
        addCandidate(symOffset, "类型定义", 920);
      }
    }
    QRegularExpressionMatchIterator tdIter = typedefRe.globalMatch(cleanCode);
    while (tdIter.hasNext()) {
      QRegularExpressionMatch m = tdIter.next();
      int symOffset = m.capturedStart(1);
      if (symOffset != -1) {
        addCandidate(symOffset, "类型别名", 910);
      }
    }
    QRegularExpressionMatchIterator uIter = usingRe.globalMatch(cleanCode);
    while (uIter.hasNext()) {
      QRegularExpressionMatch m = uIter.next();
      int symOffset = m.capturedStart(1);
      if (symOffset != -1) {
        addCandidate(symOffset, "类型别名", 910);
      }
    }

    // 扫描规则 4: 宏定义 (#define FOO ...)
    QRegularExpressionMatchIterator macIter = macroDefRe.globalMatch(cleanCode);
    while (macIter.hasNext()) {
      QRegularExpressionMatch m = macIter.next();
      int symOffset = m.capturedStart(1);
      if (symOffset != -1) {
        addCandidate(symOffset, "宏定义", 950);
      }
    }

    // 扫描规则 5: 变量/成员定义
    QRegularExpressionMatchIterator varIter = globalVarRe.globalMatch(cleanCode);
    while (varIter.hasNext()) {
      QRegularExpressionMatch m = varIter.next();
      int symOffset = m.capturedStart(1);
      if (symOffset != -1) {
        addCandidate(symOffset, "变量定义", 750);
      }
    }

    // 扫描规则 6: 函数声明 (头文件中)
    if (isHeader) {
      QRegularExpressionMatchIterator declIter = funcDeclRe.globalMatch(cleanCode);
      while (declIter.hasNext()) {
        QRegularExpressionMatch m = declIter.next();
        int symOffset = m.capturedStart(1);
        int openParen = cleanCode.indexOf('(', m.capturedStart());
        if (openParen != -1) {
          int searchLimit = qMin(cleanCode.length(), openParen + 400);
          int semiColon = cleanCode.indexOf(';', openParen);
          int openBrace = cleanCode.indexOf('{', openParen);
          if (semiColon != -1 && semiColon < searchLimit && (openBrace == -1 || semiColon < openBrace)) {
            if (symOffset != -1) {
              addCandidate(symOffset, "函数声明", 650);
            }
          }
        }
      }
      QRegularExpressionMatchIterator sigIter = signalDeclRe.globalMatch(cleanCode);
      while (sigIter.hasNext()) {
        QRegularExpressionMatch m = sigIter.next();
        int symOffset = m.capturedStart(1);
        if (symOffset != -1) {
          addCandidate(symOffset, "信号声明", 650);
        }
      }
    }
  }

  // 排序：按 score 从高到低
  std::sort(candidates.begin(), candidates.end(), [](const DefinitionCandidate &a, const DefinitionCandidate &b) {
    return a.score > b.score;
  });

  return candidates;
}

void CodeEditor::openDefinitionCandidate(const DefinitionCandidate &cand) {
  if (cand.filePath.isEmpty()) return;
  if (cand.filePath != m_currentFilePath) {
    openFile(cand.filePath);
  }
  if (m_currentEditor) {
    m_currentEditor->setCursorPosition(cand.line, cand.col);
    m_currentEditor->ensureLineVisible(cand.line);
    int visibleLines = m_currentEditor->SendScintilla(QsciScintilla::SCI_LINESONSCREEN);
    int scrollLine = qMax(0, cand.line - visibleLines / 3);
    m_currentEditor->SendScintilla(QsciScintilla::SCI_SETFIRSTVISIBLELINE, scrollLine);
    m_currentEditor->setSelection(cand.line, cand.col, cand.line, cand.col + cand.length);
    m_currentEditor->setFocus();
    updateBreadcrumb(cand.line);
    if (m_occurrenceTimer) m_occurrenceTimer->start();
  }
}

void CodeEditor::gotoDefinitionAtCursor() {
  if (!m_currentEditor) return;
  int line, col;
  m_currentEditor->getCursorPosition(&line, &col);

  QString sym = getIdentifierAt(m_currentEditor, line, col);
  if (sym.isEmpty()) return;

  SymbolScopeResult res = analyzeSymbolScopeAt(line, col, m_currentEditor);

  // 1. 若为当前函数内的局部变量或形参，且定义在当前文件中，直接在当前文件跳转
  if (res.isValid && (res.kind == ScopeLocalVariable || res.kind == ScopeFunctionParam)) {
    if (res.definitionLine != -1) {
      if (res.definitionLine != line || res.definitionCol != col) {
        m_currentEditor->setCursorPosition(res.definitionLine, res.definitionCol);
        m_currentEditor->ensureLineVisible(res.definitionLine);
        int visibleLines = m_currentEditor->SendScintilla(QsciScintilla::SCI_LINESONSCREEN);
        int scrollLine = qMax(0, res.definitionLine - visibleLines / 3);
        m_currentEditor->SendScintilla(QsciScintilla::SCI_SETFIRSTVISIBLELINE, scrollLine);
        m_currentEditor->setSelection(res.definitionLine, res.definitionCol, res.definitionLine, res.definitionCol + sym.length());
        m_currentEditor->setFocus();
        updateBreadcrumb(res.definitionLine);
        if (m_occurrenceTimer) m_occurrenceTimer->start();
      }
      return; // 已经属于当前局部变量/形参定义，无论是否处于定义行自身，都不向全工程盲目穿透
    }
  }

  // 2. 提取外层类作用域
  QString enclosingClass = extractEnclosingClassName(line, m_currentEditor);
  QString curLineText = m_currentEditor->text(line);

  // 检查当前行是否显式包含 ClassName::sym 或 &ClassName::sym
  QRegularExpression explicitClassRe(QString(R"(&?([A-Za-z_]\w*)\s*::\s*%1\b)").arg(QRegularExpression::escape(sym)));
  QRegularExpressionMatch ecMatch = explicitClassRe.match(curLineText);
  if (ecMatch.hasMatch()) {
    enclosingClass = ecMatch.captured(1).trimmed();
  }

  // 3. 全工程搜索候选定义
  QList<DefinitionCandidate> candidates = findSymbolDefinitionsInProject(sym, enclosingClass);

  // 若当前为头文件，过滤掉自身当前行的候选声明，优先跳转到源文件中的实现
  bool isCurrentHeader = m_currentFilePath.endsWith(".h", Qt::CaseInsensitive) || m_currentFilePath.endsWith(".hpp", Qt::CaseInsensitive);
  if (isCurrentHeader) {
    QList<DefinitionCandidate> filtered;
    for (const auto &c : candidates) {
      if (c.filePath == m_currentFilePath && c.line == line) continue; // 排除当前声明行自身
      filtered.append(c);
    }
    if (!filtered.isEmpty()) {
      candidates = filtered;
    }
  }

  if (candidates.isEmpty()) {
    if (res.definitionLine != -1 && (res.definitionLine != line || res.definitionCol != col)) {
      m_currentEditor->setCursorPosition(res.definitionLine, res.definitionCol);
      m_currentEditor->ensureLineVisible(res.definitionLine);
      m_currentEditor->setSelection(res.definitionLine, res.definitionCol, res.definitionLine, res.definitionCol + sym.length());
      m_currentEditor->setFocus();
      updateBreadcrumb(res.definitionLine);
      if (m_occurrenceTimer) m_occurrenceTimer->start();
    }
    return;
  }

  // 4. 智能直跳决策：
  // 若只有 1 个候选，或首个候选为高置信度实现/定义（score >= 950 或领先第二名 >= 100），直接平滑跳转！
  bool shouldAutoJump = false;
  if (candidates.size() == 1) {
    shouldAutoJump = true;
  } else if (candidates.first().score >= 950) {
    shouldAutoJump = true;
  } else if (candidates.size() > 1 && (candidates.first().score - candidates[1].score >= 100)) {
    shouldAutoJump = true;
  } else if (isCurrentHeader && (candidates.first().type.contains("实现") || candidates.first().type.contains("构造"))) {
    shouldAutoJump = true;
  }

  if (shouldAutoJump) {
    openDefinitionCandidate(candidates.first());
    return;
  }

  // 5. 多个同分/近分候选时，弹出精美快速选择对话框
  GoToDefinitionDialog dlg(sym, candidates, this, this);
  dlg.exec();
}

void CodeEditor::findReferencesAtCursor() {
  if (!m_currentEditor) return;
  int line, col;
  m_currentEditor->getCursorPosition(&line, &col);
  SymbolScopeResult res = analyzeSymbolScopeAt(line, col, m_currentEditor);
  if (!res.isValid || res.occurrences.isEmpty()) return;

  ReferencesDialog dlg(res.symbolName, res.occurrences, m_currentFilePath, this, this);
  dlg.exec();
}

void CodeEditor::renameSymbolAtCursor() {
  if (!m_currentEditor) return;
  int line, col;
  m_currentEditor->getCursorPosition(&line, &col);
  SymbolScopeResult res = analyzeSymbolScopeAt(line, col, m_currentEditor);
  if (!res.isValid) return;

  // 1. 如果是局部变量或形参：严格限制在当前函数内部重命名
  if (res.kind == ScopeLocalVariable || res.kind == ScopeFunctionParam) {
    QString scopeDesc = QString("【局部变量】当前函数 \"%1\" 作用域 (不影响其他函数/全局同名变量)").arg(res.scopeOwner);
    RenameSymbolDialog dlg(res.symbolName, scopeDesc, res.occurrences.size(), this);
    if (dlg.exec() == QDialog::Accepted) {
      QString newName = dlg.newName();
      if (!newName.isEmpty() && newName != res.symbolName) {
        renameSymbolInScope(res, newName);
      }
    }
    return;
  }

  // 2. 如果是全局/跨文件符号（全局变量、类成员、函数名、结构体、宏定义等）：执行全工程同作用域批量重命名
  renameSymbolInProject(res.symbolName, res.kind, "");
}

void CodeEditor::renameSymbolInScope(const SymbolScopeResult &scopeResult, const QString &newName) {
  if (!m_currentEditor || scopeResult.occurrences.isEmpty() || newName.isEmpty()) return;

  m_currentEditor->SendScintilla(QsciScintilla::SCI_BEGINUNDOACTION);

  // 从后向前倒序替换，防止前面的替换引起后续行/列坐标偏移
  QList<SymbolOccurrence> sortedOccs = scopeResult.occurrences;
  std::sort(sortedOccs.begin(), sortedOccs.end(), [](const SymbolOccurrence &a, const SymbolOccurrence &b) {
    return a.startOffset > b.startOffset;
  });

  for (const SymbolOccurrence &occ : sortedOccs) {
    m_currentEditor->setSelection(occ.line, occ.col, occ.line, occ.col + occ.length);
    m_currentEditor->replaceSelectedText(newName);
  }

  m_currentEditor->SendScintilla(QsciScintilla::SCI_ENDUNDOACTION);

  // 刷新函数列表与符号高亮
  updateFunctionList();
  if (m_occurrenceTimer) m_occurrenceTimer->start();
}

void CodeEditor::renameSymbolInProject(const QString &symbolName, ScopeKind kind, const QString &newName) {
  Q_UNUSED(newName);
  if (symbolName.isEmpty()) return;

  QString escSym = QRegularExpression::escape(symbolName);
  QString symPattern = symbolName.startsWith('~') ?
      QString(R"(~(?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(QRegularExpression::escape(symbolName.mid(1))) :
      QString(R"((?<![A-Za-z0-9_])%1(?![A-Za-z0-9_]))").arg(escSym);
  QRegularExpression globalSymRe(symPattern);

  QRegularExpression localDeclRe(QString(R"(\b(?:[A-Za-z_]\w*(?:\s*<[^>]*>)?(?:\s*::\s*[A-Za-z_]\w*)*)\s*[*&]*\s*(%1)\s*(?:[;=,\[\)]))").arg(escSym));
  QRegularExpression paramDeclRe(QString(R"(\b(?:const\s+)?([A-Za-z_]\w*(?:\s*<[^>]*>)?(?:\s*::\s*[A-Za-z_]\w*)*)\s*[*&]*\s*(%1)\b)").arg(escSym));

  QStringList projectFiles = getAllProjectSourceFiles();
  QMap<QString, QList<SymbolOccurrence>> fileOccurrences;

  for (const QString &fPath : projectFiles) {
    QString code;
    if (fPath == m_currentFilePath && m_currentEditor) {
      code = m_currentEditor->text();
    } else {
      QFile file(fPath);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
      code = QString::fromUtf8(file.readAll());
    }

    QString clean = generateCleanCppCode(code);
    const int codeLen = code.length();

    // 扫描该文件中的所有函数，识别存在局部同名变量遮蔽的区间
    QList<QPair<int, int>> shadowedRanges;
    QRegularExpression funcRe(R"((?:[A-Za-z_]\w*(?:\s*<[^>]*>)?(?:\s*::\s*[A-Za-z_]\w*)*\s+)?(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*\s*\([^;]*\)\s*(?:const\s*)?\{)");
    QRegularExpressionMatchIterator funcIter = funcRe.globalMatch(clean);
    while (funcIter.hasNext()) {
      QRegularExpressionMatch fm = funcIter.next();
      int bodyStart = fm.capturedEnd() - 1; // '{' 位置
      int braceDepth = 1;
      int bodyEnd = bodyStart + 1;
      while (bodyEnd < codeLen && braceDepth > 0) {
        if (clean[bodyEnd] == '{') ++braceDepth;
        else if (clean[bodyEnd] == '}') --braceDepth;
        ++bodyEnd;
      }
      QString funcBodyClean = clean.mid(fm.capturedStart(), bodyEnd - fm.capturedStart());
      if (paramDeclRe.match(funcBodyClean).hasMatch() || localDeclRe.match(funcBodyClean).hasMatch()) {
        shadowedRanges.append(qMakePair(fm.capturedStart(), bodyEnd));
      }
    }

    auto isShadowed = [&](int offset) -> bool {
      for (const auto &rng : shadowedRanges) {
        if (offset >= rng.first && offset < rng.second) return true;
      }
      return false;
    };

    QList<SymbolOccurrence> occs;
    QRegularExpressionMatchIterator iter = globalSymRe.globalMatch(clean);
    while (iter.hasNext()) {
      QRegularExpressionMatch m = iter.next();
      int absOffset = m.capturedStart();
      if (isShadowed(absOffset)) continue;

      SymbolOccurrence occ;
      occ.startOffset = absOffset;
      occ.endOffset = absOffset + symbolName.length();
      occ.line = code.left(absOffset).count('\n');
      int lineStart = code.lastIndexOf('\n', absOffset - 1) + 1;
      occ.col = absOffset - lineStart;
      occ.length = symbolName.length();
      int lineEnd = code.indexOf('\n', absOffset);
      if (lineEnd == -1) lineEnd = codeLen;
      occ.lineContent = code.mid(lineStart, lineEnd - lineStart);
      occs.append(occ);
    }

    if (!occs.isEmpty()) {
      fileOccurrences.insert(fPath, occs);
    }
  }

  if (fileOccurrences.isEmpty()) return;

  QString scopeDesc = (kind == ScopeFunction) ? "【全工程函数】" : "【全工程全局/类成员符号】";
  ProjectRenameDialog dlg(symbolName, scopeDesc, fileOccurrences, this);
  if (dlg.exec() != QDialog::Accepted) return;

  QString targetNewName = dlg.newName();
  if (targetNewName.isEmpty() || targetNewName == symbolName) return;

  int totalReplaced = 0;
  // 执行全工程跨文件原子替换
  for (auto it = fileOccurrences.begin(); it != fileOccurrences.end(); ++it) {
    QString fPath = it.key();
    QList<SymbolOccurrence> occs = it.value();
    totalReplaced += occs.size();

    // 倒序替换
    std::sort(occs.begin(), occs.end(), [](const SymbolOccurrence &a, const SymbolOccurrence &b) {
      return a.startOffset > b.startOffset;
    });

    if (fPath == m_currentFilePath && m_currentEditor) {
      m_currentEditor->SendScintilla(QsciScintilla::SCI_BEGINUNDOACTION);
      for (const auto &occ : occs) {
        m_currentEditor->setSelection(occ.line, occ.col, occ.line, occ.col + occ.length);
        m_currentEditor->replaceSelectedText(targetNewName);
      }
      m_currentEditor->SendScintilla(QsciScintilla::SCI_ENDUNDOACTION);
    } else {
      QFile file(fPath);
      if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString fText = QString::fromUtf8(file.readAll());
        file.close();

        for (const auto &occ : occs) {
          fText.replace(occ.startOffset, occ.length, targetNewName);
        }

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
          QTextStream out(&file);
          out.setCodec("UTF-8");
          out << fText;
          file.close();
        }
      }
    }
  }

  // 刷新当前编辑器的函数列表与符号高亮
  updateFunctionList();
  if (m_occurrenceTimer) m_occurrenceTimer->start();

  QMessageBox::information(this, "重命名成功",
      QString("符号 \"%1\" 已成功重命名为 \"%2\"\n涉及 %3 个文件，共更新 %4 处引用。")
          .arg(symbolName).arg(targetNewName).arg(fileOccurrences.size()).arg(totalReplaced));
}

void CodeEditor::onCustomContextMenuRequested(const QPoint &pos) {
  if (!m_currentEditor) return;

  // 将光标定位到右键点击处（如果当前未选中文本）
  if (!m_currentEditor->hasSelectedText()) {
    long scintillaPos = m_currentEditor->SendScintilla(QsciScintilla::SCI_POSITIONFROMPOINT, pos.x(), pos.y());
    if (scintillaPos >= 0) {
      int clickLine = m_currentEditor->SendScintilla(QsciScintilla::SCI_LINEFROMPOSITION, scintillaPos);
      int lineStartPos = m_currentEditor->SendScintilla(QsciScintilla::SCI_POSITIONFROMLINE, clickLine);
      QString lineText = m_currentEditor->text(clickLine);
      int byteOffsetInLine = scintillaPos - lineStartPos;
      int clickCol = 0;
      int curBytes = 0;
      while (clickCol < lineText.length() && curBytes < byteOffsetInLine) {
        curBytes += QString(lineText[clickCol]).toUtf8().length();
        ++clickCol;
      }
      m_currentEditor->setCursorPosition(clickLine, clickCol);
    }
  }

  QMenu menu(this);
  menu.setStyleSheet(
      "QMenu {"
      "  background-color: #252526;"
      "  color: #CCCCCC;"
      "  border: 1px solid #3E4451;"
      "  padding: 4px;"
      "}"
      "QMenu::item {"
      "  padding: 5px 24px 5px 12px;"
      "  border-radius: 3px;"
      "}"
      "QMenu::item:selected {"
      "  background-color: #094771;"
      "  color: #FFFFFF;"
      "}"
      "QMenu::separator {"
      "  height: 1px;"
      "  background: #3E4451;"
      "  margin: 4px 0;"
      "}");

  QAction *defAct = menu.addAction(QString("🔍 转到定义 (F12)"));
  connect(defAct, &QAction::triggered, this, &CodeEditor::gotoDefinitionAtCursor);

  QAction *refAct = menu.addAction(QString("📑 查找所有引用 (Shift+F12)"));
  connect(refAct, &QAction::triggered, this, &CodeEditor::findReferencesAtCursor);

  QAction *renAct = menu.addAction(QString("✏️ 一键重命名符号 (F2)"));
  connect(renAct, &QAction::triggered, this, &CodeEditor::renameSymbolAtCursor);

  menu.addSeparator();

  QAction *findAct = menu.addAction(QString("🔎 查找 (Ctrl+F)"));
  connect(findAct, &QAction::triggered, this, [this]() { showFindReplaceBar(false); });

  QAction *repAct = menu.addAction(QString("🔄 替换 (Ctrl+H)"));
  connect(repAct, &QAction::triggered, this, [this]() { showFindReplaceBar(true); });

  menu.addSeparator();

  QAction *undoAct = menu.addAction("撤销 (Ctrl+Z)");
  undoAct->setEnabled(m_currentEditor->isUndoAvailable());
  connect(undoAct, &QAction::triggered, m_currentEditor, &QsciScintilla::undo);

  QAction *redoAct = menu.addAction("重做 (Ctrl+Y)");
  redoAct->setEnabled(m_currentEditor->isRedoAvailable());
  connect(redoAct, &QAction::triggered, m_currentEditor, &QsciScintilla::redo);

  menu.addSeparator();

  QAction *cutAct = menu.addAction("剪切 (Ctrl+X)");
  cutAct->setEnabled(m_currentEditor->hasSelectedText());
  connect(cutAct, &QAction::triggered, m_currentEditor, &QsciScintilla::cut);

  QAction *copyAct = menu.addAction("复制 (Ctrl+C)");
  copyAct->setEnabled(m_currentEditor->hasSelectedText());
  connect(copyAct, &QAction::triggered, m_currentEditor, &QsciScintilla::copy);

  QAction *pasteAct = menu.addAction("粘贴 (Ctrl+V)");
  connect(pasteAct, &QAction::triggered, m_currentEditor, &QsciScintilla::paste);

  QAction *selAllAct = menu.addAction("全选 (Ctrl+A)");
  connect(selAllAct, &QAction::triggered, m_currentEditor, &QsciScintilla::selectAll);

  menu.exec(m_currentEditor->mapToGlobal(pos));
}

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
