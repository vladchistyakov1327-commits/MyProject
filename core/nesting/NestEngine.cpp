#include "NestEngine.h"
#include "NFPCalculator.h"
#include "RotationSampler.h"
#include "SheetManager.h"
#include "core/geometry/GeomUtils.h"
#include "core/geometry/CoedgeDetector.h"
#include "core/logging/AppLogger.h"
#include "core/logging/LogChannel.h"

#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>
#include <QDateTime>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <limits>

NestEngine::NestEngine(QObject* parent) : QObject(parent) {}
NestEngine::~NestEngine() { cancel(); }

// ── Вспомогательные ───────────────────────────────────────────────────────────

/// Применить вращение и отражение к контуру детали.
static std::vector<QPointF> applyTransform(const std::vector<QPointF>& pts,
                                            double rotDeg, bool flip)
{
    std::vector<QPointF> result = pts;
    if (flip) result = GeomUtils::mirrorX(result);
    if (std::abs(rotDeg) > 1e-6) {
        result = GeomUtils::transformContour(result, 0.0, 0.0,
                                              rotDeg * M_PI / 180.0);
    }
    return result;
}

/// Расставить трансформированный контур в позицию.
static QPolygonF makeWorldContour(const std::vector<QPointF>& localPts,
                                   const QPointF& pos)
{
    QPolygonF poly;
    poly.reserve(static_cast<int>(localPts.size()));
    for (const auto& p : localPts)
        poly << QPointF(p.x() + pos.x(), p.y() + pos.y());
    return poly;
}

// ── Построение списка экземпляров ─────────────────────────────────────────────

std::vector<std::pair<int, int>>
NestEngine::buildSortedInstances(const NestJob& job)
{
    // Пара: {index в parts, экземпляр 0..quantity-1}
    // Сортируем по убыванию areaMm2 типа детали
    std::vector<std::pair<int, int>> instances;
    for (int i = 0; i < (int)job.parts.size(); ++i) {
        const int q = job.parts[i].quantity;
        for (int k = 0; k < q; ++k)
            instances.push_back({i, k});
    }

    std::stable_sort(instances.begin(), instances.end(),
        [&job](const std::pair<int,int>& a, const std::pair<int,int>& b) {
            return job.parts[a.first].geometry.areaMm2 >
                   job.parts[b.first].geometry.areaMm2;
        });

    return instances;
}

// ── Прогресс ──────────────────────────────────────────────────────────────────

void NestEngine::reportProgress(QPromise<NestResult>* promise,
                                 int placed, int total, int currentSheet)
{
    if (promise) {
        const int pct = (total > 0) ? (placed * 100 / total) : 0;
        promise->setProgressValue(pct);
    }
    const int pct = (total > 0) ? (placed * 100 / total) : 0;
    emit progressChanged(pct, placed, total, currentSheet);
}

// ── Основной алгоритм ─────────────────────────────────────────────────────────

NestResult NestEngine::doNesting(const NestJob& job, QPromise<NestResult>* promise)
{
    NestResult result;
    result.jobId        = job.jobId;
    result.calculatedAt = QDateTime::currentDateTime();

    const auto& params = job.params;
    const int totalInstances = [&] {
        int n = 0;
        for (const auto& p : job.parts) n += p.quantity;
        return n;
    }();
    result.totalPartsRequested = totalInstances;

    AppLogger::instance().beginOperation("NestEngine::run");
    AppLogger::instance().info(LogChannel::NESTING,
        QString("Старт нестинга: %1 типов, %2 деталей, лист %3×%4 мм, стратегия=%5")
        .arg(job.parts.size())
        .arg(totalInstances)
        .arg(job.sheet.widthMm)
        .arg(job.sheet.heightMm)
        .arg(static_cast<int>(params.strategy)));

    if (job.parts.empty()) {
        result.errorMessage = "Нет деталей для раскладки";
        AppLogger::instance().warning(LogChannel::NESTING, result.errorMessage);
        return result;
    }

    SheetManager sheetMgr(job.sheet);
    NFPCalculator nfpCalc;
    CoedgeDetector coedge(params.coedgeTolerance);

    // Подготовить контуры с offset (зазор)
    const double halfSpacing = params.partSpacingMm / 2.0;

    struct PartInstance {
        int    typeIdx;
        int    instanceNum;
        QString instanceId;
    };

    auto instances = buildSortedInstances(job);

    std::vector<SheetResult> sheets;
    int placed = 0;

    // Нестинг полистовой
    int sheetIdx = 0;
    int instIdx  = 0;

    while (instIdx < (int)instances.size()) {
        if (m_cancelRequested.load()) {
            AppLogger::instance().info(LogChannel::NESTING, "Нестинг отменён пользователем");
            break;
        }

        if (!sheetMgr.canAddSheet(sheetIdx)) {
            AppLogger::instance().warning(LogChannel::NESTING,
                QString("Достигнут лимит листов (%1), оставшиеся детали не размещены")
                .arg(sheetIdx));
            break;
        }

        SheetResult sr = sheetMgr.createSheetResult(sheetIdx);
        const auto sheetContour = sheetMgr.sheetContourWithMargin(params.sheetMarginMm);
        const double sheetArea  = GeomUtils::area(sheetContour);
        sr.totalAreaMm2 = sheetMgr.sheetTotalAreaMm2();

        AppLogger::instance().info(LogChannel::NESTING,
            QString("Начало раскладки на лист #%1 (площадь=%.0f мм²)")
            .arg(sheetIdx + 1).arg(sheetArea));

        // --- NFP union для текущего листа ---
        Clipper2Lib::PathsD nfpUnion; // объединение всех NFP

        int placedOnSheet = 0;
        double usedArea   = 0.0;

        while (instIdx < (int)instances.size()) {
            if (m_cancelRequested.load()) break;

            const auto& [typeIdx, instanceNum] = instances[instIdx];
            const auto& partEntry = job.parts[typeIdx];
            const auto& geom      = partEntry.geometry;

            // Углы для перебора
            const bool  flip = partEntry.allowFlip;
            auto angles = RotationSampler::sample(partEntry.rotationMode,
                                                   partEntry.customRotStep, flip);
            if (!params.enableRotation) angles = {0.0};

            // Попытаться разместить деталь с одним из углов
            bool placed_here = false;
            for (double angleDeg : angles) {
                if (m_cancelRequested.load()) break;

                const bool  isFlip  = angleDeg >= 360.0;
                const double realAngle = isFlip ? (angleDeg - 360.0) : angleDeg;

                // Трансформированный контур (локальный)
                std::vector<QPointF> localPts = applyTransform(
                    geom.outerContour.vertices, realAngle, isFlip);

                // Offset для зазора
                std::vector<QPointF> offsetPts = GeomUtils::offsetContour(
                    localPts, halfSpacing);
                if (offsetPts.empty()) offsetPts = localPts;

                // IFP — допустимые позиции внутри листа
                Clipper2Lib::PathsD ifp = nfpCalc.computeIFP(sheetContour, offsetPts);
                if (ifp.empty()) continue;

                // Вычесть NFP союз
                Clipper2Lib::PathsD validRegion =
                    nfpCalc.computeValidRegion(ifp, nfpUnion);
                if (validRegion.empty()) continue;

                // Найти лучшую позицию
                auto bestPos = nfpCalc.findBestPosition(
                    validRegion, params.strategy, sr.sheetBounds);
                if (!bestPos.has_value()) continue;

                // Coedge
                if (params.enableCoedge && !sr.placedParts.empty()) {
                    // Проверить все уже размещённые детали
                    for (const auto& pp : sr.placedParts) {
                        const auto& ppGeom = job.parts[
                            [&]() {
                                for (int i = 0; i < (int)job.parts.size(); ++i)
                                    if (job.parts[i].geometry.partId == pp.partId)
                                        return i;
                                return 0;
                            }()].geometry;
                        auto matches = coedge.findCoedges(
                            ppGeom,  pp.positionMm,  pp.rotationDeg,
                            geom,    *bestPos,        realAngle);
                        if (!matches.empty()) {
                            *bestPos = coedge.applyCoedge(matches[0], *bestPos);
                            AppLogger::instance().info(LogChannel::COEDGE,
                                QString("Coedge применён: %1 ↔ %2, экономия=%.2f мм²")
                                .arg(ppGeom.partId).arg(geom.partId)
                                .arg(matches[0].savedAreaMm2));
                        }
                    }
                }

                // Создать PlacedPart
                PlacedPart pp;
                pp.partId      = geom.partId;
                pp.instanceId  = QString("%1_%2").arg(geom.partId).arg(instanceNum);
                pp.positionMm  = *bestPos;
                pp.rotationDeg = realAngle;
                pp.flipped     = isFlip;
                pp.sheetIndex  = sheetIdx;
                pp.placedContour = makeWorldContour(localPts, *bestPos);

                sr.placedParts.push_back(pp);
                usedArea += geom.areaMm2;
                ++placedOnSheet;
                ++placed;
                ++instIdx;
                placed_here = true;

                // Обновить NFP union: добавить NFP(новая деталь, любая следующая)
                // Упрощение: добавляем смещённый контур как "запрещённую зону"
                // Точный подход: пересчитывать NFP для каждой следующей детали
                auto worldPts = GeomUtils::transformContour(
                    offsetPts, bestPos->x(), bestPos->y(), 0.0);
                Clipper2Lib::PathD worldPath;
                for (const auto& wp : worldPts)
                    worldPath.push_back({wp.x(), wp.y()});
                nfpUnion.push_back(worldPath);

                // Каждые 5% прогресса — логируем
                if (placed % std::max(1, totalInstances / 20) == 0) {
                    const int pct = placed * 100 / totalInstances;
                    AppLogger::instance().info(LogChannel::NESTING,
                        QString("Прогресс: %1% (%2/%3 деталей, лист #%4)")
                        .arg(pct).arg(placed).arg(totalInstances).arg(sheetIdx + 1));
                    reportProgress(promise, placed, totalInstances, sheetIdx);
                }

                break; // Нашли позицию — переходим к следующей детали
            }

            if (!placed_here) {
                // Не нашли место на текущем листе — переходим к следующему
                break;
            }
        }

        // Завершить лист
        sr.partsCount      = placedOnSheet;
        sr.usedAreaMm2     = usedArea;
        sr.wasteAreaMm2    = sr.totalAreaMm2 - usedArea;
        sr.utilizationPercent  = (sr.totalAreaMm2 > 0.0)
                             ? (usedArea / sr.totalAreaMm2) * 100.0 : 0.0;

        AppLogger::instance().log(LogLevel::INFO, LogChannel::NESTING,
            QString("Лист #%1 завершён: %2 деталей, утилизация=%.1f%%")
            .arg(sheetIdx + 1).arg(placedOnSheet).arg(sr.utilizationPercent),
            {}, {}, sheetIdx);

        sheets.push_back(std::move(sr));
        ++sheetIdx;
    }

    // Детали, которые не удалось разместить
    for (int i = instIdx; i < (int)instances.size(); ++i) {
        const auto& [typeIdx, instanceNum] = instances[i];
        const QString iid = QString("%1_%2")
                            .arg(job.parts[typeIdx].geometry.partId)
                            .arg(instanceNum);
        result.unplacedPartIds.push_back(iid);
        AppLogger::instance().warning(LogChannel::NESTING,
            QString("Деталь не размещена: %1").arg(iid), iid);
    }

    result.sheets           = std::move(sheets);
    result.totalPlacedCount = placed;
    result.cancelled        = m_cancelRequested.load();
    result.unplacedPartsCount = static_cast<int>(result.unplacedPartIds.size());
    result.elapsedMs        = result.calculatedAt.msecsTo(QDateTime::currentDateTime());

    // Средняя утилизация
    if (!result.sheets.empty()) {
        double sum = 0.0;
        for (const auto& s : result.sheets) sum += s.utilizationPercent;
        result.avgUtilizationPct = sum / result.sheets.size();
    }

    result.success = (placed > 0);
    reportProgress(promise, placed, totalInstances, sheetIdx);

    AppLogger::instance().info(LogChannel::NESTING,
        QString("Нестинг завершён: %1 листов, %2/%3 деталей, ср. утилизация=%.1f%%")
        .arg(result.sheets.size())
        .arg(placed)
        .arg(totalInstances)
        .arg(result.avgUtilizationPct));

    AppLogger::instance().endOperation("NestEngine::run");

    return result;
}

// ── Публичный API ─────────────────────────────────────────────────────────────

NestResult NestEngine::runSync(const NestJob& job)
{
    m_cancelRequested.store(false);
    m_running.store(true);
    auto result = doNesting(job, nullptr);
    m_running.store(false);
    emit finished(result);
    return result;
}

QFuture<NestResult> NestEngine::runAsync(const NestJob& job)
{
    m_cancelRequested.store(false);
    m_running.store(true);

    // Захватить job по значению
    return QtConcurrent::run([this, job]() -> NestResult {
        auto result = doNesting(job, nullptr);
        m_running.store(false);
        emit finished(result);
        return result;
    });
}
