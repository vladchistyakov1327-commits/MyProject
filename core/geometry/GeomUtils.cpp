#include "GeomUtils.h"
#include <clipper2/clipper.h>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <stdexcept>

using namespace Clipper2Lib;

static constexpr double kScale = 1e6; // мм → нанометры для Clipper2

namespace GeomUtils
{

// ── Конвертация ───────────────────────────────────────────────────────────────

PathD toPathD(const std::vector<QPointF>& pts)
{
    PathD path;
    path.reserve(pts.size());
    for (const auto& p : pts)
        path.push_back({p.x(), p.y()});
    return path;
}

PathsD toPathsD(const std::vector<PolyContour>& contours)
{
    PathsD paths;
    paths.reserve(contours.size());
    for (const auto& c : contours)
        paths.push_back(toPathD(c.vertices));
    return paths;
}

std::vector<QPointF> fromPathD(const PathD& path)
{
    std::vector<QPointF> pts;
    pts.reserve(path.size());
    for (const auto& p : path)
        pts.push_back({p.x, p.y});
    return pts;
}

// ── Utilities ─────────────────────────────────────────────────────────────────

double dist2(const QPointF& a, const QPointF& b)
{
    const double dx = a.x() - b.x();
    const double dy = a.y() - b.y();
    return dx * dx + dy * dy;
}

double dist(const QPointF& a, const QPointF& b)
{
    return std::sqrt(dist2(a, b));
}

double signedArea(const std::vector<QPointF>& pts)
{
    if (pts.size() < 3) return 0.0;
    double area = 0.0;
    const int n = static_cast<int>(pts.size());
    for (int i = 0; i < n; ++i) {
        const auto& p1 = pts[i];
        const auto& p2 = pts[(i + 1) % n];
        area += (p1.x() * p2.y()) - (p2.x() * p1.y());
    }
    return area * 0.5;
}

double area(const std::vector<QPointF>& pts)
{
    return std::abs(signedArea(pts));
}

QRectF boundingBox(const std::vector<QPointF>& pts)
{
    if (pts.empty()) return {};
    double minX = pts[0].x(), maxX = pts[0].x();
    double minY = pts[0].y(), maxY = pts[0].y();
    for (const auto& p : pts) {
        minX = std::min(minX, p.x()); maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y()); maxY = std::max(maxY, p.y());
    }
    return {minX, minY, maxX - minX, maxY - minY};
}

bool pointInPolygon(const QPointF& pt, const std::vector<QPointF>& poly)
{
    // Ray casting algorithm
    bool inside = false;
    const int n = static_cast<int>(poly.size());
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const double xi = poly[i].x(), yi = poly[i].y();
        const double xj = poly[j].x(), yj = poly[j].y();
        if (((yi > pt.y()) != (yj > pt.y())) &&
            (pt.x() < (xj - xi) * (pt.y() - yi) / (yj - yi) + xi))
        {
            inside = !inside;
        }
    }
    return inside;
}

bool isContainedIn(const std::vector<QPointF>& inner,
                   const std::vector<QPointF>& outer)
{
    if (inner.empty()) return false;
    // Проверяем первую точку и несколько контрольных точек
    for (const auto& p : inner) {
        if (!pointInPolygon(p, outer)) return false;
    }
    return true;
}

// ── Offset ────────────────────────────────────────────────────────────────────

std::vector<QPointF> offsetContour(const std::vector<QPointF>& pts, double deltaMm)
{
    if (pts.empty()) return {};
    PathsD paths = {toPathD(pts)};
    PathsD result = InflatePaths(paths, deltaMm,
                                 JoinType::Round, EndType::Polygon, 2.0);
    if (result.empty()) return pts;
    return fromPathD(result[0]);
}

std::vector<QPointF> deflateSheet(const std::vector<QPointF>& sheetPts, double marginMm)
{
    return offsetContour(sheetPts, -marginMm);
}

// ── Simplify (Douglas-Peucker via Clipper2) ───────────────────────────────────

std::vector<QPointF> simplify(const std::vector<QPointF>& pts, double toleranceMm)
{
    if (pts.size() < 3) return pts;
    PathD path = toPathD(pts);
    PathD simplified = SimplifyPath(path, toleranceMm, true);
    return fromPathD(simplified);
}

// ── Self-intersection ─────────────────────────────────────────────────────────

bool hasSelfIntersection(const std::vector<QPointF>& pts)
{
    if (pts.size() < 4) return false;
    // Используем Clipper2 — если SimplifyPolygon изменяет форму → есть самопересечения
    PathD path = toPathD(pts);
    PathsD union_result;
    ClipperD clipper;
    clipper.AddSubject({path});
    clipper.Execute(ClipType::Union, FillRule::NonZero, union_result);
    // Если количество путей или вершин изменилось — было самопересечение
    if (union_result.size() != 1) return true;
    if (union_result[0].size() != path.size()) return true;
    return false;
}

std::vector<QPointF> fixSelfIntersection(const std::vector<QPointF>& pts)
{
    PathD path = toPathD(pts);
    PathsD result;
    ClipperD clipper;
    clipper.AddSubject({path});
    clipper.Execute(ClipType::Union, FillRule::NonZero, result);
    if (result.empty()) return pts;
    // Берём наибольший контур
    auto it = std::max_element(result.begin(), result.end(),
        [](const PathD& a, const PathD& b) {
            return std::abs(Area(a)) < std::abs(Area(b));
        });
    return fromPathD(*it);
}

// ── Arc approximation ─────────────────────────────────────────────────────────

std::vector<QPointF> approximateArc(const QPointF& center, double radius,
                                     double startRad, double endRad,
                                     double toleranceMm)
{
    if (radius <= 0.0) return {};

    // Количество сегментов: чем меньше radius, тем меньше нужно точек
    // Формула: n = PI / acos(1 - tolerance/radius)
    double cosArg = 1.0 - toleranceMm / radius;
    cosArg = std::max(-1.0, std::min(1.0, cosArg));
    const int minSeg = 8;
    const int n = std::max(minSeg,
        static_cast<int>(std::ceil(std::abs(endRad - startRad)
                                   / std::acos(cosArg))));

    std::vector<QPointF> pts;
    pts.reserve(n + 1);

    double angle = startRad;
    double step  = (endRad - startRad) / n;

    for (int i = 0; i <= n; ++i) {
        pts.push_back({center.x() + radius * std::cos(angle),
                       center.y() + radius * std::sin(angle)});
        angle += step;
    }
    return pts;
}

std::vector<QPointF> approximateEllipse(const QPointF& center,
                                         double rx, double ry,
                                         double rotationRad,
                                         double startRad, double endRad,
                                         double toleranceMm)
{
    const double maxR = std::max(rx, ry);
    if (maxR <= 0.0) return {};

    double cosArg = 1.0 - toleranceMm / maxR;
    cosArg = std::max(-1.0, std::min(1.0, cosArg));
    const int n = std::max(16,
        static_cast<int>(std::ceil(std::abs(endRad - startRad) / std::acos(cosArg))));

    std::vector<QPointF> pts;
    pts.reserve(n + 1);

    const double cosR = std::cos(rotationRad);
    const double sinR = std::sin(rotationRad);
    const double step = (endRad - startRad) / n;

    for (int i = 0; i <= n; ++i) {
        const double t = startRad + i * step;
        const double ex = rx * std::cos(t);
        const double ey = ry * std::sin(t);
        pts.push_back({center.x() + ex * cosR - ey * sinR,
                       center.y() + ex * sinR + ey * cosR});
    }
    return pts;
}

// ── Spline (De Boor) ──────────────────────────────────────────────────────────

static QPointF deBoor(int k, int degree,
                       const std::vector<QPointF>& controls,
                       const std::vector<double>& knots,
                       double t)
{
    std::vector<QPointF> d(controls.begin() + k - degree,
                            controls.begin() + k + 1);
    for (int r = 1; r <= degree; ++r) {
        for (int j = degree; j >= r; --j) {
            const int idx = j + k - degree;
            const double denom = knots[idx + degree - r + 1] - knots[idx];
            const double alpha = (denom < 1e-12) ? 0.0
                : (t - knots[idx]) / denom;
            d[j].setX((1.0 - alpha) * d[j - 1].x() + alpha * d[j].x());
            d[j].setY((1.0 - alpha) * d[j - 1].y() + alpha * d[j].y());
        }
    }
    return d[degree];
}

std::vector<QPointF> approximateSpline(const std::vector<QPointF>& controlPts,
                                        int degree,
                                        const std::vector<double>& knots,
                                        double /*toleranceMm*/)
{
    if (controlPts.size() < 2 || knots.empty()) return controlPts;
    if (degree < 1) return controlPts;

    const int n  = static_cast<int>(controlPts.size()) - 1;
    const int p  = degree;
    const int m  = static_cast<int>(knots.size()) - 1;

    if (m != n + p + 1) return controlPts; // несоответствие узлового вектора

    const double tStart = knots[p];
    const double tEnd   = knots[m - p];
    const int    steps  = std::max(64, (n + 1) * 8);

    std::vector<QPointF> pts;
    pts.reserve(steps + 1);

    for (int i = 0; i <= steps; ++i) {
        double t = tStart + (tEnd - tStart) * i / steps;
        if (i == steps) t = tEnd - 1e-10;

        // Найти knotSpan
        int k = p;
        while (k < m - p && knots[k + 1] <= t) ++k;

        pts.push_back(deBoor(k, p, controlPts, knots, t));
    }
    return pts;
}

// ── Transform ─────────────────────────────────────────────────────────────────

std::vector<QPointF> transformContour(const std::vector<QPointF>& pts,
                                       double translateX, double translateY,
                                       double rotationRad,
                                       double scaleX, double scaleY)
{
    const double cosA = std::cos(rotationRad);
    const double sinA = std::sin(rotationRad);
    std::vector<QPointF> result;
    result.reserve(pts.size());
    for (const auto& p : pts) {
        const double sx = p.x() * scaleX;
        const double sy = p.y() * scaleY;
        result.push_back({sx * cosA - sy * sinA + translateX,
                          sx * sinA + sy * cosA + translateY});
    }
    return result;
}

std::vector<QPointF> mirrorX(const std::vector<QPointF>& pts)
{
    std::vector<QPointF> result;
    result.reserve(pts.size());
    for (const auto& p : pts)
        result.push_back({-p.x(), p.y()});
    return result;
}

// ── buildPartGeometry ─────────────────────────────────────────────────────────

PartGeometry buildPartGeometry(const std::vector<std::vector<QPointF>>& rawContours,
                                const QString& partId,
                                const QString& sourceName,
                                const QString& sourceLayer)
{
    PartGeometry geom;
    geom.partId      = partId;
    geom.sourceName  = sourceName;
    geom.sourceLayer = sourceLayer;

    if (rawContours.empty()) return geom;

    // Найти внешний контур — тот с наибольшей площадью
    std::size_t outerIdx = 0;
    double maxArea = 0.0;
    for (std::size_t i = 0; i < rawContours.size(); ++i) {
        const double a = area(rawContours[i]);
        if (a > maxArea) { maxArea = a; outerIdx = i; }
    }

    geom.outerContour.vertices = rawContours[outerIdx];
    geom.outerContour.isHole   = false;
    geom.outerContour.fixOrientation();

    // Остальные контуры — потенциальные отверстия
    for (std::size_t i = 0; i < rawContours.size(); ++i) {
        if (i == outerIdx) continue;
        // Отверстие должно находиться внутри внешнего контура
        if (isContainedIn(rawContours[i], geom.outerContour.vertices)) {
            PolyContour hole;
            hole.vertices = rawContours[i];
            hole.isHole   = true;
            hole.fixOrientation();
            geom.holes.push_back(std::move(hole));
        }
    }

    geom.boundingBox = boundingBox(geom.outerContour.vertices);
    geom.areaMm2     = area(geom.outerContour.vertices);
    for (const auto& h : geom.holes)
        geom.areaMm2 -= h.area();

    return geom;
}

} // namespace GeomUtils
