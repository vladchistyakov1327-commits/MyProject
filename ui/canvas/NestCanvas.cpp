#include "NestCanvas.h"
#include "PartGraphicsItem.h"
#include "SheetGraphicsItem.h"
#include "AppStyle.h"

#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QHash>
#include <cmath>

NestCanvas::NestCanvas(QWidget* parent)
    : QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));
    setBackgroundBrush(QBrush(QColor(AppPalette::BG_VOID)));
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
}

// ── Цвет по типу детали ───────────────────────────────────────────────────────

QColor NestCanvas::colorForPartType(const QString& partId)
{
    // Генерируем уникальный цвет из HSV по хешу partId
    const std::size_t h = qHash(partId);
    const int hue = static_cast<int>(h % 360);
    return QColor::fromHsv(hue, 120, 200);
}

// ── Основной метод ────────────────────────────────────────────────────────────

void NestCanvas::setSheetResult(const SheetResult& result, double marginMm)
{
    scene()->clear();
    m_partItems.clear();

    // Лист
    auto* sheetItem = new SheetGraphicsItem(result, marginMm);
    scene()->addItem(sheetItem);

    // Детали
    for (const auto& pp : result.placedParts) {
        const QColor color = colorForPartType(pp.partId);
        auto* item = new PartGraphicsItem(pp, color);
        scene()->addItem(item);
        m_partItems[pp.instanceId] = item;
    }

    fitToWindow();
}

void NestCanvas::clearScene()
{
    scene()->clear();
    m_partItems.clear();
}

void NestCanvas::clearLayout()
{
    clearScene();
}

void NestCanvas::setResult(const NestResult& result, double sheetW, double sheetH)
{
    clearScene();
    Q_UNUSED(sheetW); Q_UNUSED(sheetH);
    // Показываем первый лист результата
    if (!result.sheets.empty()) {
        setSheetResult(result.sheets[0]);
    }
}

void NestCanvas::setZoom(double factor)
{
    m_zoom = std::max(0.01, std::min(factor, 100.0));
    resetTransform();
    scale(m_zoom, m_zoom);
    emit zoomChanged(m_zoom);
}

void NestCanvas::fitToWindow()
{
    if (scene()->sceneRect().isEmpty()) return;
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    m_zoom = transform().m11();
    emit zoomChanged(m_zoom);
}

void NestCanvas::highlightPart(const QString& instanceId)
{
    // Убрать предыдущее выделение
    if (!m_highlighted.isEmpty()) {
        auto* prev = m_partItems.value(m_highlighted, nullptr);
        if (prev) prev->setHighlighted(false);
    }

    m_highlighted = instanceId;
    auto* item = m_partItems.value(instanceId, nullptr);
    if (item) {
        item->setHighlighted(true);
        ensureVisible(item, 50, 50);
    }
}

// ── События ──────────────────────────────────────────────────────────────────

void NestCanvas::wheelEvent(QWheelEvent* event)
{
    const double factor = (event->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
    m_zoom *= factor;
    m_zoom = std::max(0.01, std::min(m_zoom, 200.0));
    scale(factor, factor);
    emit zoomChanged(m_zoom);
    event->accept();
}

void NestCanvas::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        m_panning  = true;
        m_panStart = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // Клик — выбрать деталь
        const QPointF scenePos = mapToScene(event->pos());
        const auto items       = scene()->items(scenePos);
        for (auto* it : items) {
            if (auto* pi = dynamic_cast<PartGraphicsItem*>(it)) {
                emit partSelected(pi->instanceId());
                break;
            }
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void NestCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        const QPoint delta = event->pos() - m_panStart;
        m_panStart = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue  (verticalScrollBar()->value()   - delta.y());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void NestCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton && m_panning) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}
