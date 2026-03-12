#include "ReportGenerator.h"
#include "core/logging/AppLogger.h"
#include "core/logging/LogChannel.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

bool ReportGenerator::generateCsv(const NestResult& result,
                                    const NestJob& job,
                                    const QString& outputPath) const
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        AppLogger::instance().error(LogChannel::EXPORT,
            QString("ReportGenerator: не удалось открыть файл %1").arg(outputPath));
        return false;
    }

    QTextStream ts(&file);
    ts.setEncoding(QStringConverter::Utf8);

    // Заголовок
    ts << "# NestingApp Report\n";
    ts << "# Сгенерирован: " << QDateTime::currentDateTime().toString() << "\n";
    ts << "# Job ID: " << result.jobId << "\n\n";

    // Сводка
    ts << "СВОДКА\n";
    ts << "Листов использовано," << result.sheets.size() << "\n";
    ts << "Деталей размещено," << result.totalPlacedCount << "\n";
    ts << "Деталей запрошено," << result.totalPartsRequested << "\n";
    ts << "Не размещено," << result.unplacedPartsCount << "\n";
    ts << "Средняя утилизация %," << QString::number(result.avgUtilizationPct, 'f', 1) << "\n";
    ts << "Время расчёта мс," << result.elapsedMs << "\n\n";

    // По листам
    ts << "ЛИСТЫ\n";
    ts << "Лист,Деталей,Утилизация %,Использовано мм²,Отходы мм²\n";
    for (const auto& sr : result.sheets) {
        ts << (sr.sheetIndex + 1) << ","
           << sr.partsCount << ","
           << QString::number(sr.utilizationPercent, 'f', 1) << ","
           << QString::number(sr.usedAreaMm2,    'f', 0) << ","
           << QString::number(sr.wasteAreaMm2,   'f', 0) << "\n";
    }
    ts << "\n";

    // Размещённые детали
    ts << "ДЕТАЛИ\n";
    ts << "Лист,ID экземпляра,ID типа,X мм,Y мм,Поворот °,Зеркало\n";
    for (const auto& sr : result.sheets) {
        for (const auto& pp : sr.placedParts) {
            ts << (pp.sheetIndex + 1) << ","
               << pp.instanceId << ","
               << pp.partId << ","
               << QString::number(pp.positionMm.x(), 'f', 4) << ","
               << QString::number(pp.positionMm.y(), 'f', 4) << ","
               << QString::number(pp.rotationDeg,    'f', 1) << ","
               << (pp.flipped ? "да" : "нет") << "\n";
        }
    }

    file.close();

    AppLogger::instance().info(LogChannel::EXPORT,
        QString("CSV отчёт сохранён: %1").arg(outputPath));
    return true;
}

QString ReportGenerator::generateText(const NestResult& result,
                                       const NestJob& job) const
{
    QString text;
    QTextStream ts(&text);

    ts << "═══════════════════════════════════════\n";
    ts << "  NestingApp — Отчёт о раскладке\n";
    ts << "═══════════════════════════════════════\n";
    ts << "Job ID:       " << result.jobId << "\n";
    ts << "Дата:         " << result.calculatedAt.toString("dd.MM.yyyy HH:mm:ss") << "\n";
    ts << "Время расчёта:" << result.elapsedMs << " мс\n\n";

    ts << "Листов использовано: " << result.sheets.size() << "\n";
    ts << "Деталей размещено:   " << result.totalPlacedCount
       << " / " << result.totalPartsRequested << "\n";
    ts << "Не размещено:        " << result.unplacedPartsCount << "\n";
    ts << "Средняя утилизация:  "
       << QString::number(result.avgUtilizationPct, 'f', 1) << "%\n\n";

    ts << "─────── Листы ───────────────────────\n";
    for (const auto& sr : result.sheets) {
        ts << QString("Лист %-3d: %4 дет.  Утилизация: %5%\n")
              .arg(sr.sheetIndex + 1)
              .arg(sr.partsCount)
              .arg(QString::number(sr.utilizationPercent, 'f', 1));
    }

    if (!result.unplacedPartIds.empty()) {
        ts << "\n─────── Не размещены ───────────────\n";
        for (const auto& id : result.unplacedPartIds)
            ts << "  • " << id << "\n";
    }

    ts << "═══════════════════════════════════════\n";
    return text;
}

bool ReportGenerator::generateCsv(const NestResult& result,
                                    const QString& outputPath) const
{
    return generateCsv(result, NestJob{}, outputPath);
}

bool ReportGenerator::generateText(const NestResult& result,
                                    const QString& outputPath) const
{
    const QString text = generateText(result, NestJob{});
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        AppLogger::instance().error(LogChannel::EXPORT,
            QString("ReportGenerator: не удалось открыть файл %1").arg(outputPath));
        return false;
    }
    QTextStream ts(&file);
    ts.setEncoding(QStringConverter::Utf8);
    ts << text;
    file.close();
    AppLogger::instance().info(LogChannel::EXPORT,
        QString("TXT отчёт сохранён: %1").arg(outputPath));
    return true;
}
