#pragma once
#include "core/geometry/PolyContour.h"
#include "core/nesting/NestResult.h"
#include "core/nesting/NestJob.h"
#include <QString>
#include <QByteArray>

/**
 * Экспорт в формат DXF.
 * Версия: AC1015 (R2000), LWPOLYLINE entity, слой "0".
 */
class DxfExporter
{
public:
    /// Экспортировать геометрию одной детали в DXF-байты (для .lxds).
    QByteArray exportGeometryToBytes(const PartGeometry& geom) const;

    /// Экспортировать всю раскладку в DXF файл.
    bool exportLayoutToFile(const NestResult& result,
                             const NestJob& job,
                             const QString& outputPath) const;

    /// Simplified overload without NestJob.
    bool exportLayoutToFile(const NestResult& result,
                             const QString& outputPath) const;

    QString lastError() const { return m_lastError; }

private:
    mutable QString m_lastError;
};
