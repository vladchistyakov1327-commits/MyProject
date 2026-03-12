#pragma once
#include "core/nesting/NestResult.h"
#include "core/nesting/NestJob.h"
#include <QString>

/**
 * Генерирует текстовый/CSV отчёт по результатам нестинга.
 */
class ReportGenerator
{
public:
    /// Сгенерировать CSV отчёт и сохранить в файл.
    bool generateCsv(const NestResult& result,
                     const NestJob& job,
                     const QString& outputPath) const;
    bool generateCsv(const NestResult& result,
                     const QString& outputPath) const;

    /// Вернуть отчёт как строку.
    QString generateText(const NestResult& result, const NestJob& job) const;

    /// Сохранить текстовый отчёт в файл, вернуть успех.
    bool generateText(const NestResult& result, const QString& outputPath) const;
};
