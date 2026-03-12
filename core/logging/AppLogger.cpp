#include "AppLogger.h"
#include <QDateTime>
#include <QMutexLocker>

AppLogger& AppLogger::instance()
{
    static AppLogger logger;
    return logger;
}

void AppLogger::log(LogLevel level, LogChannel channel,
                    const QString& message, const QString& detail,
                    const QString& partId, int sheetIndex)
{
    LogEntry entry;
    entry.timestamp  = QDateTime::currentDateTime();
    entry.level      = level;
    entry.channel    = channel;
    entry.message    = message;
    entry.detail     = detail;
    entry.partId     = partId;
    entry.sheetIndex = sheetIndex;

    {
        QMutexLocker lock(&m_mutex);
        m_entries.push_back(entry);
    }

    // Сигнал — всегда через очередь, безопасно из любого потока
    emit entryAdded(entry);
}

void AppLogger::debug(LogChannel ch, const QString& msg, const QString& detail)
{
    log(LogLevel::DEBUG, ch, msg, detail);
}

void AppLogger::info(LogChannel ch, const QString& msg, const QString& detail)
{
    log(LogLevel::INFO, ch, msg, detail);
}

void AppLogger::warning(LogChannel ch, const QString& msg, const QString& detail)
{
    log(LogLevel::WARNING, ch, msg, detail);
}

void AppLogger::error(LogChannel ch, const QString& msg, const QString& detail)
{
    log(LogLevel::ERROR, ch, msg, detail);
}

void AppLogger::critical(LogChannel ch, const QString& msg, const QString& detail)
{
    log(LogLevel::CRITICAL, ch, msg, detail);
}

void AppLogger::beginOperation(const QString& name)
{
    {
        QMutexLocker lock(&m_mutex);
        m_operationStart[name.toStdString()] =
            QDateTime::currentDateTime().toMSecsSinceEpoch();
    }
    log(LogLevel::DEBUG, LogChannel::SYSTEM,
        QString("→ START: %1").arg(name));
}

void AppLogger::endOperation(const QString& name)
{
    qint64 elapsed = 0;
    {
        QMutexLocker lock(&m_mutex);
        auto it = m_operationStart.find(name.toStdString());
        if (it != m_operationStart.end()) {
            elapsed = QDateTime::currentDateTime().toMSecsSinceEpoch() - it->second;
            m_operationStart.erase(it);
        }
    }
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level     = LogLevel::DEBUG;
    entry.channel   = LogChannel::SYSTEM;
    entry.message   = QString("← END: %1").arg(name);
    entry.elapsedMs = elapsed;
    entry.detail    = QString("Elapsed: %1 мс").arg(elapsed);

    {
        QMutexLocker lock(&m_mutex);
        m_entries.push_back(entry);
    }
    emit entryAdded(entry);
}

std::vector<LogEntry> AppLogger::entries() const
{
    QMutexLocker lock(&m_mutex);
    return m_entries;
}

void AppLogger::clear()
{
    QMutexLocker lock(&m_mutex);
    m_entries.clear();
}
