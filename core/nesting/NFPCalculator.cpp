#include "NFPCalculator.h"
#include "PlacementStrategy.h"
#include "core/geometry/GeomUtils.h"
#include <clipper2/clipper.h>
#include <cmath>
#include <algorithm>
#include <optional>
#include <limits>

using namespace Clipper2Lib;

// ── Конвертация ───────────────────────────────────────────────────────────────

PathD NFPCalculator::toPathD(const std::vector<QPointF>& pts) const
{
    PathD p;
    p.reserve(pts.size());
    for (const auto& v : pts)
        p.push_back({v.x(), v.y()});
    return p;
}

std::vector<QPointF> NFPCalculator::fromPathD(const PathD& path) const
{
    std::vector<QPointF> pts;
    pts.reserve(path.size());
    for (const auto& v : path)
        pts.push_back({v.x, v.y});
    return pts;
}

// ── Хэш контура ──────────────────────────────────────────────────────────────

std::size_t NFPCalculator::hashContour(const std::vector<QPointF>& pts)
{
    std::size_t h = pts.size();
    for (const auto& p : pts) {
        // Округляем до 0.001 мм
        const long long ix = static_cast<long long>(std::round(p.x() * 1000.0));
        const long long iy = static_cast<long long>(std::round(p.y() * 1000.0));
        h ^= std::hash<long long>{}(ix) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<long long>{}(iy) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

// ── NFP через Minkowski Sum ───────────────────────────────────────────────────

PathsD NFPCalculator::computeNFP(const std::vector<QPointF>& fixedPts,
                                   const std::vector<QPointF>& movingPts) const
{
    if (fixedPts.size() < 3 || movingPts.size() < 3) return {};

    PathD fixed  = toPathD(fixedPts);
    PathD moving = toPathD(movingPts);

    // Отразить moving через origin: -v
    PathD reflected;
    reflected.reserve(moving.size());
    for (const auto& v : moving)
        reflected.push_back({-v.x, -v.y});

    // Minkowski Sum(fixed, reflected) = NFP boundary
    PathsD result = MinkowskiSum(fixed, reflected, true);
    return result;
}

// ── IFP (Inner Fit Polygon) ───────────────────────────────────────────────────
// IFP — это контур листа, уменьшенный на размер moving part
// Реализуется как erosion (deflate) листа на bounding radius детали

PathsD NFPCalculator::computeIFP(const std::vector<QPointF>& sheetPts,
                                   const std::vector<QPointF>& movingPts) const
{
    if (sheetPts.size() < 3 || movingPts.size() < 3) return {};

    // Используем Minkowski Difference для вычисления IFP
    // IFP = Minkowski_diff(sheet, moving)
    // = пространство, в котором origin детали B находится полностью внутри листа

    PathD sheet  = toPathD(sheetPts);
    PathD moving = toPathD(movingPts);

    // Отразить листовой контур и вычислить Minkowski Sum
    // IFP = sheet ⊕ (-moving)
    PathD reflected;
    reflected.reserve(moving.size());
    for (const auto& v : moving)
        reflected.push_back({-v.x, -v.y});

    // Простая реализация через offset (приближение):
    // Найти bounding box детали
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for (const auto& v : moving) {
        minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
    }

    // Shrink sheet by half of moving part dimensions
    // Это приближение, точный IFP требует Minkowski difference
    const double shrinkX = (maxX - minX) * 0.5;
    const double shrinkY = (maxY - minY) * 0.5;
    const double shrink  = std::max(shrinkX, shrinkY);

    // Для прямоугольного листа — это просто
    PathsD sheetPath = {sheet};
    PathsD ifp = InflatePaths(sheetPath, -shrink, JoinType::Miter, EndType::Polygon, 2.0);

    if (ifp.empty()) {
        // Попробовать точный Minkowski
        ifp = MinkowskiSum(sheet, reflected, true);
    }

    return ifp;
}

// ── Допустимая область ────────────────────────────────────────────────────────

PathsD NFPCalculator::computeValidRegion(
    const PathsD& ifp,
    const PathsD& nfpUnion) const
{
    if (ifp.empty()) return {};
    if (nfpUnion.empty()) return ifp;

    ClipperD clipper(8);
    clipper.AddSubject(ifp);
    clipper.AddClip(nfpUnion);
    PathsD result;
    clipper.Execute(ClipType::Difference, FillRule::NonZero, result);
    return result;
}

// ── Стратегия выбора позиции ─────────────────────────────────────────────────

std::optional<QPointF> NFPCalculator::findBestPosition(
    const PathsD& validRegion,
    PlacementStrategy strategy,
    const QRectF& sheetBounds) const
{
    if (validRegion.empty()) return std::nullopt;

    // Собираем все вершины допустимой области и выбираем лучшую по стратегии
    std::optional<QPointF> best;
    double bestScore = std::numeric_limits<double>::max();

    const double cx = sheetBounds.center().x();
    const double cy = sheetBounds.center().y();

    auto score = [&](double x, double y) -> double {
        switch (strategy) {
            case PlacementStrategy::BOTTOM_LEFT:
                return y * 1e9 + x;
            case PlacementStrategy::BOTTOM_RIGHT:
                return y * 1e9 - x;
            case PlacementStrategy::TOP_LEFT:
                return -y * 1e9 + x;
            case PlacementStrategy::TOP_RIGHT:
                return -y * 1e9 - x;
            case PlacementStrategy::LEFT_BOTTOM:
                return x * 1e9 + y;
            case PlacementStrategy::LEFT_TOP:
                return x * 1e9 - y;
            case PlacementStrategy::RIGHT_BOTTOM:
                return -x * 1e9 + y;
            case PlacementStrategy::RIGHT_TOP:
                return -x * 1e9 - y;
            case PlacementStrategy::GRAVITY_CENTER: {
                const double dx = x - cx, dy = y - cy;
                return dx * dx + dy * dy;
            }
            case PlacementStrategy::GRAVITY_EDGE: {
                const double dx = x - cx, dy = y - cy;
                return -(dx * dx + dy * dy);
            }
        }
        return y * 1e9 + x;
    };

    for (const auto& path : validRegion) {
        for (const auto& pt : path) {
            const double s = score(pt.x, pt.y);
            if (s < bestScore) {
                bestScore = s;
                best = QPointF(pt.x, pt.y);
            }
        }
    }

    return best;
}
