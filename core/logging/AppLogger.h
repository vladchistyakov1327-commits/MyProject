#pragma once
#include "LogEntry.h"
#include <QObject>
#include <QMutex>
#include <vector>
#include <unordered_map>

/**
 * Центральный синглтон-логгер приложения.
 *
 * Все подсистемы пишут через AppLogger::instance().log(...).
 * UI подключается к сигналу entryAdded() через Qt::QueuedConnection,
 * что позволяет логировать из любого потока.
 */
class AppLogger : public QObject
{
    Q_OBJECT
public:
    static AppLogger& instance();

    // ── Основной метод ────────────────────────────────────────────────────────
    void log(LogLevel level, LogChannel channel,
             const QString& message,
             const QString& detail    = {},
             const QString& partId    = {},
             int            sheetIndex = -1);

    // ── Удобные методы ────────────────────────────────────────────────────────
    void debug  (LogChannel ch, const QString& msg, const QString& detail = {});
    void info   (LogChannel ch, const QString& msg, const QString& detail = {});
    void warning(LogChannel ch, const QString& msg, const QString& detail = {});
    void error  (LogChannel ch, const QString& msg, const QString& detail = {});
    void critical(LogChannel ch, const QString& msg, const QString& detail = {});

    // ── Замер операций ────────────────────────────────────────────────────────
    void beginOperation(const QString& name);   ///< Записывает START + сохраняет время
    void endOperation  (const QString& name);   ///< Записывает END + elapsed

    // ── Доступ к записям (thread-safe) ────────────────────────────────────────
    std::vector<LogEntry> entries() const;
    void clear();

signals:
    void entryAdded(LogEntry entry);

private:
    AppLogger() = default;
    ~AppLogger() = default;
    AppLogger(const AppLogger&) = delete;
    AppLogger& operator=(const AppLogger&) = delete;

    mutable QMutex             m_mutex;
    std::vector<LogEntry>      m_entries;
    std::unordered_map<std::string, qint64> m_operationStart; ///< name → msecsSinceEpoch
};

// ── Макросы ──────────────────────────────────────────────────────────────────
#define LOG_DEBUG(ch, msg)           AppLogger::instance().debug  (ch, msg)
#define LOG_INFO(ch, msg)            AppLogger::instance().info   (ch, msg)
#define LOG_WARN(ch, msg, ...)       AppLogger::instance().warning(ch, msg, ##__VA_ARGS__)
#define LOG_ERR(ch, msg, ...)        AppLogger::instance().error  (ch, msg, ##__VA_ARGS__)
#define LOG_CRIT(ch, msg, ...)       AppLogger::instance().critical(ch, msg, ##__VA_ARGS__)
