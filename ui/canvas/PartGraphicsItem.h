#pragma once
#include "core/nesting/NestResult.h"
#include "core/geometry/PolyContour.h"
#include <QGraphicsItem>
#include <QColor>

/**
 * QGraphicsItem для отображения одного размещённого экземпляра детали.
 * Поддерживает LOD — при zoom < 0.3 рисует только bounding rect.
 */
class PartGraphicsItem : public QGraphicsItem
{
public:
    explicit PartGraphicsItem(const PlacedPart& pp, const QColor& fillColor,
                               QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void   paint(QPainter* painter,
                 const QStyleOptionGraphicsItem* option,
                 QWidget* widget = nullptr) override;
    QPainterPath shape() const override;

    void setHighlighted(bool on);
    bool isHighlighted() const { return m_highlighted; }

    const QString& instanceId() const { return m_pp.instanceId; }
    const QString& partId()     const { return m_pp.partId;     }

private:
    PlacedPart   m_pp;
    QColor       m_fillColor;
    bool         m_highlighted = false;
    QRectF       m_bbox;
    QPainterPath m_path;

    void buildPath();
};
