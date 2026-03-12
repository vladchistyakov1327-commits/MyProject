#pragma once
#include <QPointF>
#include <QRectF>
#include <QString>
#include <vector>

/// Нормализованный контур детали (внешний контур или отверстие).
struct PolyContour {
    std::vector<QPointF> vertices;               ///< Полигон (аппроксимация всех кривых)
    bool                 isHole = false;          ///< true = отверстие внутри детали
    double               approximationTolerance = 0.01; ///< мм, использованный при аппроксимации

    bool isEmpty() const { return vertices.empty(); }

    /// Площадь контура со знаком (< 0 → обход по часовой, > 0 → против).
    double signedArea() const;

    double area() const;

    /// Перевернуть порядок обхода вершин.
    void reverse();

    /// Ориентировать контур: внешний — против часовой, отверстие — по часовой.
    void fixOrientation();
};

/// Полная геометрия одной детали (результат импорта / обработки).
struct PartGeometry {
    QString              partId;        ///< Уникальный ID
    QString              sourceName;    ///< Имя файла DXF
    PolyContour          outerContour;  ///< Внешний контур
    std::vector<PolyContour> holes;     ///< Внутренние отверстия
    QRectF               boundingBox;
    double               areaMm2 = 0.0;
    QString              sourceLayer;  ///< Из какого слоя DXF

    bool isValid() const { return !outerContour.isEmpty() && areaMm2 > 1e-9; }
};
