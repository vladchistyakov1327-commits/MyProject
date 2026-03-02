#include "MainWindow.h"
#include "DashboardForm.h"
#include "ProductsForm.h"
#include "NormsForm.h"
#include "MaterialsForm.h"
#include "ReceiptForm.h"
#include "ShipmentForm.h"
#include "MovementForm.h"
#include "EmployeesForm.h"
#include "SalaryForm.h"
#include "PerformedForm.h"
#include "RatingForm.h"
#include "NotificationsForm.h"
#include "NotificationSystem.h"
#include "NotificationSettingsDialog.h"
#include "DatabaseManager.h"

#include <QToolBar>
#include <QWidget>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QAction>
#include <QFont>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_unreadCount(0)
    , m_notifTabIdx(0)
    , m_salaryTabIdx(0)
    , m_materialsTabIdx(0)
{
    if (!DatabaseManager::instance().open("production.db")) {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось открыть базу данных");
    }

    setupUI();
    setupToolbar();
    setupNotificationSystem();

    setWindowTitle("Production Manager Pro — Управление Производством");
    resize(1400, 800);
}

void MainWindow::setupUI() {
    m_tabWidget = new QTabWidget(this);

    // 1. Dashboard + доска почёта
    m_dashboardForm = new DashboardForm();
    m_tabWidget->addTab(m_dashboardForm, "Дашборд");

    // 2. Справочник изделий с себестоимостью
    m_productsForm = new ProductsForm();
    m_tabWidget->addTab(m_productsForm, "Справочники");

    // 3. Нормы расхода материалов
    m_normsForm = new NormsForm();
    m_tabWidget->addTab(m_normsForm, "Нормы расхода");

    // 4. Справочник материалов (склад)
    m_materialsForm = new MaterialsForm();
    m_materialsTabIdx = m_tabWidget->count();
    m_tabWidget->addTab(m_materialsForm, "Материалы");

    // 5. Приход материалов
    m_receiptForm = new ReceiptForm();
    m_tabWidget->addTab(m_receiptForm, "Приход");

    // 6. Отгрузка
    m_shipmentForm = new ShipmentForm();
    m_tabWidget->addTab(m_shipmentForm, "Отгрузка");

    // 7. Движение материалов (журнал)
    m_movementForm = new MovementForm();
    m_tabWidget->addTab(m_movementForm, "Движение");

    // 8. Штат сотрудников
    m_employeesForm = new EmployeesForm();
    m_tabWidget->addTab(m_employeesForm, "Штат");

    // 9. Выполненные операции
    m_performedForm = new PerformedForm();
    m_tabWidget->addTab(m_performedForm, "Выполнено");

    // 10. Зарплата
    m_salaryForm = new SalaryForm();
    m_salaryTabIdx = m_tabWidget->count();
    m_tabWidget->addTab(m_salaryForm, "Зарплата");

    // 11. Рейтинг и геймификация
    m_ratingForm = new RatingForm();
    m_tabWidget->addTab(m_ratingForm, "Рейтинг");

    // 12. Журнал уведомлений
    m_notificationsForm = new NotificationsForm();
    m_notifTabIdx = m_tabWidget->count();
    m_tabWidget->addTab(m_notificationsForm, "Уведомления");

    setCentralWidget(m_tabWidget);
}

void MainWindow::setupToolbar() {
    QToolBar *toolbar = addToolBar("Главная панель");
    toolbar->setMovable(false);

    // Push everything to the right
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // Settings action
    QAction *settingsAct = new QAction("Настройки уведомлений", this);
    connect(settingsAct, &QAction::triggered, this, &MainWindow::openNotificationSettings);
    toolbar->addAction(settingsAct);

    // Bell + badge widget
    QWidget *bellWidget = new QWidget(toolbar);
    QHBoxLayout *bellLay = new QHBoxLayout(bellWidget);
    bellLay->setContentsMargins(6, 0, 6, 0);
    bellLay->setSpacing(2);

    m_bellBtn = new QToolButton(bellWidget);
    m_bellBtn->setText("Уведомления");
    m_bellBtn->setToolTip("Открыть журнал уведомлений");
    m_bellBtn->setAutoRaise(true);
    connect(m_bellBtn, &QToolButton::clicked, this, &MainWindow::onBellClicked);

    m_bellBadge = new QLabel("", bellWidget);
    m_bellBadge->setStyleSheet(
        "background: red; color: white; border-radius: 8px;"
        "min-width:18px; max-width:18px; min-height:18px; max-height:18px;"
        "font-size:10px; font-weight:bold; padding: 0px;");
    m_bellBadge->setAlignment(Qt::AlignCenter);
    m_bellBadge->setVisible(false);

    bellLay->addWidget(m_bellBtn);
    bellLay->addWidget(m_bellBadge);
    toolbar->addWidget(bellWidget);
}

void MainWindow::setupNotificationSystem() {
    m_notifSystem = new NotificationSystem(this);

    connect(m_notifSystem, &NotificationSystem::newNotification,
            this, &MainWindow::onNewNotification);

    // Run startup checks after UI is ready
    QTimer::singleShot(800, m_notifSystem, &NotificationSystem::checkAll);
}

void MainWindow::onNewNotification(int type, const QString &message, int priority) {
    m_notificationsForm->appendNotification(type, message, priority);
    m_dashboardForm->addNotification(message);

    ++m_unreadCount;
    updateBellBadge();
}

void MainWindow::onBellClicked() {
    m_tabWidget->setCurrentIndex(m_notifTabIdx);
    m_unreadCount = 0;
    updateBellBadge();
}

void MainWindow::openNotificationSettings() {
    NotificationSettingsDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_notifSystem->setSalaryThreshold(dlg.salaryThreshold());
    m_notifSystem->setPayrollDay(dlg.payrollDay());
    m_notifSystem->setShowOnStartup(dlg.showOnStartup());

    // Re-run with updated settings
    m_notifSystem->checkAll();
}

void MainWindow::updateBellBadge() {
    if (m_unreadCount > 0) {
        m_bellBadge->setText(m_unreadCount > 99 ? "99+" : QString::number(m_unreadCount));
        m_bellBadge->setVisible(true);
    } else {
        m_bellBadge->setVisible(false);
    }
}
