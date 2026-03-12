#include "SheetGraphicsItem.h"
#include "AppStyle.h"
#include <QPainter>
#include <QPen>

SheetGraphicsItem::SheetGraphicsItem(const SheetResult& sr,
                                      double marginMm,
                                      QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_sr(sr)
    , m_marginMm(marginMm)
{}

QRectF SheetGraphicsItem::boundingRect() const
{
    return m_sr.sheetBounds.adjusted(-2, -2, 2, 2);
}

void SheetGraphicsItem::paint(QPainter* painter,
                               const QStyleOptionGraphicsItem* /*option*/,
                               QWidget* /*widget*/)
{
    const QRectF& bounds = m_sr.sheetBounds;

    // Фон листа
    painter->fillRect(bounds, QColor(AppPalette::BG_DEEP));

    // Рамка листа
    painter->setPen(QPen(QColor(AppPalette::BORDER_LIGHT), 1.5));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(bounds);

    // Margin — штриховая линия
    if (m_marginMm > 0.0) {
        const QRectF marginRect = bounds.adjusted(m_marginMm, m_marginMm,
                                                    -m_marginMm, -m_marginMm);
        QPen dashPen(QColor(AppPalette::TEXT_GHOST), 0.5, Qt::DashLine);
        painter->setPen(dashPen);
        painter->drawRect(marginRect);
    }
}
