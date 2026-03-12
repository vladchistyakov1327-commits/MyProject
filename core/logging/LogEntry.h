#pragma once
#include "LogChannel.h"
#include <QString>
#include <QDateTime>

/// Уровни серьёзности записи лога.
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

/// Одна запись в журнале приложения.
struct LogEntry {
    QDateTime  timestamp;
    LogLevel   level         = LogLevel::INFO;
    LogChannel channel       = LogChannel::SYSTEM;
    QString    message;
    QString    detail;       ///< Расширенный контекст (значения, трассировка)
    QString    partId;       ///< Если относится к конкретной детали
    int        sheetIndex    = -1; ///< Если относится к конкретному листу
    qint64     elapsedMs     = 0;  ///< Время от начала последней операции

    /// Строковое представление уровня для UI.
    QString levelString() const {
        switch (level) {
            case LogLevel::DEBUG:    return "DEBUG";
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARN";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRIT";
        }
        return {};
    }

    /// Строковое представление канала для UI.
    QString channelString() const {
        switch (channel) {
            case LogChannel::IMPORT:   return "IMPORT";
            case LogChannel::GEOMETRY: return "GEOM";
            case LogChannel::NESTING:  return "NESTING";
            case LogChannel::COEDGE:   return "COEDGE";
            case LogChannel::EXPORT:   return "EXPORT";
            case LogChannel::UI:       return "UI";
            case LogChannel::SYSTEM:   return "SYSTEM";
        }
        return {};
    }
};
