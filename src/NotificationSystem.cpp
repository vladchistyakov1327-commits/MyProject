#include "NotificationSystem.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDebug>

NotificationSystem::NotificationSystem(QObject *parent)
    : QObject(parent),
      m_salaryThreshold(500.0),
      m_payrollDay(20),
      m_showOnStartup(true) {}

void NotificationSystem::checkAll() {
    checkSalaryMismatch();
    checkPayrollReminder();
    checkLowMaterials();
    checkNegativeMargin();
    checkBirthdays();
}

void NotificationSystem::setSalaryThreshold(double rubles) {
    m_salaryThreshold = rubles;
}

void NotificationSystem::setPayrollDay(int day) {
    m_payrollDay = day;
}

void NotificationSystem::setShowOnStartup(bool show) {
    m_showOnStartup = show;
}

void NotificationSystem::checkSalaryMismatch() {
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare(
        "SELECT e.name, s.difference "
        "FROM salary s "
        "JOIN employees e ON s.employee_id = e.id "
        "WHERE ABS(s.difference) > :threshold"
    );
    query.bindValue(":threshold", m_salaryThreshold);
    if (!query.exec()) {
        qDebug() << "checkSalaryMismatch error:" << query.lastError().text();
        return;
    }
    while (query.next()) {
        QString name = query.value(0).toString();
        double diff = query.value(1).toDouble();
        emit newNotification(
            static_cast<int>(SalaryMismatch),
            QString("⚠️ Несоответствие зарплаты: %1, разница %2 руб. Проверьте выполненные работы!")
                .arg(name).arg(qAbs(diff), 0, 'f', 0),
            1
        );
    }
}

void NotificationSystem::checkPayrollReminder() {
    QDate today = QDate::currentDate();
    int dayOfMonth = today.day();
    int daysUntil = m_payrollDay - dayOfMonth;

    if (daysUntil != 0 && daysUntil != 3) return;

    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    QDate monthStart(today.year(), today.month(), 1);
    query.prepare(
        "SELECT COALESCE(SUM(actual), 0) FROM salary "
        "WHERE month >= :ms AND paid = 0"
    );
    query.bindValue(":ms", monthStart.toString(Qt::ISODate));
    if (!query.exec() || !query.next()) return;

    double unpaid = query.value(0).toDouble();
    if (unpaid <= 0) return;

    QString msg;
    if (daysUntil == 0) {
        msg = QString("💰 Сегодня %1 число — день выплаты зарплаты! Сумма к выплате: %2 руб. Подготовьте ведомость.")
            .arg(m_payrollDay).arg(unpaid, 0, 'f', 0);
    } else {
        msg = QString("💰 Через 3 дня (%1 числа) — день выплаты зарплаты! Сумма к выплате: %2 руб.")
            .arg(m_payrollDay).arg(unpaid, 0, 'f', 0);
    }
    emit newNotification(static_cast<int>(PayrollReminder), msg, 2);
}

void NotificationSystem::checkLowMaterials() {
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    if (!query.exec("SELECT name, current, minimum, unit FROM materials WHERE current < minimum")) {
        qDebug() << "checkLowMaterials error:" << query.lastError().text();
        return;
    }
    while (query.next()) {
        QString name = query.value(0).toString();
        double cur  = query.value(1).toDouble();
        double mini = query.value(2).toDouble();
        QString unit = query.value(3).toString();
        emit newNotification(
            static_cast<int>(LowMaterial),
            QString("⚡ Критический остаток: %1 — осталось %2 %3 (мин. %4 %3)")
                .arg(name).arg(cur, 0, 'f', 2).arg(unit).arg(mini, 0, 'f', 2),
            1
        );
    }
}

void NotificationSystem::checkNegativeMargin() {
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    if (!query.exec(
        "SELECT p.name, p.price, p.production_cost, "
        "COALESCE((SELECT SUM(n.quantity * m.price) "
        "          FROM norms n JOIN materials m ON n.material = m.name "
        "          WHERE n.product = p.name), 0) AS mat_cost "
        "FROM products p"
    )) {
        qDebug() << "checkNegativeMargin error:" << query.lastError().text();
        return;
    }
    while (query.next()) {
        QString name     = query.value(0).toString();
        double price     = query.value(1).toDouble();
        double prodCost  = query.value(2).toDouble();
        double matCost   = query.value(3).toDouble();
        if (price <= 0) continue;
        double fullCost  = matCost + prodCost;
        double margin    = (price - fullCost) / price * 100.0;
        if (margin < 0) {
            emit newNotification(
                static_cast<int>(NegativeMargin),
                QString("📉 Товар '%1' продается в убыток! Маржинальность: %2%. Проверьте цены или нормы расхода.")
                    .arg(name).arg(margin, 0, 'f', 1),
                2
            );
        }
    }
}

void NotificationSystem::checkBirthdays() {
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    QDate today = QDate::currentDate();
    query.prepare(
        "SELECT name FROM employees "
        "WHERE strftime('%m-%d', birth_date) = :md"
    );
    query.bindValue(":md", today.toString("MM-dd"));
    if (!query.exec()) {
        qDebug() << "checkBirthdays error:" << query.lastError().text();
        return;
    }
    while (query.next()) {
        QString name = query.value(0).toString();
        emit newNotification(
            static_cast<int>(Birthday),
            QString("🎂 Сегодня день рождения у %1! Не забудьте поздравить.").arg(name),
            3
        );
    }
}
