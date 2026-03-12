#include "DxfExporter.h"
#include "core/logging/AppLogger.h"
#include "core/logging/LogChannel.h"
#include <QFile>
#include <QTextStream>
#include <cmath>

// ── Вспомогательные функции генерации DXF ────────────────────────────────────

static void writeDxfHeader(QTextStream& ts)
{
    ts << "  0\nSECTION\n"
       << "  2\nHEADER\n"
       << "  9\n$ACADVER\n  1\nAC1015\n"
       << "  0\nENDSEC\n";
}

static void writeDxfTablesSection(QTextStream& ts)
{
    ts << "  0\nSECTION\n  2\nTABLES\n"
       << "  0\nTABLE\n  2\nLAYER\n  70\n1\n"
       << "  0\nLAYER\n  2\n0\n  70\n0\n  62\n7\n  6\nCONTINUOUS\n"
       << "  0\nENDTAB\n"
       << "  0\nENDSEC\n";
}

static void writeLWPolyline(QTextStream& ts,
                             const std::vector<QPointF>& pts,
                             bool closed,
                             const QString& layer = "0")
{
    ts << "  0\nLWPOLYLINE\n"
       << "  8\n" << layer << "\n"
       << " 90\n" << pts.size() << "\n"
       << " 70\n" << (closed ? "1" : "0") << "\n";
    for (const auto& p : pts) {
        ts << " 10\n" << QString::number(p.x(), 'f', 4) << "\n"
           << " 20\n" << QString::number(p.y(), 'f', 4) << "\n";
    }
}

// ── DxfExporter ───────────────────────────────────────────────────────────────

QByteArray DxfExporter::exportGeometryToBytes(const PartGeometry& geom) const
{
    QByteArray result;
    QTextStream ts(&result, QIODevice::WriteOnly);
    ts.setEncoding(QStringConverter::Utf8);

    writeDxfHeader(ts);
    writeDxfTablesSection(ts);

    ts << "  0\nSECTION\n  2\nENTITIES\n";

    // Внешний контур
    if (!geom.outerContour.isEmpty()) {
        writeLWPolyline(ts, geom.outerContour.vertices, true, "0");
    }
    // Отверстия
    for (const auto& hole : geom.holes) {
        if (!hole.isEmpty()) {
            writeLWPolyline(ts, hole.vertices, true, "HOLES");
        }
    }

    ts << "  0\nENDSEC\n"
       << "  0\nEOF\n";

    ts.flush();
    return result;
}

bool DxfExporter::exportLayoutToFile(const NestResult& result,
                                      const NestJob& job,
                                      const QString& outputPath) const
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = QString("DxfExporter: не удалось открыть файл %1").arg(outputPath);
        AppLogger::instance().error(LogChannel::EXPORT, m_lastError);
        return false;
    }

    QTextStream ts(&file);
    ts.setEncoding(QStringConverter::Utf8);

    writeDxfHeader(ts);
    writeDxfTablesSection(ts);
    ts << "  0\nSECTION\n  2\nENTITIES\n";

    // Рамка листа
    const double W = job.sheet.widthMm;
    const double H = job.sheet.heightMm;
    std::vector<QPointF> sheetRect = {{0,0},{W,0},{W,H},{0,H},{0,0}};
    writeLWPolyline(ts, sheetRect, true, "SHEET");

    // Все размещённые детали
    for (const auto& sr : result.sheets) {
        for (const auto& pp : sr.placedParts) {
            // Трансформированный контур уже в pp.placedContour
            std::vector<QPointF> pts;
            for (const auto& p : pp.placedContour)
                pts.push_back(p);
            writeLWPolyline(ts, pts, true, "PARTS");
        }
    }

    ts << "  0\nENDSEC\n"
       << "  0\nEOF\n";
    file.close();

    AppLogger::instance().info(LogChannel::EXPORT,
        QString("DXF раскладки сохранён: %1").arg(outputPath));
    return true;
}

bool DxfExporter::exportLayoutToFile(const NestResult& result,
                                      const QString& outputPath) const
{
    NestJob emptyJob;
    return exportLayoutToFile(result, emptyJob, outputPath);
}
