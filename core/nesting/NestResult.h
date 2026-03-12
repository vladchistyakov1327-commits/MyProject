#pragma once
#include "core/logging/LogEntry.h"
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QDateTime>
#include <QString>
#include <vector>

/// Размещённый экземпляр детали на листе.
struct PlacedPart {
    QString    partId;           ///< ID типа детали
    QString    instanceId;       ///< partId + "_" + порядковый номер
    QPointF    positionMm;       ///< Позиция origin на листе (мм)
    double     rotationDeg = 0.0;///< Применённый угол поворота
    bool       flipped     = false; ///< Зеркальное отражение
    int        sheetIndex  = 0;  ///< На каком листе (0-based)
    QPolygonF  placedContour;    ///< Трансформированный контур (для отрисовки)
};

/// Результат раскладки на одном листе.
struct SheetResult {
    int    sheetIndex          = 0;
    double utilizationPercent  = 0.0; ///< % использования площади
    double usedAreaMm2         = 0.0;
    double totalAreaMm2        = 0.0;
    double wasteAreaMm2        = 0.0;
    int    partsCount          = 0;
    std::vector<PlacedPart> placedParts;
    QRectF sheetBounds;
};

/// Итоговый результат нестинга.
struct NestResult {
    QString    jobId;
    bool       success           = false;
    bool       cancelled         = false;
    QString    errorMessage;
    std::vector<SheetResult> sheets;

    int        totalPlacedCount     = 0;
    int        totalPartsRequested  = 0;
    int        unplacedPartsCount   = 0;
    std::vector<QString> unplacedPartIds;

    double     avgUtilizationPct    = 0.0;
    QDateTime  calculatedAt;
    qint64     elapsedMs            = 0;
    std::vector<LogEntry> nestLog;
};
