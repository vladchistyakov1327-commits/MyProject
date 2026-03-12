#pragma once
#include "PolyContour.h"
#include <QString>
#include <vector>

/// Описание найденной общей кромки двух деталей.
struct CoedgeMatch {
    QString  partIdA;
    QString  partIdB;
    int      edgeIndexA = -1;   ///< Индекс ребра у детали A
    int      edgeIndexB = -1;   ///< Индекс ребра у детали B
    double   edgeLength = 0.0;  ///< Длина ребра в мм
    double   offsetX    = 0.0;  ///< Смещение детали B по X для совмещения
    double   offsetY    = 0.0;  ///< Смещение детали B по Y для совмещения
    double   savedAreaMm2 = 0.0;///< Экономия площади
};

/**
 * Детектор общих кромок (Coedge).
 *
 * Для двух деталей находит прямолинейные рёбра, которые могут быть совмещены
 * (нулевой зазор резки), вычисляет смещение второй детали.
 */
class CoedgeDetector
{
public:
    explicit CoedgeDetector(double toleranceMm = 0.5);

    /**
     * Найти общие кромки между деталью A и деталью B.
     * Деталь B уже размещена — её позиция не меняется.
     * Возвращает список совпадений (обычно 0 или 1 на пару).
     */
    std::vector<CoedgeMatch> findCoedges(const PartGeometry& geomA,
                                          const QPointF& posA, double rotA,
                                          const PartGeometry& geomB,
                                          const QPointF& posB, double rotB) const;

    /**
     * Применить coedge: вычислить скорректированную позицию детали B
     * для совмещения кромки edgeB с кромкой edgeA.
     */
    QPointF applyCoedge(const CoedgeMatch& match,
                        const QPointF& currentPosB) const;

    void setTolerance(double mm) { m_tolerance = mm; }
    double tolerance() const     { return m_tolerance; }

private:
    double m_tolerance;

    struct Edge {
        QPointF start;
        QPointF end;
        double  length() const;
    };

    std::vector<Edge> extractEdges(const PartGeometry& geom,
                                    const QPointF& pos, double rotDeg) const;

    bool edgesMatch(const Edge& a, const Edge& b) const;
};
