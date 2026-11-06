#ifndef IDEWORKFLOWICONS_H
#define IDEWORKFLOWICONS_H

#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace IDEWorkflowIcons {
enum class Kind { Build, Rebuild, Clean, Flash, Debug, Stop, Pause, Continue,
                  StepOver, StepInto, StepOut, Breakpoint,
                  ChipConfig, FlashConfig, ToolchainConfig };

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
        case Kind::ChipConfig: color = QColor("#06B6D4"); break;
        case Kind::FlashConfig: color = QColor("#F97316"); break;
        case Kind::ToolchainConfig: color = QColor("#60A5FA"); break;
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
        case Kind::ChipConfig: {
            // 形象化 QFP 微控制器芯片封装图标（四边引脚 + 内部晶圆内核 + 1脚定位圆点）
            for (qreal pin : {7.2, 10.4, 13.6, 16.8}) {
                painter->drawLine(QPointF(pin, 1.8), QPointF(pin, 5.0));
                painter->drawLine(QPointF(pin, 19.0), QPointF(pin, 22.2));
                painter->drawLine(QPointF(1.8, pin), QPointF(5.0, pin));
                painter->drawLine(QPointF(19.0, pin), QPointF(22.2, pin));
            }
            QColor fillBody = color;
            fillBody.setAlpha(mode == QIcon::Disabled ? 25 : 40);
            painter->setBrush(fillBody);
            painter->drawRoundedRect(QRectF(5.0, 5.0, 14.0, 14.0), 2.0, 2.0);
            QColor coreFill = color;
            coreFill.setAlpha(mode == QIcon::Disabled ? 45 : 85);
            painter->setBrush(coreFill);
            painter->drawRoundedRect(QRectF(8.6, 8.6, 6.8, 6.8), 1.0, 1.0);
            painter->setBrush(color);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(7.2, 7.2), 1.0, 1.0);
            break;
        }
        case Kind::FlashConfig: {
            // 形象化烧录配置图标（左侧高速编程闪电 + 右下角精密配置齿轮徽标）
            path.moveTo(10.8, 2.2);
            path.lineTo(4.0, 11.5);
            path.lineTo(9.2, 11.5);
            path.lineTo(7.0, 20.5);
            path.lineTo(15.2, 9.8);
            path.lineTo(10.0, 9.8);
            path.closeSubpath();
            QColor boltFill = color;
            boltFill.setAlpha(mode == QIcon::Disabled ? 45 : 85);
            painter->setBrush(boltFill);
            painter->drawPath(path);

            const QColor gearColor = (mode == QIcon::Disabled) ? QColor("#64748B") : QColor("#38BDF8");
            painter->setPen(QPen(gearColor, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(Qt::NoBrush);
            const QPointF gearCenter(17.5, 17.2);
            painter->drawEllipse(gearCenter, 3.0, 3.0);
            for (int deg = 0; deg < 360; deg += 60) {
                const qreal rad = qDegreesToRadians(static_cast<qreal>(deg));
                const QPointF p1(gearCenter.x() + 2.8 * qCos(rad), gearCenter.y() + 2.8 * qSin(rad));
                const QPointF p2(gearCenter.x() + 4.4 * qCos(rad), gearCenter.y() + 4.4 * qSin(rad));
                painter->drawLine(p1, p2);
            }
            painter->setBrush(gearColor);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(gearCenter, 1.1, 1.1);
            break;
        }
        case Kind::ToolchainConfig: {
            // 形象化工具链配置图标（交叉扳手与精密齿轮）
            const QPointF c(12.0, 12.0);
            painter->drawEllipse(c, 5.2, 5.2);
            for (int deg = 0; deg < 360; deg += 45) {
                const qreal rad = qDegreesToRadians(static_cast<qreal>(deg));
                painter->drawLine(QPointF(c.x() + 5.0 * qCos(rad), c.y() + 5.0 * qSin(rad)),
                                  QPointF(c.x() + 7.4 * qCos(rad), c.y() + 7.4 * qSin(rad)));
            }
            painter->drawEllipse(c, 2.2, 2.2);
            break;
        }
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
