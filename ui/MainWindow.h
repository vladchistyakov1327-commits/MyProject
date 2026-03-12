#pragma once
#include <QMainWindow>
#include "core/nesting/NestJob.h"
#include "core/nesting/NestResult.h"

class TopBar;
class NestCanvas;
class ProgressOverlay;
class PartListTab;
class NestSettingsTab;
class SheetsTab;
class LogTab;
class NestEngine;

template <typename T> class QFutureWatcher;

class QTabWidget;
class QSplitter;
class QLabel;

/**
 * @brief Главное окно приложения NestingPro.
 *
 * Компоновка:
 *   TopBar (48px)
 *   ─────────────────────────────────────────
 *   | Левая панель (340px)  | NestCanvas    |
 *   |  QTabWidget           |               |
 *   |  ─ Детали             |  + Overlay    |
 *   |  ─ Настройки          |               |
 *   |  ─ Листы              |               |
 *   ─────────────────────────────────────────
 *   Нижние вкладки: Лог
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onImportDxf();
    void onSheetSetup();
    void onStartNest();
    void onCancelNest();
    void onExport();

    void onNestProgress(int percent, int placed, int total, int currentSheet);
    void onNestFinished(NestResult result);

private:
    void buildUi();
    void setupConnections();
    void applyCurrentJob();
    bool confirmCancelIfRunning();

    // Layout
    TopBar*          m_topBar        = nullptr;
    QSplitter*       m_mainSplitter  = nullptr;
    QTabWidget*      m_sideTabWidget = nullptr;
    NestCanvas*      m_canvas        = nullptr;
    ProgressOverlay* m_overlay       = nullptr;
    QTabWidget*      m_bottomTabs    = nullptr;

    // Вкладки
    PartListTab*      m_partListTab   = nullptr;
    NestSettingsTab*  m_settingsTab   = nullptr;
    SheetsTab*        m_sheetsTab     = nullptr;
    LogTab*           m_logTab        = nullptr;

    // Бизнес-логика
    NestJob    m_job;
    NestResult m_lastResult;

    NestEngine*                    m_engine  = nullptr;
    QFutureWatcher<NestResult>*    m_watcher = nullptr;

    // Статус-бар
    QLabel*    m_statusLabel = nullptr;
};
