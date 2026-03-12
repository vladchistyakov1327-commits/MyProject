#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/nesting/NestEngine.h"
#include "core/nesting/NestJob.h"
#include "core/nesting/NestResult.h"
#include "core/geometry/PolyContour.h"

// Создать PartEntry с прямоугольным контуром
static PartEntry makeRectPart(const QString& id, double w, double h, int qty = 1)
{
    PolyContour c;
    c.vertices = {{0,0},{w,0},{w,h},{0,h}};
    c.isHole   = false;

    PartGeometry g;
    g.partId      = id;
    g.sourceName  = id;
    g.outerContour = c;
    g.areaMm2      = w * h;
    g.boundingBox  = QRectF(0, 0, w, h);

    PartEntry pe;
    pe.geometry     = g;
    pe.name         = id;
    pe.quantity     = qty;
    pe.rotationMode = RotationMode::NONE;
    return pe;
}

TEST_CASE("NestEngine: 2 детали 100×100 на листе 300×300 — все размещены")
{
    NestJob job;
    job.sheet.type     = SheetDefinition::Type::RECTANGLE;
    job.sheet.widthMm  = 300.0;
    job.sheet.heightMm = 300.0;
    job.sheet.quantity = 1;

    job.params.partSpacingMm  = 1.0;
    job.params.sheetMarginMm  = 5.0;
    job.params.enableRotation = false;
    job.params.algorithm      = NestAlgorithm::BOTTOM_LEFT_NFP;

    job.parts.push_back(makeRectPart("A", 100, 100, 2));

    NestEngine engine;
    const NestResult result = engine.runSync(job);

    CHECK(!result.cancelled);
    REQUIRE(!result.sheets.empty());
    CHECK(result.totalPlacedCount == 2);
    CHECK(result.unplacedPartIds.empty());

    // Утилизация: 2 * 10000 = 20000 из 300*300 = 90000 → ≈22%
    CHECK_THAT(result.sheets.front().utilizationPercent,
               Catch::Matchers::WithinRel(22.2, 0.1));
}

TEST_CASE("NestEngine: деталь больше листа — не размещена")
{
    NestJob job;
    job.sheet.type     = SheetDefinition::Type::RECTANGLE;
    job.sheet.widthMm  = 50.0;
    job.sheet.heightMm = 50.0;
    job.sheet.quantity = 1;

    job.params.partSpacingMm  = 1.0;
    job.params.sheetMarginMm  = 5.0;
    job.params.enableRotation = false;

    job.parts.push_back(makeRectPart("Big", 200, 200, 1));

    NestEngine engine;
    const NestResult result = engine.runSync(job);

    CHECK(result.totalPlacedCount == 0);
    CHECK(result.unplacedPartIds.size() == 1);
}

TEST_CASE("NestEngine: без ограничения листов — несколько листов")
{
    NestJob job;
    job.sheet.type     = SheetDefinition::Type::RECTANGLE;
    job.sheet.widthMm  = 110.0;
    job.sheet.heightMm = 110.0;
    job.sheet.quantity = 0; // бесконечно

    job.params.partSpacingMm  = 2.0;
    job.params.sheetMarginMm  = 5.0;
    job.params.enableRotation = false;

    // 5 деталей 100×100, на лист помещается 1 (110 - 2*5 = 100 → ровно)
    job.parts.push_back(makeRectPart("P", 100, 100, 5));

    NestEngine engine;
    const NestResult result = engine.runSync(job);

    CHECK(result.totalPlacedCount == 5);
    CHECK(result.unplacedPartIds.empty());
    // Нужно несколько листов
    CHECK(result.sheets.size() >= 2);
}

TEST_CASE("NestEngine: отмена расчёта")
{
    NestJob job;
    job.sheet.type     = SheetDefinition::Type::RECTANGLE;
    job.sheet.widthMm  = 1000.0;
    job.sheet.heightMm = 1000.0;
    job.sheet.quantity = 0;

    job.params.enableRotation = true;
    job.params.partSpacingMm  = 1.0;

    // Много деталей
    for (int i = 0; i < 50; ++i)
        job.parts.push_back(makeRectPart(QString("P%1").arg(i), 80, 80, 1));

    NestEngine engine;
    // Запускаем асинхронно, сразу отменяем
    auto future = engine.runAsync(job);
    engine.cancel();
    future.waitForFinished();

    const auto result = future.result();
    CHECK(result.cancelled);
}
