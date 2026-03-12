#pragma once
#include "PolyContour.h"
#include <clipper2/clipper.h>
#include <optional>
#include <vector>

/**
 * Геометрические утилиты на базе Clipper2.
 * Все координаты в мм (double).
 */
namespace GeomUtils
{
    // ── Конвертация между Qt и Clipper2 ──────────────────────────────────
    Clipper2Lib::PathD    toPathD   (const std::vector<QPointF>& pts);
    Clipper2Lib::PathsD   toPathsD  (const std::vector<PolyContour>& contours);
    std::vector<QPointF>  fromPathD (const Clipper2Lib::PathD& path);

    // ── Offset (Minkowski) ───────────────────────────────────────────────
    /// Расширить / сжать контур на delta мм (положительный → расширить).
    std::vector<QPointF> offsetContour(const std::vector<QPointF>& pts,
                                       double deltaMm);

    /// Deflate (уменьшить) лист на margin мм с каждой стороны.
    std::vector<QPointF> deflateSheet(const std::vector<QPointF>& sheetPts,
                                      double marginMm);

    // ── Упрощение ────────────────────────────────────────────────────────
    /// Douglas–Peucker упрощение полигона с допуском toleranceMm.
    std::vector<QPointF> simplify(const std::vector<QPointF>& pts,
                                  double toleranceMm);

    // ── BBox ─────────────────────────────────────────────────────────────
    QRectF boundingBox(const std::vector<QPointF>& pts);

    // ── Площадь ──────────────────────────────────────────────────────────
    double signedArea(const std::vector<QPointF>& pts);
    double area      (const std::vector<QPointF>& pts);

    // ── Точка внутри полигона ─────────────────────────────────────────────
    bool pointInPolygon(const QPointF& pt, const std::vector<QPointF>& poly);

    // ── Определение вложенности (для hole detection) ──────────────────────
    bool isContainedIn(const std::vector<QPointF>& inner,
                       const std::vector<QPointF>& outer);

    // ── Нормализация контуров: определение внешних и отверстий ───────────
    PartGeometry buildPartGeometry(const std::vector<std::vector<QPointF>>& rawContours,
                                   const QString& partId,
                                   const QString& sourceName,
                                   const QString& sourceLayer = {});

    // ── Обнаружение самопересечения ───────────────────────────────────────
    bool hasSelfIntersection(const std::vector<QPointF>& pts);

    /// Попытка исправить самопересекающийся контур через SimplifyPolygons.
    std::vector<QPointF> fixSelfIntersection(const std::vector<QPointF>& pts);

    // ── Расстояние ───────────────────────────────────────────────────────
    double dist(const QPointF& a, const QPointF& b);
    double dist2(const QPointF& a, const QPointF& b);  ///< Квадрат расстояния

    // ── Аппроксимация дуги точками ───────────────────────────────────────
    /// Дуга задана центром, радиусом, начальным и конечным углом (в радианах).
    std::vector<QPointF> approximateArc(const QPointF& center, double radius,
                                        double startRad, double endRad,
                                        double toleranceMm = 0.01);

    /// Аппроксимация эллипса точками.
    std::vector<QPointF> approximateEllipse(const QPointF& center,
                                            double rx, double ry,
                                            double rotationRad,
                                            double startRad, double endRad,
                                            double toleranceMm = 0.01);

    /// Аппроксимация B-сплайна точками (De Boor).
    std::vector<QPointF> approximateSpline(const std::vector<QPointF>& controlPts,
                                           int degree,
                                           const std::vector<double>& knots,
                                           double toleranceMm = 0.01);

    // ── Трансформация ────────────────────────────────────────────────────
    std::vector<QPointF> transformContour(const std::vector<QPointF>& pts,
                                          double translateX, double translateY,
                                          double rotationRad,
                                          double scaleX = 1.0, double scaleY = 1.0);

    std::vector<QPointF> mirrorX(const std::vector<QPointF>& pts);
}
