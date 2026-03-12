#include "LxdsExporter.h"
#include "DxfExporter.h"
#include "core/logging/AppLogger.h"
#include "core/logging/LogChannel.h"

#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QXmlStreamWriter>
#include <QBuffer>
#include <QFileInfo>

// Qt не имеет встроенного QZipWriter в публичном API.
// Используем минимальную реализацию ZIP через quazip или записываем
// временный каталог и архивируем через QProcess.
// Для максимальной переносимости — реализуем ZIP вручную (deflate via zlib).
// Простой вариант без сжатия (stored):

#include <zlib.h>

// ── Минимальный ZIP writer ────────────────────────────────────────────────────

struct ZipEntry {
    QString  name;
    QByteArray data;
};

static QByteArray buildZip(const std::vector<ZipEntry>& entries)
{
    // Local file header signature: 0x04034b50
    // Central directory signature: 0x02014b50
    // EOCD signature: 0x06054b50

    auto writeU16 = [](QByteArray& buf, quint16 v) {
        buf.append(static_cast<char>(v & 0xFF));
        buf.append(static_cast<char>((v >> 8) & 0xFF));
    };
    auto writeU32 = [](QByteArray& buf, quint32 v) {
        buf.append(static_cast<char>(v & 0xFF));
        buf.append(static_cast<char>((v >> 8) & 0xFF));
        buf.append(static_cast<char>((v >> 16) & 0xFF));
        buf.append(static_cast<char>((v >> 24) & 0xFF));
    };

    QByteArray localFiles;
    QByteArray centralDir;

    struct EntryInfo {
        quint32 offset;
        quint32 crc32;
        quint32 compressedSize;
        quint32 uncompressedSize;
        QByteArray nameBytes;
        QByteArray compressedData;
    };

    std::vector<EntryInfo> infos;
    infos.reserve(entries.size());

    for (const auto& e : entries) {
        EntryInfo info;
        info.offset            = static_cast<quint32>(localFiles.size());
        info.uncompressedSize  = static_cast<quint32>(e.data.size());
        info.nameBytes         = e.name.toUtf8();

        // CRC32
        info.crc32 = static_cast<quint32>(
            crc32(0L, reinterpret_cast<const Bytef*>(e.data.constData()),
                  static_cast<uInt>(e.data.size())));

        // Deflate compress
        uLongf compLen = compressBound(static_cast<uLong>(e.data.size()));
        info.compressedData.resize(static_cast<int>(compLen));
        compress2(reinterpret_cast<Bytef*>(info.compressedData.data()),
                  &compLen,
                  reinterpret_cast<const Bytef*>(e.data.constData()),
                  static_cast<uLong>(e.data.size()),
                  Z_DEFAULT_COMPRESSION);
        info.compressedData.resize(static_cast<int>(compLen));
        // Strip zlib header/trailer (2 bytes header, 4 bytes adler32 at end)
        if (info.compressedData.size() > 6) {
            info.compressedData = info.compressedData.mid(2, info.compressedData.size() - 6);
        }
        info.compressedSize = static_cast<quint32>(info.compressedData.size());

        // Local file header
        writeU32(localFiles, 0x04034b50);
        writeU16(localFiles, 20);          // version needed
        writeU16(localFiles, 0);           // flags
        writeU16(localFiles, 8);           // compression: deflate
        writeU16(localFiles, 0);           // mod time
        writeU16(localFiles, 0);           // mod date
        writeU32(localFiles, info.crc32);
        writeU32(localFiles, info.compressedSize);
        writeU32(localFiles, info.uncompressedSize);
        writeU16(localFiles, static_cast<quint16>(info.nameBytes.size()));
        writeU16(localFiles, 0);           // extra length
        localFiles.append(info.nameBytes);
        localFiles.append(info.compressedData);

        infos.push_back(std::move(info));
    }

    const quint32 centralDirOffset = static_cast<quint32>(localFiles.size());

    for (const auto& info : infos) {
        writeU32(centralDir, 0x02014b50);
        writeU16(centralDir, 20);          // version made by
        writeU16(centralDir, 20);          // version needed
        writeU16(centralDir, 0);           // flags
        writeU16(centralDir, 8);           // compression
        writeU16(centralDir, 0);           // mod time
        writeU16(centralDir, 0);           // mod date
        writeU32(centralDir, info.crc32);
        writeU32(centralDir, info.compressedSize);
        writeU32(centralDir, info.uncompressedSize);
        writeU16(centralDir, static_cast<quint16>(info.nameBytes.size()));
        writeU16(centralDir, 0);           // extra
        writeU16(centralDir, 0);           // comment
        writeU16(centralDir, 0);           // disk start
        writeU16(centralDir, 0);           // int attrs
        writeU32(centralDir, 0);           // ext attrs
        writeU32(centralDir, info.offset);
        centralDir.append(info.nameBytes);
    }

    QByteArray eocd;
    writeU32(eocd, 0x06054b50);
    writeU16(eocd, 0);    // disk number
    writeU16(eocd, 0);    // disk with central dir
    writeU16(eocd, static_cast<quint16>(infos.size()));
    writeU16(eocd, static_cast<quint16>(infos.size()));
    writeU32(eocd, static_cast<quint32>(centralDir.size()));
    writeU32(eocd, centralDirOffset);
    writeU16(eocd, 0);    // comment length

    return localFiles + centralDir + eocd;
}

// ── LxdsExporter ─────────────────────────────────────────────────────────────

ExportResult LxdsExporter::exportToFile(const NestResult& result,
                                         const NestJob& job,
                                         const QString& outputPath)
{
    ExportResult er;
    er.filePath = outputPath;

    AppLogger::instance().beginOperation("LxdsExporter");
    AppLogger::instance().info(LogChannel::EXPORT,
        QString("Экспорт .lxds: %1").arg(outputPath));

    if (!result.success || result.sheets.empty()) {
        er.errorMessage = "Нет результатов для экспорта";
        AppLogger::instance().error(LogChannel::EXPORT, er.errorMessage);
        return er;
    }

    const double sheetH = job.sheet.heightMm;

    // ── layout.xml ────────────────────────────────────────────────────────
    QByteArray xmlData;
    {
        QBuffer buf(&xmlData);
        buf.open(QIODevice::WriteOnly);
        QXmlStreamWriter xml(&buf);
        xml.setAutoFormatting(true);
        xml.writeStartDocument();

        xml.writeStartElement("Layout");
        xml.writeAttribute("version",  "2.0");
        xml.writeAttribute("software", "NestingApp");
        xml.writeAttribute("created",  QDateTime::currentDateTime().toString(Qt::ISODate));

        // Sheet
        xml.writeStartElement("Sheet");
        xml.writeAttribute("width",     QString::number(job.sheet.widthMm,  'f', 4));
        xml.writeAttribute("height",    QString::number(job.sheet.heightMm, 'f', 4));
        xml.writeAttribute("unit",      "mm");
        xml.writeAttribute("material",  job.sheet.materialId.isEmpty()
                                        ? "Steel" : job.sheet.materialId);
        xml.writeAttribute("thickness", QString::number(job.sheet.thicknessMm, 'f', 2));
        xml.writeEndElement(); // Sheet

        // Parts — группируем по partId
        xml.writeStartElement("Parts");

        // Собрать уникальные типы деталей
        std::map<QString, std::vector<const PlacedPart*>> byType;
        for (const auto& sr : result.sheets)
            for (const auto& pp : sr.placedParts)
                byType[pp.partId].push_back(&pp);

        int fileNum = 1;
        for (const auto& [partId, instances] : byType) {
            xml.writeStartElement("Part");
            xml.writeAttribute("id",               partId);
            xml.writeAttribute("name",             partId);
            xml.writeAttribute("quantity_placed",  QString::number(instances.size()));
            xml.writeAttribute("file",             QString("parts/part_%1.dxf").arg(fileNum));

            for (const auto* pp : instances) {
                // Инвертируем Y: exportY = sheetHeight - partY
                const double exportX = pp->positionMm.x();
                const double exportY = sheetH - pp->positionMm.y();
                // Поворот: CW от математического CCW
                const double exportRot = std::fmod(360.0 - pp->rotationDeg, 360.0);

                xml.writeStartElement("Instance");
                xml.writeAttribute("id",       pp->instanceId);
                xml.writeAttribute("x",        QString::number(exportX,   'f', 4));
                xml.writeAttribute("y",        QString::number(exportY,   'f', 4));
                xml.writeAttribute("rotation", QString::number(exportRot, 'f', 4));
                xml.writeAttribute("flipped",  pp->flipped ? "true" : "false");
                xml.writeAttribute("sheet",    QString::number(pp->sheetIndex + 1));
                xml.writeEndElement(); // Instance
            }
            xml.writeEndElement(); // Part
            ++fileNum;
        }

        xml.writeEndElement(); // Parts

        // Statistics
        xml.writeStartElement("Statistics");
        xml.writeAttribute("total_parts",      QString::number(result.totalPlacedCount));
        xml.writeAttribute("sheets_used",      QString::number(result.sheets.size()));
        xml.writeAttribute("avg_utilization",  QString::number(result.avgUtilizationPct, 'f', 1));
        xml.writeEndElement(); // Statistics

        xml.writeEndElement(); // Layout
        xml.writeEndDocument();
        buf.close();
    }

    // ── DXF для каждого уникального типа детали ───────────────────────────
    std::vector<ZipEntry> zipEntries;
    zipEntries.push_back({"layout.xml", xmlData});

    // Собрать уникальные геометрии
    std::map<QString, const PartGeometry*> uniqueGeoms;
    for (const auto& pe : job.parts)
        uniqueGeoms[pe.geometry.partId] = &pe.geometry;

    // Собрать порядок partId как в layout.xml
    std::vector<QString> partOrder;
    for (const auto& sr : result.sheets)
        for (const auto& pp : sr.placedParts)
            if (std::find(partOrder.begin(), partOrder.end(), pp.partId) == partOrder.end())
                partOrder.push_back(pp.partId);

    DxfExporter dxfExp;
    int fileNum = 1;
    for (const auto& pid : partOrder) {
        auto it = uniqueGeoms.find(pid);
        if (it == uniqueGeoms.end()) continue;

        QByteArray dxfData = dxfExp.exportGeometryToBytes(*it->second);
        zipEntries.push_back({QString("parts/part_%1.dxf").arg(fileNum), dxfData});
        ++fileNum;
    }

    // ── Сборка ZIP ────────────────────────────────────────────────────────
    QByteArray zipData = buildZip(zipEntries);

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        er.errorMessage = QString("Не удалось открыть файл для записи: %1").arg(outputPath);
        AppLogger::instance().error(LogChannel::EXPORT, er.errorMessage);
        return er;
    }
    outFile.write(zipData);
    outFile.close();

    er.success        = true;
    er.fileSizeBytes  = zipData.size();
    er.sheetsExported = static_cast<int>(result.sheets.size());
    er.partsExported  = result.totalPlacedCount;

    AppLogger::instance().info(LogChannel::EXPORT,
        QString("Экспорт завершён: %1 листов, %2 деталей, размер=%3 байт")
        .arg(er.sheetsExported).arg(er.partsExported).arg(er.fileSizeBytes));

    AppLogger::instance().endOperation("LxdsExporter");
    return er;
}

bool LxdsExporter::exportToFile(const NestResult& result, const QString& outputPath)
{
    NestJob emptyJob;
    const ExportResult er = exportToFile(result, emptyJob, outputPath);
    m_lastError = er.errorMessage;
    return er.success;
}
