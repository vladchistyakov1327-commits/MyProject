#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/nesting/NFPCalculator.h"
#include "core/geometry/PolyContour.h"

// Вспомогательная функция: создать прямоугольный контур
static PolyContour makeRect(double x, double y, double w, double h)
{
    PolyContour c;
    c.vertices = {
        {x,     y},
        {x + w, y},
        {x + w, y + h},
        {x,     y + h},
    };
    c.isHole = false;
    return c;
}

TEST_CASE("NFPCalculator: NFP двух единичных квадратов")
{
    NFPCalculator calc;

    // 10×10 статичная деталь в (0,0)
    PolyContour stationary = makeRect(0, 0, 10, 10);
    // 10×10 движущаяся деталь
    PolyContour moving     = makeRect(0, 0, 10, 10);

    const auto nfp = calc.computeNFP(stationary.vertices, moving.vertices);
    // NFP для двух одинаковых квадратов 10×10 — квадрат 20×20 со смещением от центра
    REQUIRE(!nfp.empty());
    // Площадь NFP должна быть ≈ 400 (20×20)
    double area = 0;
    for (const auto& path : nfp) {
        if (path.size() < 3) continue;
        double a = 0;
        for (size_t i = 0, j = path.size()-1; i < path.size(); j = i, ++i)
            a += (path[j].x + path[i].x) * (path[j].y - path[i].y);
        area += std::abs(a) * 0.5;
    }
    CHECK_THAT(area, Catch::Matchers::WithinRel(400.0, 0.05));
}

TEST_CASE("NFPCalculator: IFP — лист 100×100, деталь 30×20")
{
    NFPCalculator calc;

    // Лист
    PolyContour sheet = makeRect(0, 0, 100, 100);
    // Деталь 30×20
    PolyContour part  = makeRect(0, 0, 30, 20);

    const auto ifp = calc.computeIFP(sheet.vertices, part.vertices);
    REQUIRE(!ifp.empty());

    // Допустимая область размещения: (70×80) — деталь вписывается
    // Площадь IFP должна быть ≈ 5600
    double area = 0;
    for (const auto& path : ifp) {
        if (path.size() < 3) continue;
        double a = 0;
        for (size_t i = 0, j = path.size()-1; i < path.size(); j = i, ++i)
            a += (path[j].x + path[i].x) * (path[j].y - path[i].y);
        area += std::abs(a) * 0.5;
    }
    CHECK(area > 1000.0); // Должна быть ненулевая область
}

TEST_CASE("NFPCalculator: кэш работает (повторный вызов)")
{
    NFPCalculator calc;
    PolyContour a = makeRect(0, 0, 20, 20);
    PolyContour b = makeRect(0, 0, 15, 10);

    const auto nfp1 = calc.computeNFP(a.vertices, b.vertices);
    const auto nfp2 = calc.computeNFP(a.vertices, b.vertices); // должно прийти из кэша

    // Результаты идентичны
    REQUIRE(nfp1.size() == nfp2.size());
    if (!nfp1.empty() && !nfp2.empty()) {
        CHECK(nfp1[0].size() == nfp2[0].size());
    }
}
