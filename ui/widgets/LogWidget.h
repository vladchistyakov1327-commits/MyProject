#pragma once
#include "core/logging/LogEntry.h"
#include <QTableView>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <vector>

/**
 * Модель данных для таблицы логов.
 */
class LogTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit LogTableModel(QObject* parent = nullptr);

    int  rowCount   (const QModelIndex& = {}) const override;
    int  columnCount(const QModelIndex& = {}) const override;
    QVariant data      (const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void appendEntry(const LogEntry& entry);
    void setEntries (const std::vector<LogEntry>& entries);
    void clear();

    const LogEntry& entryAt(int row) const;

private:
    std::vector<LogEntry> m_entries;
};

/**
 * Виджет таблицы логов с фильтрами.
 * Подключается к AppLogger::entryAdded.
 */
class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget* parent = nullptr);

    void setAutoScroll(bool on) { m_autoScroll = on; }

public slots:
    void appendEntry(const LogEntry& entry);
    void setEntries (const std::vector<LogEntry>& entries);
    void clearLog();
    void saveToFile();

signals:
    void entryDoubleClicked(LogEntry entry);

private slots:
    void onChannelChanged(int index);
    void onLevelChanged  (int index);
    void onSearchChanged (const QString& text);
    void onDoubleClick   (const QModelIndex& index);
    void scrollToBottom  ();

private:
    LogTableModel*         m_model  = nullptr;
    QSortFilterProxyModel* m_proxy  = nullptr;
    QTableView*            m_view   = nullptr;
    bool                   m_autoScroll = true;
    bool                   m_userScrolled = false;

    void buildUi();
};
