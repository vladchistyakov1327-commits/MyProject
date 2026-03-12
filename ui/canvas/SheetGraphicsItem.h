#pragma once
#include "core/nesting/NestResult.h"
#include <QGraphicsItem>
#include <QRectF>

/**
 * QGraphicsItem для отображения листа-заготовки (фон + рамка + margin).
 */
class SheetGraphicsItem : public QGraphicsItem
{
public:
    explicit SheetGraphicsItem(const SheetResult& sr,
                                double marginMm = 5.0,
                                QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void   paint(QPainter* painter,
                 const QStyleOptionGraphicsItem* option,
                 QWidget* widget = nullptr) override;

private:
    SheetResult m_sr;
    double      m_marginMm;
};
