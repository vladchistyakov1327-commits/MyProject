#include <catch2/catch_test_macros.hpp>
#include "core/export/LxdsExporter.h"
#include "core/export/DxfExporter.h"
#include "core/export/ReportGenerator.h"
#include "core/nesting/NestResult.h"
#include "core/geometry/PolyContour.h"
#include <QTemporaryFile>
#include <QPolygonF>
#include <QDir>
#include <QFile>

// Создать простой NestResult с одним листом и одной деталью
static NestResult makeSimpleResult()
{
    PlacedPart pp;
    pp.partId     = "PART_001";
    pp.instanceId = "PART_001_0";
    pp.positionMm = {10.0, 10.0};
    pp.rotationDeg= 0.0;
    pp.flipped    = false;
    pp.placedContour << QPointF(10,10) << QPointF(110,10)
                     << QPointF(110,60) << QPointF(10,60);

    SheetResult sr;
    sr.sheetIndex          = 0;
    sr.sheetBounds         = QRectF(0, 0, 300, 200);
    sr.utilizationPercent  = 8.33;
    sr.placedParts.push_back(pp);

    NestResult result;
    result.sheets.push_back(sr);
    result.totalPlacedCount = 1;
    result.cancelled        = false;
    return result;
}

TEST_CASE("LxdsExporter: создаёт непустой файл")
{
    const NestResult result = makeSimpleResult();

    QTemporaryFile tmpFile;
    tmpFile.setFileTemplate(QDir::tempPath() + "/test_XXXXXX.lxds");
    tmpFile.setAutoRemove(false);
    REQUIRE(tmpFile.open());
    const QString path = tmpFile.fileName();
    tmpFile.close();

    LxdsExporter exp;
    const bool ok = exp.exportToFile(result, path);

    INFO("LxdsExporter error: " << exp.lastError().toStdString());
    CHECK(ok);
    CHECK(QFile::exists(path));
    CHECK(QFile(path).size() > 0);

    QFile::remove(path);
}

TEST_CASE("DxfExporter: создаёт непустой DXF-файл")
{
    const NestResult result = makeSimpleResult();

    QTemporaryFile tmpFile;
    tmpFile.setFileTemplate(QDir::tempPath() + "/test_XXXXXX.dxf");
    tmpFile.setAutoRemove(false);
    REQUIRE(tmpFile.open());
    const QString path = tmpFile.fileName();
    tmpFile.close();

    DxfExporter exp;
    const bool ok = exp.exportLayoutToFile(result, path);

    INFO("DxfExporter error: " << exp.lastError().toStdString());
    CHECK(ok);

    QFile file(path);
    REQUIRE(file.open(QIODevice::ReadOnly));
    const QString content = QString::fromLatin1(file.readAll());
    file.close();

    // Должен содержать стандартный заголовок DXF
    CHECK(content.contains("SECTION"));
    CHECK(content.contains("ENTITIES"));
    CHECK(content.contains("LWPOLYLINE"));
    CHECK(content.contains("EOF"));

    QFile::remove(path);
}

TEST_CASE("ReportGenerator: CSV содержит ожидаемые данные")
{
    const NestResult result = makeSimpleResult();

    QTemporaryFile tmpFile;
    tmpFile.setFileTemplate(QDir::tempPath() + "/test_XXXXXX.csv");
    tmpFile.setAutoRemove(false);
    REQUIRE(tmpFile.open());
    const QString path = tmpFile.fileName();
    tmpFile.close();

    ReportGenerator gen;
    const bool ok = gen.generateCsv(result, path);
    CHECK(ok);

    QFile file(path);
    REQUIRE(file.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    CHECK(content.contains("PART_001"));
    CHECK(content.contains("1")); // кол-во деталей / лист

    QFile::remove(path);
}

TEST_CASE("ReportGenerator: TXT содержит заголовок отчёта")
{
    const NestResult result = makeSimpleResult();

    QTemporaryFile tmpFile;
    tmpFile.setFileTemplate(QDir::tempPath() + "/test_XXXXXX.txt");
    tmpFile.setAutoRemove(false);
    REQUIRE(tmpFile.open());
    const QString path = tmpFile.fileName();
    tmpFile.close();

    ReportGenerator gen;
    const bool ok = gen.generateText(result, path);
    CHECK(ok);

    QFile file(path);
    REQUIRE(file.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    CHECK(content.contains("Лист"));
    CHECK(content.contains("PART_001"));

    QFile::remove(path);
}
