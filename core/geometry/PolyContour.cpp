#include "PolyContour.h"
#include <cmath>
#include <algorithm>

double PolyContour::signedArea() const
{
    if (vertices.size() < 3) return 0.0;
    double area = 0.0;
    const int n = static_cast<int>(vertices.size());
    for (int i = 0; i < n; ++i) {
        const auto& p1 = vertices[i];
        const auto& p2 = vertices[(i + 1) % n];
        area += (p1.x() * p2.y()) - (p2.x() * p1.y());
    }
    return area * 0.5;
}

double PolyContour::area() const
{
    return std::abs(signedArea());
}

void PolyContour::reverse()
{
    std::reverse(vertices.begin(), vertices.end());
}

void PolyContour::fixOrientation()
{
    const double sa = signedArea();
    if (!isHole) {
        // Внешний контур: против часовой стрелки (sa > 0)
        if (sa < 0.0) reverse();
    } else {
        // Отверстие: по часовой стрелке (sa < 0)
        if (sa > 0.0) reverse();
    }
}
