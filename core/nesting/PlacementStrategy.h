#pragma once

/// Директивы направления раскладки деталей на листе.
enum class PlacementStrategy {
    BOTTOM_LEFT,    ///< Снизу-вверх, слева-направо (классика)
    BOTTOM_RIGHT,   ///< Снизу-вверх, справа-налево
    TOP_LEFT,       ///< Сверху-вниз, слева-направо
    TOP_RIGHT,      ///< Сверху-вниз, справа-налево
    LEFT_BOTTOM,    ///< Слева-направо, снизу-вверх
    LEFT_TOP,       ///< Слева-направо, сверху-вниз
    RIGHT_BOTTOM,   ///< Справа-налево, снизу-вверх
    RIGHT_TOP,      ///< Справа-налево, сверху-вниз
    GRAVITY_CENTER, ///< К центру листа
    GRAVITY_EDGE    ///< К ближайшему краю
};

/// Режим вращения детали.
enum class RotationMode {
    NONE,        ///< Вращение запрещено
    STEP_90,     ///< 0, 90, 180, 270
    STEP_45,     ///< 0, 45, 90, ..., 315
    CUSTOM       ///< Произвольный шаг (customRotStep)
};

/// Алгоритм нестинга.
enum class NestAlgorithm {
    BOTTOM_LEFT_NFP,       ///< Bottom-Left с NFP (основной)
    GENETIC,               ///< Генетический алгоритм (заглушка)
    SIMULATED_ANNEALING    ///< Имитация отжига (заглушка)
};

/// Строковые описания стратегий для UI.
inline const char* strategyDescription(PlacementStrategy s) {
    switch (s) {
        case PlacementStrategy::BOTTOM_LEFT:    return "Снизу-вверх, слева-направо";
        case PlacementStrategy::BOTTOM_RIGHT:   return "Снизу-вверх, справа-налево";
        case PlacementStrategy::TOP_LEFT:       return "Сверху-вниз, слева-направо";
        case PlacementStrategy::TOP_RIGHT:      return "Сверху-вниз, справа-налево";
        case PlacementStrategy::LEFT_BOTTOM:    return "Слева-направо, снизу-вверх";
        case PlacementStrategy::LEFT_TOP:       return "Слева-направо, сверху-вниз";
        case PlacementStrategy::RIGHT_BOTTOM:   return "Справа-налево, снизу-вверх";
        case PlacementStrategy::RIGHT_TOP:      return "Справа-налево, сверху-вниз";
        case PlacementStrategy::GRAVITY_CENTER: return "К центру листа";
        case PlacementStrategy::GRAVITY_EDGE:   return "К ближайшему краю";
    }
    return "";
}
