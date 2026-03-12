#include "DxfImporter.h"
#include "GeomUtils.h"
#include "core/logging/AppLogger.h"
#include "core/logging/LogChannel.h"

#include <drw_interface.h>
#include <libdxfrw.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <cmath>
#include <algorithm>
#include <stdexcept>

static constexpr double kPi    = M_PI;
static constexpr double kTwoPi = 2.0 * M_PI;

// ── Внутренний слушатель DRW ──────────────────────────────────────────────────

class DxfListener : public DRW_Interface
{
public:
    struct Segment {
        QPointF start;
        QPointF end;
    };

    // Накапливаем все сегменты (LINE, ARC→ломаная, и т.д.)
    std::vector<std::vector<QPointF>> closedContours;  // уже замкнутые контуры
    std::vector<Segment>              segments;         // открытые сегменты

    // Настройки
    double   toleranceMm       = 0.1;
    double   approxTol         = 0.01;
    QString  filterLayer;
    double   unitScale         = 1.0;  // умножать координаты на это
    QString  dxfVersion        = "UNKNOWN";
    QString  detectedUnits     = "unknown";

    // Статистика
    int entityCount  = 0;
    int skippedCount = 0;

    std::vector<LogEntry> logEntries;

    // ── Блоки ────────────────────────────────────────────────────────────
    struct BlockDef {
        QString name;
        std::vector<std::vector<QPointF>> contours;
        std::vector<Segment>              segs;
    };
    std::map<std::string, BlockDef> blockDefs;
    std::string currentBlock;

private:
    void logInfo(const QString& msg, const QString& detail = {}) {
        LogEntry e;
        e.timestamp = QDateTime::currentDateTime();
        e.level     = LogLevel::INFO;
        e.channel   = LogChannel::IMPORT;
        e.message   = msg;
        e.detail    = detail;
        logEntries.push_back(e);
        AppLogger::instance().info(LogChannel::IMPORT, msg, detail);
    }

    void logWarn(const QString& msg, const QString& detail = {}) {
        LogEntry e;
        e.timestamp = QDateTime::currentDateTime();
        e.level     = LogLevel::WARNING;
        e.channel   = LogChannel::IMPORT;
        e.message   = msg;
        e.detail    = detail;
        logEntries.push_back(e);
        AppLogger::instance().warning(LogChannel::IMPORT, msg, detail);
    }

    bool acceptLayer(const std::string& layer) const {
        if (filterLayer.isEmpty()) return true;
        return QString::fromStdString(layer) == filterLayer;
    }

    void addSegment(const QPointF& a, const QPointF& b,
                    const std::string& layer = {}) {
        if (!acceptLayer(layer)) return;
        if (GeomUtils::dist2(a, b) < 1e-16) return;
        if (!currentBlock.empty()) {
            blockDefs[currentBlock].segs.push_back({a, b});
        } else {
            segments.push_back({a, b});
        }
    }

    void addContour(const std::vector<QPointF>& pts,
                    const std::string& layer = {}) {
        if (!acceptLayer(layer)) return;
        if (pts.size() < 3) return;
        if (!currentBlock.empty()) {
            blockDefs[currentBlock].contours.push_back(pts);
        } else {
            closedContours.push_back(pts);
        }
    }

    // Масштабирование
    QPointF s(double x, double y) const {
        return {x * unitScale, y * unitScale};
    }
    double  sc(double v) const { return v * unitScale; }

    // ── Bulge → arc points ────────────────────────────────────────────────
    std::vector<QPointF> bulgeToArc(const QPointF& p1, const QPointF& p2,
                                     double bulge) const {
        if (std::abs(bulge) < 1e-9) return {p1, p2};

        const double chord = GeomUtils::dist(p1, p2);
        if (chord < 1e-9) return {p1};

        const double halfAngle = 2.0 * std::atan(std::abs(bulge));
        const double radius    = chord / (2.0 * std::sin(halfAngle));
        const double sagitta   = radius * (1.0 - std::cos(halfAngle));

        // Направление от p1 к p2
        const double dx = p2.x() - p1.x();
        const double dy = p2.y() - p1.y();
        const double len = chord;
        // Перпендикуляр (повернут на 90° против часовой)
        double perpX = -dy / len;
        double perpY =  dx / len;
        if (bulge < 0.0) { perpX = -perpX; perpY = -perpY; }

        // Центр дуги
        const double midX = (p1.x() + p2.x()) * 0.5;
        const double midY = (p1.y() + p2.y()) * 0.5;
        const double distToCenter = radius - sagitta;
        const QPointF center {midX + perpX * distToCenter,
                               midY + perpY * distToCenter};

        // Углы
        double angle1 = std::atan2(p1.y() - center.y(), p1.x() - center.x());
        double angle2 = std::atan2(p2.y() - center.y(), p2.x() - center.x());

        if (bulge > 0.0) {
            // CCW: angle2 > angle1
            while (angle2 < angle1) angle2 += kTwoPi;
        } else {
            // CW: angle2 < angle1
            while (angle2 > angle1) angle2 -= kTwoPi;
        }

        return GeomUtils::approximateArc(center, radius, angle1, angle2, approxTol);
    }

public:
    // ── DRW_Interface callbacks ───────────────────────────────────────────

    void addHeader(const DRW_Header* data) override {
        if (!data) return;
        // Версия DXF
        // Единицы: $INSUNITS
        auto it = data->vars.find("$INSUNITS");
        if (it != data->vars.end()) {
            int u = static_cast<int>(it->second->content.i);
            switch (u) {
                case 1:  unitScale = 25.4;  detectedUnits = "in"; break;  // inches
                case 4:  unitScale = 1.0;   detectedUnits = "mm"; break;  // mm
                case 5:  unitScale = 10.0;  detectedUnits = "cm"; break;  // cm
                case 6:  unitScale = 1000.0;detectedUnits = "m";  break;  // m
                default: unitScale = 1.0;   detectedUnits = "mm"; break;  // assume mm
            }
            logInfo(QString("Единицы DXF: %1 (INSUNITS=%2, scale=%3)")
                    .arg(detectedUnits).arg(u).arg(unitScale));
        } else {
            logWarn("$INSUNITS не найден, предполагаем мм");
        }
    }

    void addLayer(const DRW_Layer& /*data*/) override {}
    void addLType(const DRW_LType& /*data*/) override {}
    void addDimStyle(const DRW_Dimstyle& /*data*/) override {}
    void addVport(const DRW_Vport& /*data*/) override {}
    void addTextStyle(const DRW_Textstyle& /*data*/) override {}
    void addAppId(const DRW_AppId& /*data*/) override {}
    void addBlock(const DRW_Block& data) override {
        currentBlock = data.name;
        blockDefs[currentBlock] = BlockDef{};
        blockDefs[currentBlock].name = QString::fromStdString(data.name);
    }
    void endBlock() override { currentBlock.clear(); }
    void addPoint(const DRW_Point& /*data*/) override { }
    void addMText(const DRW_MText& /*data*/) override {}
    void addText(const DRW_Text& /*data*/) override {}
    void addDimAlign(const DRW_DimAligned* /*data*/) override {}
    void addDimLinear(const DRW_DimLinear* /*data*/) override {}
    void addDimRadial(const DRW_DimRadial* /*data*/) override {}
    void addDimDiametric(const DRW_DimDiametric* /*data*/) override {}
    void addDimAngular(const DRW_DimAngular* /*data*/) override {}
    void addDimAngular3P(const DRW_DimAngular3p* /*data*/) override {}
    void addDimOrdinate(const DRW_DimOrdinate* /*data*/) override {}
    void addLeader(const DRW_Leader* /*data*/) override {}
    void addHatch(const DRW_Hatch* /*data*/) override {}
    void addViewport(const DRW_Viewport& /*data*/) override {}
    void addImage(const DRW_Image* /*data*/) override {}
    void linkImage(DRW_ImageDef* /*data*/) override {}
    void addComment(const char* /*comment*/) override {}
    void setBlock(const int /*handle*/) override {}
    void writeHeader(DRW_Header& /*data*/) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}

    void addLine(const DRW_Line& data) override {
        ++entityCount;
        const QPointF a = s(data.basePoint.x, data.basePoint.y);
        const QPointF b = s(data.secPoint.x,  data.secPoint.y);
        addSegment(a, b, data.layer);
    }

    void addArc(const DRW_Arc& data) override {
        ++entityCount;
        if (!acceptLayer(data.layer)) return;
        const QPointF center = s(data.basePoint.x, data.basePoint.y);
        const double radius  = sc(data.radious);
        const double startDeg = data.staangle * 180.0 / kPi;  // libdxfrw daёт в радианах
        const double endDeg   = data.endangle  * 180.0 / kPi;

        // Переводим градусы → радианы
        double startRad = data.staangle;
        double endRad   = data.endangle;

        // Дуга идёт по CCW от start до end
        if (endRad < startRad) endRad += kTwoPi;

        auto pts = GeomUtils::approximateArc(center, radius, startRad, endRad, approxTol);
        if (pts.size() < 2) return;

        // Добавляем как сегменты
        for (std::size_t i = 0; i + 1 < pts.size(); ++i)
            addSegment(pts[i], pts[i + 1], data.layer);
    }

    void addCircle(const DRW_Circle& data) override {
        ++entityCount;
        if (!acceptLayer(data.layer)) return;
        const QPointF center = s(data.basePoint.x, data.basePoint.y);
        const double radius  = sc(data.radious);
        auto pts = GeomUtils::approximateArc(center, radius, 0.0, kTwoPi - 1e-9, approxTol);
        if (!pts.empty()) pts.push_back(pts.front());
        addContour(pts, data.layer);
    }

    void addLWPolyline(const DRW_LWPolyline& data) override {
        ++entityCount;
        if (!acceptLayer(data.layer)) return;
        if (data.vertlist.empty()) return;

        std::vector<QPointF> pts;
        const int n = static_cast<int>(data.vertlist.size());
        for (int i = 0; i < n; ++i) {
            const auto& v1 = data.vertlist[i];
            const auto& v2 = data.vertlist[(i + 1) % n];

            const QPointF p1 = s(v1->x, v1->y);
            const QPointF p2 = s(v2->x, v2->y);

            if (std::abs(v1->bulge) > 1e-9) {
                auto arcPts = bulgeToArc(p1, p2, v1->bulge);
                for (std::size_t j = 0; j + 1 < arcPts.size(); ++j)
                    pts.push_back(arcPts[j]);
            } else {
                pts.push_back(p1);
            }

            // Последняя итерация — добавить конечную точку если не замкнуто
            if (i == n - 1 && !data.flags) pts.push_back(p2);
        }

        if (data.flags & 1) {
            // Замкнутый полигон
            addContour(pts, data.layer);
        } else {
            // Разомкнутая ломаная — добавляем как сегменты
            for (std::size_t i = 0; i + 1 < pts.size(); ++i)
                addSegment(pts[i], pts[i + 1], data.layer);
        }
    }

    void addPolyline(const DRW_Polyline& data) override {
        ++entityCount;
        if (!acceptLayer(data.layer)) return;
        if (data.vertlist.empty()) return;

        std::vector<QPointF> pts;
        for (const auto* v : data.vertlist)
            pts.push_back(s(v->basePoint.x, v->basePoint.y));

        if (data.flags & 1) {
            addContour(pts, data.layer);
        } else {
            for (std::size_t i = 0; i + 1 < pts.size(); ++i)
                addSegment(pts[i], pts[i + 1], data.layer);
        }
    }

    void addSpline(const DRW_Spline* data) override {
        ++entityCount;
        if (!data || !acceptLayer(data->layer)) return;

        std::vector<QPointF> cps;
        for (const auto* cp : data->controllist)
            cps.push_back(s(cp->x, cp->y));

        std::vector<double> knots;
        for (double k : data->knotslist)
            knots.push_back(k);

        auto pts = GeomUtils::approximateSpline(cps, data->degree, knots, approxTol);
        if (pts.size() < 2) return;

        const bool closed = (data->flags & 1);
        if (closed) {
            if (GeomUtils::dist(pts.front(), pts.back()) > approxTol)
                pts.push_back(pts.front());
            addContour(pts, data->layer);
        } else {
            for (std::size_t i = 0; i + 1 < pts.size(); ++i)
                addSegment(pts[i], pts[i + 1], data->layer);
        }
    }

    void addEllipse(const DRW_Ellipse& data) override {
        ++entityCount;
        if (!acceptLayer(data.layer)) return;
        const QPointF center = s(data.basePoint.x, data.basePoint.y);
        const double rx = sc(std::sqrt(data.secPoint.x * data.secPoint.x +
                                       data.secPoint.y * data.secPoint.y));
        const double ry = rx * data.ratio;
        const double rot = std::atan2(data.secPoint.y, data.secPoint.x);

        auto pts = GeomUtils::approximateEllipse(center, rx, ry, rot,
                                                  data.staparam, data.endparam,
                                                  approxTol);
        if (pts.size() < 2) return;

        const bool full = std::abs(data.endparam - data.staparam) > kTwoPi - 0.01;
        if (full) {
            addContour(pts, data.layer);
        } else {
            for (std::size_t i = 0; i + 1 < pts.size(); ++i)
                addSegment(pts[i], pts[i + 1], data.layer);
        }
    }

    void addInsert(const DRW_Insert& data) override {
        ++entityCount;
        const QString blockName = QString::fromStdString(data.name);

        auto it = blockDefs.find(data.name);
        if (it == blockDefs.end()) {
            logWarn(QString("INSERT: блок '%1' не найден").arg(blockName));
            ++skippedCount;
            return;
        }

        const double rotRad = data.angle * kPi / 180.0;
        const double tx = sc(data.basePoint.x);
        const double ty = sc(data.basePoint.y);
        const double sx = data.xscale;
        const double sy = data.yscale;

        logInfo(QString("INSERT блок '%1': pos=(%.2f,%.2f) rot=%.2f sx=%.2f sy=%.2f")
                .arg(blockName).arg(tx).arg(ty).arg(data.angle).arg(sx).arg(sy));

        // Применить трансформацию к контурам блока
        for (const auto& c : it->second.contours) {
            auto transformed = GeomUtils::transformContour(c, tx, ty, rotRad, sx, sy);
            if (!currentBlock.empty())
                blockDefs[currentBlock].contours.push_back(transformed);
            else
                closedContours.push_back(transformed);
        }
        for (const auto& seg : it->second.segs) {
            auto a = GeomUtils::transformContour({seg.start}, tx, ty, rotRad, sx, sy);
            auto b = GeomUtils::transformContour({seg.end},   tx, ty, rotRad, sx, sy);
            if (!a.empty() && !b.empty()) {
                if (!currentBlock.empty())
                    blockDefs[currentBlock].segs.push_back({a[0], b[0]});
                else
                    segments.push_back({a[0], b[0]});
            }
        }
    }
};

// ── Сборка контуров из сегментов ─────────────────────────────────────────────

static std::vector<std::vector<QPointF>>
buildContours(std::vector<DxfListener::Segment>& segs,
              double toleranceMm,
              std::vector<LogEntry>& outLog)
{
    std::vector<std::vector<QPointF>> result;
    std::vector<bool> used(segs.size(), false);

    auto logMsg = [&](LogLevel lv, const QString& msg) {
        LogEntry e;
        e.timestamp = QDateTime::currentDateTime();
        e.level     = lv;
        e.channel   = LogChannel::IMPORT;
        e.message   = msg;
        outLog.push_back(e);
        AppLogger::instance().log(lv, LogChannel::IMPORT, msg);
    };

    const double tol2 = toleranceMm * toleranceMm;

    while (true) {
        // Найти первый неиспользованный сегмент
        int startIdx = -1;
        for (int i = 0; i < (int)segs.size(); ++i) {
            if (!used[i]) { startIdx = i; break; }
        }
        if (startIdx < 0) break;

        std::vector<QPointF> chain;
        chain.push_back(segs[startIdx].start);
        chain.push_back(segs[startIdx].end);
        used[startIdx] = true;

        bool extended = true;
        while (extended) {
            extended = false;
            const QPointF& tip = chain.back();
            for (int i = 0; i < (int)segs.size(); ++i) {
                if (used[i]) continue;
                if (GeomUtils::dist2(tip, segs[i].start) <= tol2) {
                    chain.push_back(segs[i].end);
                    used[i] = true;
                    extended = true;
                    break;
                }
                if (GeomUtils::dist2(tip, segs[i].end) <= tol2) {
                    chain.push_back(segs[i].start);
                    used[i] = true;
                    extended = true;
                    break;
                }
            }
        }

        if (chain.size() < 3) continue;

        // Проверить замкнутость
        const double gap = GeomUtils::dist(chain.front(), chain.back());
        if (gap > toleranceMm) {
            logMsg(LogLevel::WARNING,
                   QString("Незамкнутый контур: gap=%.4f мм, вершин=%2")
                   .arg(gap).arg(chain.size()));
            // Автозакрытие
            chain.push_back(chain.front());
            logMsg(LogLevel::INFO, "Контур автоматически закрыт");
        } else {
            chain.back() = chain.front(); // snap
        }

        result.push_back(std::move(chain));
    }

    return result;
}

// ── DxfImporter::importFile ───────────────────────────────────────────────────

ImportResult DxfImporter::importFile(const QString& path, const ImportOptions& opts)
{
    ImportResult result;
    const QString name = QFileInfo(path).fileName();

    AppLogger::instance().beginOperation(QString("DXF импорт: %1").arg(name));
    AppLogger::instance().info(LogChannel::IMPORT,
                               QString("Начало парсинга: %1").arg(path));

    if (!QFile::exists(path)) {
        result.errorMessage = QString("Файл не найден: %1").arg(path);
        AppLogger::instance().error(LogChannel::IMPORT, result.errorMessage);
        return result;
    }

    DxfListener listener;
    listener.toleranceMm = opts.toleranceMm;
    listener.approxTol   = opts.approxToleranceMm;
    listener.filterLayer = opts.filterLayer;

    dxfRW reader(path.toLocal8Bit().constData());
    const bool ok = reader.read(&listener, false);

    if (!ok) {
        result.errorMessage = QString("Ошибка парсинга DXF: %1").arg(path);
        AppLogger::instance().error(LogChannel::IMPORT, result.errorMessage, path);
        return result;
    }

    result.dxfVersion   = listener.dxfVersion;
    result.units        = listener.detectedUnits;
    result.entityCount  = listener.entityCount;
    result.log          = listener.logEntries;

    AppLogger::instance().info(LogChannel::IMPORT,
        QString("Entity найдено: %1, единицы: %2")
        .arg(listener.entityCount).arg(listener.detectedUnits));

    // Собираем контуры из сегментов
    auto assembled = buildContours(listener.segments, opts.toleranceMm, result.log);
    for (auto& c : assembled)
        listener.closedContours.push_back(std::move(c));

    if (listener.closedContours.empty()) {
        result.errorMessage = "Не найдено ни одного замкнутого контура";
        AppLogger::instance().warning(LogChannel::IMPORT, result.errorMessage, path);
        return result;
    }

    // Группировка контуров в детали
    // Простая эвристика: каждый внешний контур (не вложенный ни в один другой) — деталь
    const int nc = static_cast<int>(listener.closedContours.size());
    std::vector<bool> isHole(nc, false);

    for (int i = 0; i < nc; ++i) {
        for (int j = 0; j < nc; ++j) {
            if (i == j) continue;
            if (GeomUtils::isContainedIn(listener.closedContours[i],
                                          listener.closedContours[j])) {
                isHole[i] = true;
                break;
            }
        }
    }

    if (opts.importAsOnepart) {
        // Все контуры → одна деталь
        std::vector<std::vector<QPointF>> all = listener.closedContours;
        QString pid = QString("%1_1").arg(QFileInfo(name).baseName());
        auto geom = GeomUtils::buildPartGeometry(all, pid, name, opts.filterLayer);
        if (geom.isValid()) {
            result.parts.push_back(std::move(geom));
        } else {
            ++result.skippedCount;
            AppLogger::instance().warning(LogChannel::IMPORT,
                QString("Деталь %1: нулевая площадь, пропущена").arg(pid));
        }
    } else {
        int partNum = 1;
        for (int i = 0; i < nc; ++i) {
            if (isHole[i]) continue;

            std::vector<std::vector<QPointF>> group;
            group.push_back(listener.closedContours[i]);
            for (int j = 0; j < nc; ++j) {
                if (i != j && isHole[j] &&
                    GeomUtils::isContainedIn(listener.closedContours[j],
                                              listener.closedContours[i])) {
                    group.push_back(listener.closedContours[j]);
                }
            }

            QString pid = QString("%1_%2")
                          .arg(QFileInfo(name).baseName()).arg(partNum++);

            auto geom = GeomUtils::buildPartGeometry(group, pid, name, opts.filterLayer);
            if (!geom.isValid()) {
                ++result.skippedCount;
                AppLogger::instance().warning(LogChannel::IMPORT,
                    QString("Деталь %1: нулевая площадь, пропущена").arg(pid),
                    pid);
                continue;
            }

            // Проверить самопересечение
            if (GeomUtils::hasSelfIntersection(geom.outerContour.vertices)) {
                AppLogger::instance().warning(LogChannel::IMPORT,
                    QString("Деталь %1: самопересечение обнаружено, исправляем").arg(pid),
                    pid);
                geom.outerContour.vertices =
                    GeomUtils::fixSelfIntersection(geom.outerContour.vertices);
            }

            result.parts.push_back(std::move(geom));
        }
    }

    result.success = !result.parts.empty();
    AppLogger::instance().info(LogChannel::IMPORT,
        QString("Итог импорта '%1': деталей=%2, пропущено=%3")
        .arg(name).arg(result.parts.size()).arg(result.skippedCount));

    AppLogger::instance().endOperation(QString("DXF импорт: %1").arg(name));
    return result;
}

ImportResult DxfImporter::importData(const QByteArray& data,
                                      const QString& sourceName,
                                      const ImportOptions& opts)
{
    // Записываем во временный файл и вызываем importFile
    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + "/nesting_dxf_XXXXXX.dxf");
    if (!tmp.open()) {
        ImportResult r;
        r.errorMessage = "Не удалось создать временный файл";
        return r;
    }
    tmp.write(data);
    tmp.flush();
    return importFile(tmp.fileName(), opts);
}

QList<PartGeometry> DxfImporter::import(const QString& path,
                                         const ImportOptions& opts)
{
    const ImportResult r = importFile(path, opts);
    QList<PartGeometry> list;
    list.reserve(static_cast<int>(r.parts.size()));
    for (const auto& p : r.parts)
        list.append(p);
    return list;
}
