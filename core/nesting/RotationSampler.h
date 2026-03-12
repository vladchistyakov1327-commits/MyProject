#pragma once
#include "PlacementStrategy.h"
#include <vector>

/**
 * Генерирует углы вращения для детали согласно RotationMode и шагу.
 */
class RotationSampler
{
public:
    /// Вернуть список углов (в градусах) для заданного режима и шага.
    static std::vector<double> sample(RotationMode mode,
                                       double stepDeg = 90.0,
                                       bool allowFlip = false);

    /// Нормализовать угол в диапазон [0.0, 360.0).
    static double normalize(double deg);
};
