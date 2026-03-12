#include "LogTab.h"
#include "ui/widgets/LogWidget.h"
#include "core/logging/AppLogger.h"

#include <QVBoxLayout>
#include <QLabel>

LogTab::LogTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
}

void LogTab::buildUi()
{
    m_logWidget = new LogWidget(this);

    m_statusBar = new QLabel("Записей: 0", this);
    m_statusBar->setContentsMargins(4, 2, 4, 2);
    m_statusBar->setStyleSheet("font-size: 11px; color: #888;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_logWidget);
    layout->addWidget(m_statusBar);

    connect(&AppLogger::instance(), &AppLogger::entryAdded,
            this, &LogTab::updateStatusBar,
            Qt::QueuedConnection);
}

LogWidget* LogTab::logWidget() const { return m_logWidget; }

void LogTab::updateStatusBar()
{
    const auto& entries = AppLogger::instance().entries();
    int warns = 0, errs = 0;
    for (const auto& e : entries) {
        if (e.level == LogLevel::WARNING)                  ++warns;
        if (e.level >= LogLevel::ERROR)                    ++errs;
    }
    m_statusBar->setText(
        QString("Всего: %1  |  ⚠ %2  |  ✖ %3")
        .arg(entries.size()).arg(warns).arg(errs));
}
