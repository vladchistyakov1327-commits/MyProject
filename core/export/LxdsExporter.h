#pragma once
#include "core/nesting/NestResult.h"
#include "core/nesting/NestJob.h"
#include <QString>

/// Результат операции экспорта.
struct ExportResult {
    bool    success = false;
    QString filePath;
    QString errorMessage;
    qint64  fileSizeBytes = 0;
    int     sheetsExported = 0;
    int     partsExported  = 0;
};

/**
 * Экспорт раскладки в формат .lxds (CypCut).
 *
 * .lxds — это ZIP-архив (Deflate) с:
 *   layout.xml        — основной файл раскладки
 *   parts/partN.dxf   — геометрия каждой уникальной детали
 *
 * Координаты: Y-ось направлена ВВЕРХ (invertY = sheetHeight - y).
 * Поворот: по часовой стрелке (0..360).
 */
class LxdsExporter
{
public:
    ExportResult exportToFile(const NestResult& result,
                               const NestJob& job,
                               const QString& outputPath);

    /// Simplified overload without NestJob (no per-part DXF in ZIP).
    bool exportToFile(const NestResult& result, const QString& outputPath);

    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
};
