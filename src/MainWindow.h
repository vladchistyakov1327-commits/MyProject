#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QToolButton>
#include <QLabel>

class DashboardForm;
class ProductsForm;
class NormsForm;
class MaterialsForm;
class ReceiptForm;
class ShipmentForm;
class MovementForm;
class EmployeesForm;
class SalaryForm;
class PerformedForm;
class RatingForm;
class NotificationsForm;
class NotificationSystem;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onNewNotification(int type, const QString &message, int priority);
    void onBellClicked();
    void openNotificationSettings();

private:
    void setupUI();
    void setupToolbar();
    void setupNotificationSystem();
    void updateBellBadge();

    QTabWidget *m_tabWidget;

    DashboardForm     *m_dashboardForm;
    ProductsForm      *m_productsForm;
    NormsForm         *m_normsForm;
    MaterialsForm     *m_materialsForm;
    ReceiptForm       *m_receiptForm;
    ShipmentForm      *m_shipmentForm;
    MovementForm      *m_movementForm;
    EmployeesForm     *m_employeesForm;
    SalaryForm        *m_salaryForm;
    PerformedForm     *m_performedForm;
    RatingForm        *m_ratingForm;
    NotificationsForm *m_notificationsForm;

    QToolButton *m_bellBtn;
    QLabel      *m_bellBadge;
    int          m_unreadCount;

    NotificationSystem *m_notifSystem;

    // tab indices for quick navigation
    int m_notifTabIdx;
    int m_salaryTabIdx;
    int m_materialsTabIdx;
};

#endif // MAINWINDOW_H
