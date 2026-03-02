#ifndef EMPLOYEERATING_H
#define EMPLOYEERATING_H

#include <QString>
#include <QVector>
#include <QStringList>

class EmployeeRating {
public:
    struct RatingEntry {
        int employeeId = 0;
        QString name;
        QString position;
        double earnings = 0.0;
        int operationCount = 0;
        double avgOperation = 0.0;
        double monthlyPlan = 0.0;
        double planPercent = 0.0;
        double bonus = 0.0;
    };

    EmployeeRating();

    /// Returns rating for current calendar month, sorted by earnings desc
    QVector<RatingEntry> calculateCurrentMonth();

    /// Returns rating for the given month/year
    QVector<RatingEntry> historyForMonth(int month, int year);

    /// Update monthly plan for an employee (stored in employees table)
    void updatePersonalPlan(const QString &employee, double plan);

    /// Returns list of badge names earned by an employee
    QStringList badgesFor(int employeeId);

    /// Evaluate and persist new badges for all employees after month close
    void updateBadges(int month, int year);

private:
    QVector<RatingEntry> queryForPeriod(const QString &dateStart, const QString &dateEnd);
};

#endif // EMPLOYEERATING_H
