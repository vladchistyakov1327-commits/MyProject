#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/geometry/DxfImporter.h"
#include <QTemporaryFile>
#include <QTextStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>

static QString writeTempDxf(const QString& content)
{
    QTemporaryFile f;
    f.setAutoRemove(false);
    f.setFileTemplate(QDir::tempPath() + "/test_XXXXXX.dxf");
    if (!f.open()) return {};
    QTextStream ts(&f);
    ts << content;
    f.close();
    return f.fileName();
}

static QString makeRectDxf(double w, double h)
{
    // Минимальный DXF с одним LWPOLYLINE (прямоугольник)
    return QString(
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLWPOLYLINE\n"
        "8\n0\n"           // layer
        "70\n1\n"          // CLOSED
        "90\n4\n"          // vertices count
        "10\n0.0\n20\n0.0\n"
        "10\n%1\n20\n0.0\n"
        "10\n%1\n20\n%2\n"
        "10\n0.0\n20\n%2\n"
        "0\nENDSEC\n0\nEOF\n"
    ).arg(w).arg(h);
}

TEST_CASE("DxfImporter: пустой файл — пустой результат")
{
    const QString path = writeTempDxf("0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");
    REQUIRE(!path.isEmpty());
    DxfImporter imp;
    const auto result = imp.import(path);
    CHECK(result.isEmpty());
    QFile::remove(path);
}

TEST_CASE("DxfImporter: прямоугольник 100×50 мм")
{
    const QString path = writeTempDxf(makeRectDxf(100.0, 50.0));
    REQUIRE(!path.isEmpty());
    DxfImporter imp;
    const auto result = imp.import(path);
    REQUIRE(result.size() == 1);
    const auto& geom = result.first();

    // Площадь ≈ 5000 мм²
    CHECK_THAT(geom.areaMm2,
               Catch::Matchers::WithinRel(5000.0, 0.01));

    // Bounding box ≈ 100 × 50
    CHECK_THAT(geom.boundingBox.width(),
               Catch::Matchers::WithinRel(100.0, 0.01));
    CHECK_THAT(geom.boundingBox.height(),
               Catch::Matchers::WithinRel(50.0, 0.01));

    // Контур должен быть замкнутым (4 вершины)
    CHECK(geom.outerContour.vertices.size() >= 4);

    QFile::remove(path);
}

TEST_CASE("DxfImporter: несуществующий файл — пустой результат")
{
    DxfImporter imp;
    const auto result = imp.import("/no/such/file.dxf");
    CHECK(result.isEmpty());
}

TEST_CASE("DxfImporter: LINE-прямоугольник")
{
    // 4 отдельные LINE-сегмента, образующие прямоугольник 200×100
    const QString dxf =
        "0\nSECTION\n2\nENTITIES\n"
        // Нижняя
        "0\nLINE\n8\n0\n10\n0\n20\n0\n30\n0\n11\n200\n21\n0\n31\n0\n"
        // Правая
        "0\nLINE\n8\n0\n10\n200\n20\n0\n30\n0\n11\n200\n21\n100\n31\n0\n"
        // Верхняя
        "0\nLINE\n8\n0\n10\n200\n20\n100\n30\n0\n11\n0\n21\n100\n31\n0\n"
        // Левая
        "0\nLINE\n8\n0\n10\n0\n20\n100\n30\n0\n11\n0\n21\n0\n31\n0\n"
        "0\nENDSEC\n0\nEOF\n";

    const QString path = writeTempDxf(dxf);
    REQUIRE(!path.isEmpty());
    DxfImporter imp;
    const auto result = imp.import(path);
    REQUIRE(!result.isEmpty());

    // Площадь ≈ 20000 мм²
    CHECK_THAT(result.first().areaMm2,
               Catch::Matchers::WithinRel(20000.0, 0.02));
    QFile::remove(path);
}
