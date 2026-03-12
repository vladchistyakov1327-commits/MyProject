#include "CoedgeDetector.h"
#include "GeomUtils.h"
#include "core/logging/AppLogger.h"
#include "core/logging/LogChannel.h"
#include <cmath>

CoedgeDetector::CoedgeDetector(double toleranceMm)
    : m_tolerance(toleranceMm)
{}

double CoedgeDetector::Edge::length() const
{
    return GeomUtils::dist(start, end);
}

std::vector<CoedgeDetector::Edge>
CoedgeDetector::extractEdges(const PartGeometry& geom,
                               const QPointF& pos, double rotDeg) const
{
    // Трансформируем контур в мировые координаты
    const double rotRad = rotDeg * M_PI / 180.0;
    auto worldPts = GeomUtils::transformContour(geom.outerContour.vertices,
                                                 pos.x(), pos.y(), rotRad);
    std::vector<Edge> edges;
    const int n = static_cast<int>(worldPts.size());
    for (int i = 0; i < n - 1; ++i) {
        Edge e;
        e.start = worldPts[i];
        e.end   = worldPts[i + 1];
        if (e.length() > m_tolerance)
            edges.push_back(e);
    }
    return edges;
}

bool CoedgeDetector::edgesMatch(const Edge& a, const Edge& b) const
{
    // Рёбра совпадают если они обратны друг другу и близко находятся
    const double tol2 = m_tolerance * m_tolerance;
    // Прямое совпадение с обратной ориентацией: a.start ≈ b.end AND a.end ≈ b.start
    if (GeomUtils::dist2(a.start, b.end)   <= tol2 &&
        GeomUtils::dist2(a.end,   b.start) <= tol2)
        return true;
    return false;
}

std::vector<CoedgeMatch>
CoedgeDetector::findCoedges(const PartGeometry& geomA,
                              const QPointF& posA, double rotA,
                              const PartGeometry& geomB,
                              const QPointF& posB, double rotB) const
{
    auto edgesA = extractEdges(geomA, posA, rotA);
    auto edgesB = extractEdges(geomB, posB, rotB);

    std::vector<CoedgeMatch> matches;

    for (int i = 0; i < (int)edgesA.size(); ++i) {
        for (int j = 0; j < (int)edgesB.size(); ++j) {
            if (edgesMatch(edgesA[i], edgesB[j])) {
                // Вычисляем длину и экономию
                const double len = edgesA[i].length();
                const double savedArea = len * 0.0; // точная оценка зависит от контекста

                CoedgeMatch m;
                m.partIdA   = geomA.partId;
                m.partIdB   = geomB.partId;
                m.edgeIndexA = i;
                m.edgeIndexB = j;
                m.edgeLength = len;
                m.savedAreaMm2 = savedArea;

                // Смещение: сдвинуть B так, чтобы edgeB совместился с edgeA
                m.offsetX = edgesA[i].start.x() - edgesB[j].end.x();
                m.offsetY = edgesA[i].start.y() - edgesB[j].end.y();

                matches.push_back(m);

                AppLogger::instance().info(LogChannel::COEDGE,
                    QString("Coedge найден: %1 ребро#%2 ↔ %3 ребро#%4 (длина=%.2f мм)")
                    .arg(geomA.partId).arg(i)
                    .arg(geomB.partId).arg(j)
                    .arg(len));
            }
        }
    }

    return matches;
}

QPointF CoedgeDetector::applyCoedge(const CoedgeMatch& match,
                                     const QPointF& currentPosB) const
{
    return {currentPosB.x() + match.offsetX,
            currentPosB.y() + match.offsetY};
}
