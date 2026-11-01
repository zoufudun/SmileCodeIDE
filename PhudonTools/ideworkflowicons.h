#ifndef IDEWORKFLOWICONS_H
#define IDEWORKFLOWICONS_H

#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>

namespace IDEWorkflowIcons {
enum class Kind { Build, Rebuild, Clean, Flash, Debug, Stop, Pause, Continue,
                  StepOver, StepInto, StepOut, Breakpoint };

class Engine : public QIconEngine {
public:
    explicit Engine(Kind kind) : m_kind(kind) {}
    QIconEngine *clone() const override { return new Engine(m_kind); }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode,
               QIcon::State) override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const qreal side = qMin(rect.width(), rect.height());
        painter->translate(rect.center().x() - side / 2, rect.center().y() - side / 2);
        painter->scale(side / 24, side / 24);
        QColor color("#38BDF8");
        switch (m_kind) {
        case Kind::Build: case Kind::Continue: color = QColor("#10B981"); break;
        case Kind::Rebuild: case Kind::Flash: color = QColor("#F59E0B"); break;
        case Kind::Stop: case Kind::Breakpoint: color = QColor("#EF4444"); break;
        case Kind::Pause: color = QColor("#EAB308"); break;
        case Kind::StepInto: case Kind::StepOut: color = QColor("#A78BFA"); break;
        case Kind::Clean: color = QColor("#94A3B8"); break;
        default: break;
        }
        if (mode == QIcon::Disabled) color = QColor("#64748B");
        painter->setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        QPainterPath path;
        switch (m_kind) {
        case Kind::Build:
            path.moveTo(5, 20); path.lineTo(14, 11); path.moveTo(9, 5);
            path.lineTo(13, 2); path.lineTo(21, 10); path.lineTo(18, 13);
            path.closeSubpath(); painter->drawPath(path); break;
        case Kind::Rebuild:
            painter->drawArc(QRectF(4, 4, 16, 16), 40 * 16, 285 * 16);
            path.moveTo(15, 3); path.lineTo(20, 4); path.lineTo(19, 9);
            painter->drawPath(path); break;
        case Kind::Clean:
            painter->drawLine(QPointF(16, 3), QPointF(10, 11));
            path.moveTo(9, 10); path.lineTo(16, 15); path.lineTo(12, 21);
            path.lineTo(3, 17); path.closeSubpath(); painter->drawPath(path);
            painter->drawLine(QPointF(7, 15), QPointF(5, 18));
            painter->drawLine(QPointF(10, 17), QPointF(8, 20)); break;
        case Kind::Flash:
            painter->drawRoundedRect(QRectF(5, 5, 14, 14), 2, 2);
            for (int pin : {8, 12, 16}) {
                painter->drawLine(pin, 2, pin, 5); painter->drawLine(pin, 19, pin, 22);
                painter->drawLine(2, pin, 5, pin); painter->drawLine(19, pin, 22, pin);
            }
            path.moveTo(12, 8); path.lineTo(12, 16); path.moveTo(9, 13);
            path.lineTo(12, 16); path.lineTo(15, 13); painter->drawPath(path); break;
        case Kind::Debug:
            painter->drawRoundedRect(QRectF(7, 8, 10, 12), 4, 4);
            painter->drawArc(QRectF(9, 4, 6, 8), 0, 180 * 16);
            for (int y : {10, 14, 18}) {
                painter->drawLine(3, y, 7, y); painter->drawLine(17, y, 21, y);
            }
            painter->drawLine(9, 3, 10, 5); painter->drawLine(15, 3, 14, 5);
            painter->drawLine(12, 10, 12, 18); break;
        case Kind::Stop:
            painter->setBrush(color); painter->drawRoundedRect(QRectF(5, 5, 14, 14), 2, 2); break;
        case Kind::Pause:
            painter->setBrush(color); painter->drawRoundedRect(QRectF(6, 4, 4, 16), 1, 1);
            painter->drawRoundedRect(QRectF(14, 4, 4, 16), 1, 1); break;
        case Kind::Continue:
            path.moveTo(7, 4); path.lineTo(20, 12); path.lineTo(7, 20);
            path.closeSubpath(); painter->fillPath(path, color); break;
        case Kind::StepOver:
            path.moveTo(3, 13); path.cubicTo(4, 2, 20, 2, 21, 13);
            path.moveTo(17, 9); path.lineTo(21, 13); path.lineTo(22, 8);
            painter->drawPath(path); painter->drawEllipse(QPointF(12, 19), 1.5, 1.5); break;
        case Kind::StepInto: case Kind::StepOut: {
            const bool into = m_kind == Kind::StepInto;
            path.moveTo(12, into ? 3 : 17); path.lineTo(12, into ? 17 : 3);
            path.moveTo(8, into ? 13 : 7); path.lineTo(12, into ? 17 : 3);
            path.lineTo(16, into ? 13 : 7); painter->drawPath(path);
            painter->drawLine(5, 21, 19, 21); break;
        }
        case Kind::Breakpoint:
            painter->setBrush(color); painter->drawEllipse(QRectF(5, 5, 14, 14)); break;
        }
        painter->restore();
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap result(size);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        paint(&painter, QRect(QPoint(), size), mode, state);
        return result;
    }
private:
    Kind m_kind;
};

inline QIcon icon(Kind kind) { return QIcon(new Engine(kind)); }
}
#endif // IDEWORKFLOWICONS_H
