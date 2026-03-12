#include "RotationSampler.h"
#include <cmath>
#include <algorithm>

double RotationSampler::normalize(double deg)
{
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0) deg += 360.0;
    return deg;
}

std::vector<double> RotationSampler::sample(RotationMode mode,
                                              double stepDeg,
                                              bool allowFlip)
{
    std::vector<double> angles;

    switch (mode) {
        case RotationMode::NONE:
            angles = {0.0};
            break;

        case RotationMode::STEP_90:
            angles = {0.0, 90.0, 180.0, 270.0};
            break;

        case RotationMode::STEP_45:
            angles = {0.0, 45.0, 90.0, 135.0, 180.0, 225.0, 270.0, 315.0};
            break;

        case RotationMode::CUSTOM: {
            if (stepDeg <= 0.0 || stepDeg > 360.0) stepDeg = 90.0;
            for (double a = 0.0; a < 360.0 - 1e-6; a += stepDeg)
                angles.push_back(normalize(a));
            break;
        }
    }

    if (allowFlip) {
        // Добавляем зеркальные версии (отражение + вращение)
        const std::size_t baseCount = angles.size();
        for (std::size_t i = 0; i < baseCount; ++i)
            angles.push_back(angles[i] + 360.0); // флаг зеркала: angle >= 360
    }

    // Убрать дубликаты
    std::sort(angles.begin(), angles.end());
    angles.erase(std::unique(angles.begin(), angles.end()), angles.end());

    return angles;
}
