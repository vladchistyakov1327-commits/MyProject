#include "PartGraphicsItem.h"
#include "AppStyle.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsScene>

PartGraphicsItem::PartGraphicsItem(const PlacedPart& pp,
                                    const QColor& fillColor,
                                    QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_pp(pp)
    , m_fillColor(fillColor)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
    setAcceptHoverEvents(true);
    buildPath();
}

void PartGraphicsItem::buildPath()
{
    m_path = QPainterPath();
    if (m_pp.placedContour.isEmpty()) return;

    m_path.addPolygon(m_pp.placedContour);
    m_path.closeSubpath();
    m_bbox = m_path.boundingRect();
}

QRectF PartGraphicsItem::boundingRect() const
{
    return m_bbox.adjusted(-1, -1, 1, 1);
}

QPainterPath PartGraphicsItem::shape() const
{
    return m_path;
}

void PartGraphicsItem::paint(QPainter* painter,
                              const QStyleOptionGraphicsItem* option,
                              QWidget* /*widget*/)
{
    if (m_pp.placedContour.isEmpty()) return;

    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    if (lod < 0.3) {
        // LOD: рисуем только bounding rect
        painter->fillRect(m_bbox, m_fillColor.darker(150));
        painter->setPen(QPen(QColor(AppPalette::BORDER_MID), 0));
        painter->drawRect(m_bbox);
        return;
    }

    // Заливка
    QColor fill = m_fillColor;
    if (m_highlighted || isSelected()) {
        fill = fill.lighter(130);
    }
    fill.setAlpha(200);

    painter->setBrush(fill);

    // Контур
    const QColor borderColor = (m_highlighted || isSelected())
        ? QColor(AppPalette::BLUE_BRIGHT)
        : QColor(AppPalette::BORDER_MID);
    const qreal penWidth = (m_highlighted || isSelected()) ? 2.0 / lod : 1.0 / lod;
    painter->setPen(QPen(borderColor, penWidth));

    painter->drawPath(m_path);

    // Метка при большом zoom
    if (lod > 2.0) {
        painter->setPen(QPen(QColor(AppPalette::TEXT_BRIGHT), 0));
        const QFont font("Segoe UI", 0);
        painter->setFont(font);
        painter->drawText(m_bbox, Qt::AlignCenter,
                          m_pp.partId.left(8));
    }
}

void PartGraphicsItem::setHighlighted(bool on)
{
    if (m_highlighted == on) return;
    m_highlighted = on;
    update();
}
