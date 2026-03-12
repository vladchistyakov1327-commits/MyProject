#pragma once
#include "core/geometry/PolyContour.h"
#include "core/nesting/PlacementStrategy.h"
#include <clipper2/clipper.h>
#include <QPointF>
#include <QString>
#include <vector>
#include <functional>
#include <optional>
#include <unordered_map>

/// Ключ кэша NFP.
struct NFPKey {
    std::size_t hashA;
    std::size_t hashB;
    double      rotA;
    double      rotB;

    bool operator==(const NFPKey& o) const {
        return hashA == o.hashA && hashB == o.hashB &&
               std::abs(rotA - o.rotA) < 0.001 &&
               std::abs(rotB - o.rotB) < 0.001;
    }
};

struct NFPKeyHash {
    std::size_t operator()(const NFPKey& k) const {
        std::size_t h = k.hashA;
        h ^= k.hashB + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(static_cast<int>(k.rotA * 10)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(static_cast<int>(k.rotB * 10)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

/**
 * Вычислитель No-Fit Polygon через Clipper2 Minkowski Sum.
 *
 * NFP(A, B) определяет все позиции origin детали B,
 * при которых B перекрывается с A.
 *
 * Кэшируется по паре (hashA, hashB, rotA, rotB).
 */
class NFPCalculator
{
public:
    NFPCalculator() = default;

    /**
     * Вычислить NFP(fixedPart, movingPart).
     * @param fixedPts  Контур фиксированной детали A (мировые координаты)
     * @param movingPts Контур размещаемой детали B (локальные, центрированные)
     * @return Пути NFP (запрещённые позиции origin B)
     */
    Clipper2Lib::PathsD computeNFP(const std::vector<QPointF>& fixedPts,
                                    const std::vector<QPointF>& movingPts) const;

    /**
     * Вычислить Inner Fit Polygon — допустимые позиции origin B внутри листа.
     * @param sheetPts  Контур листа (после отступа margin)
     * @param movingPts Контур размещаемой детали B (локальные)
     */
    Clipper2Lib::PathsD computeIFP(const std::vector<QPointF>& sheetPts,
                                    const std::vector<QPointF>& movingPts) const;

    /**
     * Вычислить допустимую область для origin детали B:
     *   допустимая = IFP(sheet, B) \ Union(NFP(A_i, B)) для всех A_i
     */
    Clipper2Lib::PathsD computeValidRegion(
        const Clipper2Lib::PathsD& ifp,
        const Clipper2Lib::PathsD& nfpUnion) const;

    /// Найти лучшую позицию из допустимой области согласно стратегии.
    std::optional<QPointF> findBestPosition(
        const Clipper2Lib::PathsD& validRegion,
        PlacementStrategy strategy,
        const QRectF& sheetBounds) const;

    /// Хэш контура для кэширования.
    static std::size_t hashContour(const std::vector<QPointF>& pts);

    void clearCache() { m_cache.clear(); }
    std::size_t cacheSize() const { return m_cache.size(); }

private:
    mutable std::unordered_map<NFPKey, Clipper2Lib::PathsD, NFPKeyHash> m_cache;

    Clipper2Lib::PathD toPathD(const std::vector<QPointF>& pts) const;
    std::vector<QPointF> fromPathD(const Clipper2Lib::PathD& path) const;
};
