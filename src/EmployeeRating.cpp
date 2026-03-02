#include "EmployeeRating.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDebug>

EmployeeRating::EmployeeRating() {}

QVector<EmployeeRating::RatingEntry> EmployeeRating::queryForPeriod(
    const QString &dateStart, const QString &dateEnd)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QVector<RatingEntry> list;

    QSqlQuery query(db);
    query.prepare(
        "SELECT e.id, e.name, e.position, e.monthly_plan, "
        "COALESCE(SUM(p.fact), 0) AS total_earnings, "
        "COUNT(p.id) AS op_count "
        "FROM employees e "
        "LEFT JOIN performed p ON p.employee_id = e.id "
        "  AND p.date >= :start AND p.date <= :end "
        "GROUP BY e.id, e.name, e.position, e.monthly_plan "
        "ORDER BY total_earnings DESC"
    );
    query.bindValue(":start", dateStart);
    query.bindValue(":end",   dateEnd);

    if (!query.exec()) {
        qDebug() << "EmployeeRating query error:" << query.lastError().text();
        return list;
    }

    while (query.next()) {
        RatingEntry e;
        e.employeeId     = query.value(0).toInt();
        e.name           = query.value(1).toString();
        e.position       = query.value(2).toString();
        e.monthlyPlan    = query.value(3).toDouble();
        e.earnings       = query.value(4).toDouble();
        e.operationCount = query.value(5).toInt();
        e.avgOperation   = (e.operationCount > 0) ? (e.earnings / e.operationCount) : 0.0;
        e.planPercent    = (e.monthlyPlan > 0)    ? (e.earnings / e.monthlyPlan * 100.0) : 0.0;
        e.bonus          = (e.earnings > e.monthlyPlan && e.monthlyPlan > 0)
                             ? e.earnings * 0.10 : 0.0;
        list.append(e);
    }
    return list;
}

QVector<EmployeeRating::RatingEntry> EmployeeRating::calculateCurrentMonth() {
    QDate today = QDate::currentDate();
    QDate monthStart(today.year(), today.month(), 1);
    QDate monthEnd(today.year(), today.month(), today.daysInMonth());
    return queryForPeriod(monthStart.toString(Qt::ISODate),
                          monthEnd.toString(Qt::ISODate));
}

QVector<EmployeeRating::RatingEntry> EmployeeRating::historyForMonth(int month, int year) {
    QDate monthStart(year, month, 1);
    QDate monthEnd(year, month, monthStart.daysInMonth());
    return queryForPeriod(monthStart.toString(Qt::ISODate),
                          monthEnd.toString(Qt::ISODate));
}

void EmployeeRating::updatePersonalPlan(const QString &employee, double plan) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE employees SET monthly_plan = :plan WHERE name = :name");
    q.bindValue(":plan", plan);
    q.bindValue(":name", employee);
    if (!q.exec()) {
        qDebug() << "updatePersonalPlan error:" << q.lastError().text();
    }
}

QStringList EmployeeRating::badgesFor(int employeeId) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QStringList badges;
    QSqlQuery q(db);
    q.prepare("SELECT badge_name FROM achievements "
              "WHERE employee_id = :id ORDER BY earned_date DESC");
    q.bindValue(":id", employeeId);
    if (!q.exec()) {
        qDebug() << "badgesFor error:" << q.lastError().text();
        return badges;
    }
    while (q.next()) {
        badges << q.value(0).toString();
    }
    return badges;
}

void EmployeeRating::updateBadges(int month, int year) {
    auto entries = historyForMonth(month, year);
    QSqlDatabase db = DatabaseManager::instance().database();
    QDate earnDate(year, month, 1);
    QString earnDateStr = earnDate.toString(Qt::ISODate);

    auto awardBadge = [&](int empId, const QString &badge) {
        QSqlQuery check(db);
        check.prepare(
            "SELECT COUNT(*) FROM achievements "
            "WHERE employee_id = :id AND badge_name = :badge "
            "AND strftime('%Y-%m', earned_date) = :ym"
        );
        check.bindValue(":id",    empId);
        check.bindValue(":badge", badge);
        check.bindValue(":ym",    earnDate.toString("yyyy-MM"));
        if (check.exec() && check.next() && check.value(0).toInt() > 0) return;

        QSqlQuery ins(db);
        ins.prepare("INSERT INTO achievements(employee_id, badge_name, earned_date) "
                    "VALUES(:id,:badge,:date)");
        ins.bindValue(":id",    empId);
        ins.bindValue(":badge", badge);
        ins.bindValue(":date",  earnDateStr);
        ins.exec();
    };

    for (int i = 0; i < entries.size(); ++i) {
        const auto &e = entries[i];
        if (i == 0) awardBadge(e.employeeId, "🏆 Мастер месяца");
        if (i == 1) awardBadge(e.employeeId, "🥈 Серебряный призер");
        if (i == 2) awardBadge(e.employeeId, "🥉 Бронзовый призер");
        if (e.operationCount > 100) awardBadge(e.employeeId, "💪 Трудяга");

        // Check record growth vs previous month
        int prevMonth = (month == 1) ? 12 : month - 1;
        int prevYear  = (month == 1) ? year - 1 : year;
        auto prev = historyForMonth(prevMonth, prevYear);
        for (const auto &pe : prev) {
            if (pe.employeeId == e.employeeId && pe.earnings > 0) {
                double growth = (e.earnings - pe.earnings) / pe.earnings * 100.0;
                if (growth >= 50.0) awardBadge(e.employeeId, "⚡ Рекордсмен");
                break;
            }
        }
    }

    // 🚀 Лидер роста — employee with biggest absolute earnings increase
    if (!entries.isEmpty()) {
        int prevMonth = (month == 1) ? 12 : month - 1;
        int prevYear  = (month == 1) ? year - 1 : year;
        auto prev = historyForMonth(prevMonth, prevYear);
        double maxGrowth = 0;
        int growthLeaderId = -1;
        for (const auto &e : entries) {
            for (const auto &pe : prev) {
                if (pe.employeeId == e.employeeId) {
                    double g = e.earnings - pe.earnings;
                    if (g > maxGrowth) { maxGrowth = g; growthLeaderId = e.employeeId; }
                    break;
                }
            }
        }
        if (growthLeaderId >= 0) {
            awardBadge(growthLeaderId, "🚀 Лидер роста");
        }
    }
}
