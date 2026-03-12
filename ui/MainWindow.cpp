#include "MainWindow.h"
#include "TopBar.h"
#include "AppStyle.h"

#include "ui/canvas/NestCanvas.h"
#include "ui/widgets/ProgressOverlay.h"
#include "ui/tabs/PartListTab.h"
#include "ui/tabs/NestSettingsTab.h"
#include "ui/tabs/SheetsTab.h"
#include "ui/tabs/LogTab.h"
#include "ui/dialogs/ImportDxfDialog.h"
#include "ui/dialogs/SheetSetupDialog.h"
#include "ui/dialogs/ExportDialog.h"

#include "core/nesting/NestEngine.h"
#include "core/logging/AppLogger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QLabel>
#include <QStatusBar>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFutureWatcher>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("NestingPro");
    setMinimumSize(1100, 700);
    resize(1400, 860);

    m_engine  = new NestEngine(this);
    m_watcher = new QFutureWatcher<NestResult>(this);

    buildUi();
    setupConnections();

    LOG_INFO(LogChannel::UI, "NestingPro запущен");
}

MainWindow::~MainWindow() = default;

// ─────────────────────────────────────────────────────────────────────────────
// Построение UI
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::buildUi()
{
    // Центральный виджет
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* rootLay = new QVBoxLayout(central);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    // Верхняя панель
    m_topBar = new TopBar(this);
    rootLay->addWidget(m_topBar);

    // Горизонтальный разделитель: боковая панель + канвас
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(4);

    // ── Боковая панель ─────────────────────────────────────────────────
    m_sideTabWidget = new QTabWidget(m_mainSplitter);
    m_sideTabWidget->setMinimumWidth(280);
    m_sideTabWidget->setMaximumWidth(480);
    m_sideTabWidget->tabBar()->setExpanding(true);

    m_partListTab  = new PartListTab(m_sideTabWidget);
    m_settingsTab  = new NestSettingsTab(m_sideTabWidget);
    m_sheetsTab    = new SheetsTab(m_sideTabWidget);

    m_sideTabWidget->addTab(m_partListTab,  "Детали");
    m_sideTabWidget->addTab(m_settingsTab,  "Настройки");
    m_sideTabWidget->addTab(m_sheetsTab,    "Листы");

    // ── Правая часть: холст + оверлей ──────────────────────────────────
    auto* canvasContainer = new QWidget(m_mainSplitter);
    auto* canvasLay = new QVBoxLayout(canvasContainer);
    canvasLay->setContentsMargins(0, 0, 0, 0);
    canvasLay->setSpacing(0);

    m_canvas = new NestCanvas(canvasContainer);
    canvasLay->addWidget(m_canvas);

    // Оверлей — дочерний виджет холста, перекрывает его
    m_overlay = new ProgressOverlay(m_canvas);
    m_overlay->setGeometry(0, 0, m_canvas->width(), m_canvas->height());

    m_mainSplitter->addWidget(m_sideTabWidget);
    m_mainSplitter->addWidget(canvasContainer);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setSizes({340, 900});

    // ── Вертикальный разделитель: основная часть + нижние вкладки ─────
    auto* vertSplit = new QSplitter(Qt::Vertical, this);
    vertSplit->addWidget(m_mainSplitter);

    m_bottomTabs = new QTabWidget(vertSplit);
    m_bottomTabs->setMaximumHeight(220);
    m_bottomTabs->setMinimumHeight(80);
    m_logTab = new LogTab(m_bottomTabs);
    m_bottomTabs->addTab(m_logTab, "Лог");

    vertSplit->addWidget(m_bottomTabs);
    vertSplit->setStretchFactor(0, 1);
    vertSplit->setStretchFactor(1, 0);
    vertSplit->setSizes({640, 180});

    rootLay->addWidget(vertSplit);

    // Строка состояния
    m_statusLabel = new QLabel("Готово", this);
    statusBar()->addWidget(m_statusLabel, 1);

    // Начальные данные
    m_settingsTab->setParameters(m_job.params);
    m_sheetsTab->setSheetDefinition(m_job.sheet);
}

// ─────────────────────────────────────────────────────────────────────────────
// Соединения сигналов/слотов
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupConnections()
{
    // TopBar
    connect(m_topBar, &TopBar::importDxfRequested,   this, &MainWindow::onImportDxf);
    connect(m_topBar, &TopBar::sheetSetupRequested,  this, &MainWindow::onSheetSetup);
    connect(m_topBar, &TopBar::startNestRequested,   this, &MainWindow::onStartNest);
    connect(m_topBar, &TopBar::cancelNestRequested,  this, &MainWindow::onCancelNest);
    connect(m_topBar, &TopBar::exportRequested,      this, &MainWindow::onExport);

    // Боковые вкладки → обновить задание
    connect(m_partListTab,  &PartListTab::partsChanged, [this] {
        m_job.parts = m_partListTab->partEntries();
    });
    connect(m_settingsTab, &NestSettingsTab::parametersChanged, [this](const NestParameters& p) {
        m_job.params = p;
    });
    connect(m_sheetsTab, &SheetsTab::sheetChanged, [this] {
        m_job.sheet = m_sheetsTab->sheetDefinition();
    });

    // Выбор детали на холсте → выделить в списке
    connect(m_canvas, &NestCanvas::partSelected, [this](const QString& partId) {
        m_statusLabel->setText(partId.isEmpty()
            ? "Готово"
            : QString("Выбрана деталь: %1").arg(partId));
    });

    // Оверлей — кнопка отмены
    connect(m_overlay, &ProgressOverlay::cancelRequested,
            this, &MainWindow::onCancelNest);

    // FutureWatcher
    connect(m_watcher, &QFutureWatcher<NestResult>::progressValueChanged,
            [this](int v) {
                m_overlay->updateProgress(v,
                    QString("Расчёт: %1%").arg(v));
            });
    connect(m_watcher, &QFutureWatcher<NestResult>::finished,
            [this] {
                onNestFinished(m_watcher->result());
            });

    // NestEngine → прогресс
    connect(m_engine, &NestEngine::progressChanged,
            this, &MainWindow::onNestProgress,
            Qt::QueuedConnection);
}

// ─────────────────────────────────────────────────────────────────────────────
// Действия
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onImportDxf()
{
    if (!confirmCancelIfRunning()) return;

    ImportDxfDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    const auto parts = dlg.importedParts();
    if (parts.empty()) return;

    // Добавить детали к заданию
    for (const auto& pe : parts)
        m_job.parts.push_back(pe);

    m_partListTab->setNestJob(&m_job);
    m_statusLabel->setText(
        QString("Импортировано деталей: %1").arg(parts.size()));
}

void MainWindow::onSheetSetup()
{
    if (!confirmCancelIfRunning()) return;

    SheetSetupDialog dlg(this, m_job.sheet);
    if (dlg.exec() != QDialog::Accepted) return;

    m_job.sheet = dlg.result();
    m_sheetsTab->setSheetDefinition(m_job.sheet);
}

void MainWindow::onStartNest()
{
    if (m_engine->isRunning()) return;

    if (m_job.parts.empty()) {
        QMessageBox::information(this, "Расчёт",
            "Нет деталей для раскладки. Импортируйте DXF-файлы.");
        return;
    }

    // Обновить задание из боковой панели
    m_job.params = m_settingsTab->parameters();
    m_job.sheet  = m_sheetsTab->sheetDefinition();
    m_job.parts  = m_partListTab->partEntries();

    m_canvas->clearLayout();
    m_overlay->showOverlay();
    m_topBar->setRunning(true);
    m_statusLabel->setText("Выполняется расчёт…");

    const QFuture<NestResult> future = m_engine->runAsync(m_job);
    m_watcher->setFuture(future);

    LOG_INFO(LogChannel::NESTING,
             QString("Запуск расчёта: %1 деталей, лист %2×%3 мм")
             .arg(m_job.parts.size())
             .arg(m_job.sheet.widthMm)
             .arg(m_job.sheet.heightMm));
}

void MainWindow::onCancelNest()
{
    m_engine->cancel();
    m_overlay->updateProgress(0, "Отмена расчёта…");
    m_statusLabel->setText("Отмена расчёта…");
}

void MainWindow::onExport()
{
    if (m_lastResult.sheets.empty()) {
        QMessageBox::information(this, "Экспорт",
            "Нет результатов для экспорта. Запустите расчёт.");
        return;
    }
    ExportDialog dlg(m_lastResult, this);
    dlg.exec();
}

void MainWindow::onNestProgress(int percent, int placed, int total, int currentSheet)
{
    const QString txt = QString("Расчёт: %1%  |  Расположено: %2/%3  |  Лист: %4")
        .arg(percent).arg(placed).arg(total).arg(currentSheet + 1);
    m_overlay->updateProgress(percent, txt);
    m_statusLabel->setText(txt);
}

void MainWindow::onNestFinished(NestResult result)
{
    m_lastResult = std::move(result);

    m_overlay->hideOverlay();
    m_topBar->setRunning(false);

    if (!m_lastResult.sheets.empty()) {
        m_canvas->setResult(m_lastResult, m_job.sheet.widthMm,
                            m_job.sheet.heightMm);
        m_sheetsTab->updateResults(m_lastResult);
    }

    const int placed   = m_lastResult.totalPlacedCount;
    const int unplaced = (int)m_lastResult.unplacedPartIds.size();
    const double util  = m_lastResult.sheets.empty() ? 0.0
        : m_lastResult.sheets.front().utilizationPercent;

    const QString status = m_lastResult.cancelled
        ? "Расчёт отменён"
        : QString("Завершено: %1 дет. размещено, %2 не размещено. "
                  "Утилизация 1-го листа: %3%")
          .arg(placed).arg(unplaced).arg(util, 0, 'f', 1);

    m_statusLabel->setText(status);
    LOG_INFO(LogChannel::NESTING, status);
}

// ─────────────────────────────────────────────────────────────────────────────

bool MainWindow::confirmCancelIfRunning()
{
    if (!m_engine->isRunning()) return true;
    const auto btn = QMessageBox::question(this, "Расчёт в процессе",
        "Расчёт ещё не завершён. Остановить и продолжить?",
        QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) return false;
    m_engine->cancel();
    m_watcher->waitForFinished();
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!confirmCancelIfRunning()) {
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::applyCurrentJob()
{
    m_partListTab->setNestJob(&m_job);
    m_settingsTab->setParameters(m_job.params);
    m_sheetsTab->setSheetDefinition(m_job.sheet);
}
