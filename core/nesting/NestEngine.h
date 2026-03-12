#pragma once
#include "NestJob.h"
#include "NestResult.h"
#include <QObject>
#include <QFuture>
#include <QPromise>
#include <atomic>
#include <functional>

/**
 * Главный оркестратор нестинга.
 *
 * - Работает полностью в рабочем потоке (QtConcurrent::run)
 * - Использует NFP + выбранную PlacementStrategy
 * - Поддерживает отмену через cancelRequested
 * - Прогресс передаётся через QPromise
 */
class NestEngine : public QObject
{
    Q_OBJECT
public:
    explicit NestEngine(QObject* parent = nullptr);
    ~NestEngine() override;

    /// Запустить нестинг асинхронно, вернуть Future.
    QFuture<NestResult> runAsync(const NestJob& job);

    /// Запустить синхронно (для тестов).
    NestResult runSync(const NestJob& job);

    /// Запросить отмену (потокобезопасно).
    void cancel() { m_cancelRequested.store(true); }

    bool isRunning() const { return m_running.load(); }

signals:
    void progressChanged(int percent, int placed, int total, int currentSheet);
    void finished(NestResult result);
    void logEntry(LogEntry entry);

private:
    std::atomic<bool> m_cancelRequested{false};
    std::atomic<bool> m_running{false};

    NestResult doNesting(const NestJob& job,
                         QPromise<NestResult>* promise = nullptr);

    /// Отсортировать детали по убыванию площади.
    static std::vector<std::pair<int, int>>
    buildSortedInstances(const NestJob& job);

    void reportProgress(QPromise<NestResult>* promise,
                        int placed, int total, int currentSheet);
};
