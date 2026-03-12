#pragma once
#include "PolyContour.h"
#include "core/logging/LogEntry.h"
#include <QString>
#include <QStringList>
#include <QList>
#include <vector>

/// Опции импорта DXF файла.
struct ImportOptions {
    double   toleranceMm      = 0.1;   ///< Допуск сборки контуров (gap)
    double   approxToleranceMm = 0.01; ///< Точность аппроксимации кривых
    QString  filterLayer;              ///< Если задан — импортировать только этот слой
    bool     autoClose        = true;  ///< Автозакрывать незамкнутые контуры
    bool     detectUnits      = true;  ///< Автодетектировать единицы измерения
    bool     importAsOnepart  = false; ///< Всё содержимое файла — одна деталь
};

/// Результат импорта одного DXF файла.
struct ImportResult {
    bool                      success      = false;
    std::vector<PartGeometry> parts;
    std::vector<LogEntry>     log;          ///< Записи лога (не фатальные предупреждения)
    QString                   errorMessage; ///< Если !success
    QString                   dxfVersion;   ///< AC1009 .. AC1032
    QString                   units;        ///< "mm" / "in" / "m" / "unknown"
    int                       entityCount   = 0;
    int                       skippedCount  = 0;
};

/**
 * Парсер DXF файлов через libdxfrw.
 *
 * Поддерживаемые entity: LINE, ARC, CIRCLE, LWPOLYLINE, POLYLINE,
 * SPLINE, ELLIPSE, INSERT (блоки с трансформацией).
 * Сборка разрозненных сегментов в замкнутые контуры.
 */
class DxfImporter
{
public:
    DxfImporter() = default;

    /// Импортировать DXF файл.
    ImportResult importFile(const QString& path,
                            const ImportOptions& opts = ImportOptions{});

    /// Импортировать из памяти (для тестов).
    ImportResult importData(const QByteArray& data,
                            const QString& sourceName,
                            const ImportOptions& opts = ImportOptions{});

    /// Удобный метод: импортировать файл и вернуть список деталей напрямую.
    /// Возвращает пустой список при ошибке.
    QList<PartGeometry> import(const QString& path,
                               const ImportOptions& opts = ImportOptions{});
};
