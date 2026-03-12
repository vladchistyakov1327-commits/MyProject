#pragma once
#include "PlacementStrategy.h"
#include "core/geometry/PolyContour.h"
#include <QString>
#include <QDateTime>
#include <QUuid>
#include <vector>

/// Описание одного типа детали в задании.
struct PartEntry {
    PartGeometry  geometry;           ///< Геометрия детали
    int           quantity     = 1;   ///< Количество (1..10000)
    QString       name;               ///< Пользовательское имя
    bool          allowFlip    = false;///< Разрешён переворот (зеркало)
    RotationMode  rotationMode = RotationMode::STEP_90;
    double        customRotStep = 45.0; ///< Шаг в градусах для CUSTOM
};

/// Описание листа-заготовки.
struct SheetDefinition {
    enum class Type { RECTANGLE, CUSTOM_DXF };

    Type     type         = Type::RECTANGLE;
    double   widthMm      = 3000.0;  ///< Для RECTANGLE
    double   heightMm     = 1500.0;  ///< Для RECTANGLE
    PartGeometry customShape;        ///< Для CUSTOM_DXF — контур листа
    int      quantity     = 0;       ///< Кол-во листов (0 = бесконечно)
    QString  materialId;
    double   thicknessMm  = 3.0;
};

/// Параметры алгоритма раскладки.
struct NestParameters {
    // Зазоры
    double   partSpacingMm  = 2.0;   ///< Расстояние между деталями (мм)
    double   sheetMarginMm  = 5.0;   ///< Отступ от края листа (мм)

    // Стратегия
    PlacementStrategy strategy = PlacementStrategy::BOTTOM_LEFT;

    // Вращение
    bool     enableRotation  = true;
    double   globalRotStepDeg = 90.0; ///< Шаг вращения по умолчанию

    // Coedge
    bool     enableCoedge    = false;
    double   coedgeTolerance = 0.5;  ///< Допуск совпадения кромок (мм)

    // Алгоритм
    NestAlgorithm algorithm     = NestAlgorithm::BOTTOM_LEFT_NFP;
    int           maxIterations = 1000;
    int           timeLimitSec  = 0;  ///< 0 = без ограничения
    int           threadCount   = 0;  ///< 0 = auto
};

/// Полное задание нестинга.
struct NestJob {
    std::vector<PartEntry> parts;
    SheetDefinition        sheet;
    NestParameters         params;
    QString                jobId      = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QDateTime              createdAt  = QDateTime::currentDateTime();
};
