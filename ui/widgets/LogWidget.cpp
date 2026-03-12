#include "LogWidget.h"
#include "AppStyle.h"
#include "core/logging/AppLogger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollBar>
#include <QDialog>
#include <QTextEdit>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

// ── LogTableModel ─────────────────────────────────────────────────────────────

LogTableModel::LogTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int LogTableModel::rowCount(const QModelIndex&) const
{
    return static_cast<int>(m_entries.size());
}

int LogTableModel::columnCount(const QModelIndex&) const { return 6; }

const LogEntry& LogTableModel::entryAt(int row) const
{
    static LogEntry dummy;
    if (row < 0 || row >= (int)m_entries.size()) return dummy;
    return m_entries[row];
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return {};
    switch (section) {
        case 0: return "Время";
        case 1: return "Уровень";
        case 2: return "Канал";
        case 3: return "Сообщение";
        case 4: return "Деталь";
        case 5: return "Лист";
    }
    return {};
}

QVariant LogTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= (int)m_entries.size()) return {};
    const auto& e = m_entries[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return e.timestamp.toString("HH:mm:ss.zzz");
            case 1: return e.levelString();
            case 2: return e.channelString();
            case 3: return e.message;
            case 4: return e.partId.isEmpty() ? QVariant("—") : QVariant(e.partId);
            case 5: return e.sheetIndex < 0 ? QVariant("—") : QVariant(e.sheetIndex + 1);
        }
    }

    if (role == Qt::BackgroundRole) {
        switch (e.level) {
            case LogLevel::ERROR:
            case LogLevel::CRITICAL: return QColor(AppPalette::RED_DIM);
            case LogLevel::WARNING:  {
                QColor c(AppPalette::AMBER_TEXT);
                c.setAlpha(25);
                return c;
            }
            default: break;
        }
    }

    if (role == Qt::ForegroundRole) {
        switch (e.level) {
            case LogLevel::ERROR:
            case LogLevel::CRITICAL: return QColor(AppPalette::RED_TEXT);
            case LogLevel::WARNING:  return QColor(AppPalette::AMBER_TEXT);
            case LogLevel::DEBUG:    return QColor(AppPalette::TEXT_MUTED);
            default:                 return QColor(AppPalette::TEXT_NORMAL);
        }
    }

    if (role == Qt::ToolTipRole && index.column() == 3) {
        return e.detail.isEmpty() ? e.message : (e.message + "\n\n" + e.detail);
    }

    return {};
}

void LogTableModel::appendEntry(const LogEntry& entry)
{
    beginInsertRows({}, rowCount(), rowCount());
    m_entries.push_back(entry);
    endInsertRows();
}

void LogTableModel::setEntries(const std::vector<LogEntry>& entries)
{
    beginResetModel();
    m_entries = entries;
    endResetModel();
}

void LogTableModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

// ── LogWidget ─────────────────────────────────────────────────────────────────

LogWidget::LogWidget(QWidget* parent) : QWidget(parent)
{
    buildUi();
    // Подключиться к глобальному логгеру
    connect(&AppLogger::instance(), &AppLogger::entryAdded,
            this, &LogWidget::appendEntry,
            Qt::QueuedConnection);
}

class LogProxyModel : public QSortFilterProxyModel {
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    LogLevel   minLevel = LogLevel::DEBUG;
    LogChannel filterCh = static_cast<LogChannel>(-1);
    QString    search;

    bool filterAcceptsRow(int row, const QModelIndex& parent) const override {
        auto* m = static_cast<LogTableModel*>(sourceModel());
        const auto& e = m->entryAt(row);
        if (filterCh != static_cast<LogChannel>(-1) && e.channel != filterCh)
            return false;
        if (e.level < minLevel) return false;
        if (!search.isEmpty() &&
            !e.message.contains(search, Qt::CaseInsensitive) &&
            !e.partId.contains(search, Qt::CaseInsensitive))
            return false;
        return true;
    }
};

void LogWidget::buildUi()
{
    m_model = new LogTableModel(this);
    auto* proxy = new LogProxyModel(this);
    proxy->setSourceModel(m_model);
    m_proxy = proxy;

    // ── Фильтры ──────────────────────────────────────────────────────────
    auto* filterBar = new QWidget(this);
    auto* filterLayout = new QHBoxLayout(filterBar);
    filterLayout->setContentsMargins(4, 2, 4, 2);
    filterLayout->setSpacing(6);

    auto* chCombo = new QComboBox(this);
    chCombo->addItem("Все каналы",     -1);
    chCombo->addItem("IMPORT",  static_cast<int>(LogChannel::IMPORT));
    chCombo->addItem("GEOM",    static_cast<int>(LogChannel::GEOMETRY));
    chCombo->addItem("NESTING", static_cast<int>(LogChannel::NESTING));
    chCombo->addItem("COEDGE",  static_cast<int>(LogChannel::COEDGE));
    chCombo->addItem("EXPORT",  static_cast<int>(LogChannel::EXPORT));
    chCombo->addItem("UI",      static_cast<int>(LogChannel::UI));
    chCombo->addItem("SYSTEM",  static_cast<int>(LogChannel::SYSTEM));

    auto* lvlCombo = new QComboBox(this);
    lvlCombo->addItem("Все уровни", -1);
    lvlCombo->addItem("DEBUG",      static_cast<int>(LogLevel::DEBUG));
    lvlCombo->addItem("INFO",       static_cast<int>(LogLevel::INFO));
    lvlCombo->addItem("WARNING",    static_cast<int>(LogLevel::WARNING));
    lvlCombo->addItem("ERROR",      static_cast<int>(LogLevel::ERROR));

    auto* searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Поиск...");

    auto* clearBtn = new QPushButton("Очистить", this);
    auto* saveBtn  = new QPushButton("Сохранить", this);

    filterLayout->addWidget(chCombo);
    filterLayout->addWidget(lvlCombo);
    filterLayout->addWidget(searchEdit, 1);
    filterLayout->addWidget(clearBtn);
    filterLayout->addWidget(saveBtn);

    // ── Таблица ───────────────────────────────────────────────────────────
    m_view = new QTableView(this);
    m_view->setModel(m_proxy);
    m_view->setAlternatingRowColors(true);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->horizontalHeader()->setStretchLastSection(false);
    m_view->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_view->verticalHeader()->hide();
    m_view->setShowGrid(false);
    m_view->setProperty("role", "log");

    // Ширины колонок
    m_view->setColumnWidth(0, 100);
    m_view->setColumnWidth(1, 60);
    m_view->setColumnWidth(2, 70);
    m_view->setColumnWidth(4, 90);
    m_view->setColumnWidth(5, 50);

    // ── Layout ────────────────────────────────────────────────────────────
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(filterBar);
    layout->addWidget(m_view);

    // ── Connections ───────────────────────────────────────────────────────
    connect(chCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogWidget::onChannelChanged);
    connect(lvlCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogWidget::onLevelChanged);
    connect(searchEdit, &QLineEdit::textChanged, this, &LogWidget::onSearchChanged);
    connect(clearBtn,   &QPushButton::clicked,   this, &LogWidget::clearLog);
    connect(saveBtn,    &QPushButton::clicked,   this, &LogWidget::saveToFile);
    connect(m_view,     &QTableView::doubleClicked,
            this, &LogWidget::onDoubleClick);

    // Отслеживать прокрутку пользователем
    connect(m_view->verticalScrollBar(), &QScrollBar::actionTriggered,
            [this](int action) {
                if (action == QAbstractSlider::SliderMove ||
                    action == QAbstractSlider::SliderPageStepSub ||
                    action == QAbstractSlider::SliderSingleStepSub) {
                    m_userScrolled = true;
                    m_autoScroll   = false;
                }
            });
}

void LogWidget::appendEntry(const LogEntry& entry)
{
    m_model->appendEntry(entry);
    if (m_autoScroll && !m_userScrolled) {
        QMetaObject::invokeMethod(this, &LogWidget::scrollToBottom,
                                  Qt::QueuedConnection);
    }
}

void LogWidget::setEntries(const std::vector<LogEntry>& entries)
{
    m_model->setEntries(entries);
    scrollToBottom();
}

void LogWidget::clearLog()
{
    m_model->clear();
    AppLogger::instance().clear();
}

void LogWidget::saveToFile()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Сохранить лог", {},
        "Text files (*.txt);;CSV files (*.csv);;All files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream ts(&file);
    ts.setEncoding(QStringConverter::Utf8);

    ts << "Время\tУровень\tКанал\tСообщение\tДеталь\tЛист\n";
    for (const auto& e : AppLogger::instance().entries()) {
        ts << e.timestamp.toString("HH:mm:ss.zzz") << "\t"
           << e.levelString() << "\t"
           << e.channelString() << "\t"
           << e.message << "\t"
           << e.partId << "\t"
           << (e.sheetIndex >= 0 ? QString::number(e.sheetIndex + 1) : "-")
           << "\n";
    }
    file.close();
}

void LogWidget::onChannelChanged(int index)
{
    auto* p = static_cast<LogProxyModel*>(m_proxy);
    const int v = static_cast<QComboBox*>(sender())->itemData(index).toInt();
    p->filterCh = (v < 0)
        ? static_cast<LogChannel>(-1)
        : static_cast<LogChannel>(v);
    p->invalidateFilter();
}

void LogWidget::onLevelChanged(int index)
{
    auto* p = static_cast<LogProxyModel*>(m_proxy);
    const int v = static_cast<QComboBox*>(sender())->itemData(index).toInt();
    p->minLevel = (v < 0) ? LogLevel::DEBUG : static_cast<LogLevel>(v);
    p->invalidateFilter();
}

void LogWidget::onSearchChanged(const QString& text)
{
    auto* p = static_cast<LogProxyModel*>(m_proxy);
    p->search = text;
    p->invalidateFilter();
}

void LogWidget::onDoubleClick(const QModelIndex& proxyIdx)
{
    const QModelIndex srcIdx = m_proxy->mapToSource(proxyIdx);
    const auto& entry = m_model->entryAt(srcIdx.row());
    emit entryDoubleClicked(entry);

    // Показать детальный диалог
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(QString("Детали: %1").arg(entry.message.left(50)));
    dlg->resize(500, 300);
    auto* layout = new QVBoxLayout(dlg);
    auto* te = new QTextEdit(dlg);
    te->setReadOnly(true);
    te->setPlainText(
        QString("Время: %1\nУровень: %2\nКанал: %3\nДеталь: %4\nЛист: %5\n\n"
                "Сообщение:\n%6\n\nПодробности:\n%7")
        .arg(entry.timestamp.toString("dd.MM.yyyy HH:mm:ss.zzz"))
        .arg(entry.levelString())
        .arg(entry.channelString())
        .arg(entry.partId.isEmpty() ? "—" : entry.partId)
        .arg(entry.sheetIndex < 0 ? "—" : QString::number(entry.sheetIndex + 1))
        .arg(entry.message)
        .arg(entry.detail.isEmpty() ? "—" : entry.detail));
    layout->addWidget(te);
    dlg->exec();
    dlg->deleteLater();
}

void LogWidget::scrollToBottom()
{
    m_view->scrollToBottom();
}
