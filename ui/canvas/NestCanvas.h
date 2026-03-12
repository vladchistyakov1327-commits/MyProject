#pragma once
#include "core/nesting/NestResult.h"
#include <QGraphicsView>
#include <QHash>
#include <QColor>

class PartGraphicsItem;
class SheetGraphicsItem;

/**
 * Основная область отображения результата нестинга.
 *
 * Управление:
 *  - Колесо мыши → zoom
 *  - ПКМ drag    → pan
 *  - ЛКМ click   → выделить деталь
 *
 * LOD: при zoom < 0.3 детали рисуются как bbox.
 *      При zoom > 2.0 внутри детали показывается её ID.
 */
class NestCanvas : public QGraphicsView
{
    Q_OBJECT
public:
    explicit NestCanvas(QWidget* parent = nullptr);

    void setSheetResult(const SheetResult& result, double marginMm = 5.0);

    /** Отобразить полный NestResult (все листы, первый видимый по умолчанию). */
    void setResult(const NestResult& result, double sheetW, double sheetH);

    /** Очистить сцену (вызывать перед новым расчётом). */
    void clearLayout();
    void clearScene();

    void setZoom(double factor);
    void fitToWindow();
    void highlightPart(const QString& instanceId);

    double currentZoom() const { return m_zoom; }

signals:
    void partSelected(QString instanceId);
    void zoomChanged(double factor);

protected:
    void wheelEvent     (QWheelEvent*  event) override;
    void mousePressEvent(QMouseEvent*  event) override;
    void mouseMoveEvent (QMouseEvent*  event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    double      m_zoom      = 1.0;
    bool        m_panning   = false;
    QPoint      m_panStart;
    QString     m_highlighted;

    QHash<QString, PartGraphicsItem*> m_partItems; // instanceId → item

    static QColor colorForPartType(const QString& partId);
};
