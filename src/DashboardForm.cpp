#include "DashboardForm.h"
#include "EmployeeRating.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSqlQuery>
#include <QDate>
#include <QTime>
#include <QFont>
#include <QDebug>

DashboardForm::DashboardForm(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    refresh();

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &DashboardForm::refresh);
    m_refreshTimer->start(60 * 1000); // refresh every minute
}

void DashboardForm::setupUI() {
    QHBoxLayout *mainRow = new QHBoxLayout(this);

    // ---- Left: Honor board ----
    QGroupBox *honorBox = new QGroupBox("🏆 ДОСКА ПОЧЁТА", this);
    QVBoxLayout *honorLayout = new QVBoxLayout(honorBox);

    m_honorTitle = new QLabel("Текущий месяц", honorBox);
    QFont boldFont = m_honorTitle->font();
    boldFont.setBold(true);
    boldFont.setPointSize(10);
    m_honorTitle->setFont(boldFont);
    m_honorTitle->setAlignment(Qt::AlignCenter);

    m_place1 = new QLabel("🥇 —", honorBox);
    m_place2 = new QLabel("🥈 —", honorBox);
    m_place3 = new QLabel("🥉 —", honorBox);

    QFont placeFont;
    placeFont.setPointSize(12);
    m_place1->setFont(placeFont);
    m_place2->setFont(placeFont);
    m_place3->setFont(placeFont);

    m_place1->setAlignment(Qt::AlignCenter);
    m_place2->setAlignment(Qt::AlignCenter);
    m_place3->setAlignment(Qt::AlignCenter);

    honorLayout->addWidget(m_honorTitle);
    honorLayout->addSpacing(8);
    honorLayout->addWidget(m_place1);
    honorLayout->addWidget(m_place2);
    honorLayout->addWidget(m_place3);
    honorLayout->addStretch();
    honorBox->setMinimumWidth(230);

    // ---- Center: Summary ----
    QGroupBox *summaryBox = new QGroupBox("📊 СВОДКА", this);
    QVBoxLayout *summaryLayout = new QVBoxLayout(summaryBox);
    m_summaryLabel = new QLabel(summaryBox);
    m_summaryLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_summaryLabel->setWordWrap(true);
    summaryLayout->addWidget(m_summaryLabel);
    summaryBox->setMinimumWidth(280);

    // ---- Right: Notifications feed ----
    QGroupBox *notifBox = new QGroupBox("🔔 Последние уведомления", this);
    QVBoxLayout *notifLayout = new QVBoxLayout(notifBox);
    m_notifList = new QListWidget(notifBox);
    m_notifList->setWordWrap(true);
    notifLayout->addWidget(m_notifList);
    notifBox->setMinimumWidth(320);

    mainRow->addWidget(honorBox);
    mainRow->addWidget(summaryBox);
    mainRow->addWidget(notifBox);
    setLayout(mainRow);
}

void DashboardForm::refresh() {
    updateHonorBoard();
    updateSummary();
}

void DashboardForm::updateHonorBoard() {
    EmployeeRating rating;
    auto entries = rating.calculateCurrentMonth();

    QDate today = QDate::currentDate();
    m_honorTitle->setText(
        QString("Текущий месяц: %1 %2")
            .arg(today.toString("MMMM"))
            .arg(today.year())
    );

    auto fmt = [](int idx, const EmployeeRating::RatingEntry &e) -> QString {
        static const char *medals[] = {"🥇", "🥈", "🥉"};
        return QString("%1 %2. %3\n   %4 руб.")
            .arg(medals[idx])
            .arg(idx + 1)
            .arg(e.name)
            .arg(e.earnings, 0, 'f', 0);
    };

    if (entries.size() > 0) m_place1->setText(fmt(0, entries[0]));
    else m_place1->setText("🥇 —");
    if (entries.size() > 1) m_place2->setText(fmt(1, entries[1]));
    else m_place2->setText("🥈 —");
    if (entries.size() > 2) m_place3->setText(fmt(2, entries[2]));
    else m_place3->setText("🥉 —");
}

void DashboardForm::updateSummary() {
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QDate today = QDate::currentDate();
    QDate monthStart(today.year(), today.month(), 1);

    // Total shipments this month
    QSqlQuery shipQ(db);
    shipQ.prepare("SELECT COUNT(*), COALESCE(SUM(quantity),0) FROM shipments "
                  "WHERE date >= :d AND status = 'Отгружено'");
    shipQ.bindValue(":d", monthStart.toString(Qt::ISODate));
    shipQ.exec();
    int shipCount = 0; double shipQty = 0;
    if (shipQ.next()) { shipCount = shipQ.value(0).toInt(); shipQty = shipQ.value(1).toDouble(); }

    // Low stock materials
    QSqlQuery lowQ(db);
    lowQ.exec("SELECT COUNT(*) FROM materials WHERE current < minimum");
    int lowCount = 0;
    if (lowQ.next()) lowCount = lowQ.value(0).toInt();

    // Total salary unpaid this month
    QSqlQuery salQ(db);
    salQ.prepare("SELECT COALESCE(SUM(actual),0) FROM salary WHERE month >= :d AND paid = 0");
    salQ.bindValue(":d", monthStart.toString(Qt::ISODate));
    salQ.exec();
    double unpaidSalary = 0;
    if (salQ.next()) unpaidSalary = salQ.value(0).toDouble();

    // Employee count
    QSqlQuery empQ(db);
    empQ.exec("SELECT COUNT(*) FROM employees");
    int empCount = 0;
    if (empQ.next()) empCount = empQ.value(0).toInt();

    m_summaryLabel->setText(
        QString(
            "<b>Дата:</b> %1<br><br>"
            "<b>Сотрудников:</b> %2<br>"
            "<b>Отгрузок в этом месяце:</b> %3 (%4 шт.)<br>"
            "<b>Зарплата к выплате:</b> %5 руб.<br>"
            "<b>Материалов ниже минимума:</b> <font color='%6'>%7</font>"
        )
        .arg(today.toString("dd.MM.yyyy"))
        .arg(empCount)
        .arg(shipCount).arg(shipQty, 0, 'f', 0)
        .arg(unpaidSalary, 0, 'f', 0)
        .arg(lowCount > 0 ? "red" : "green")
        .arg(lowCount)
    );
}

void DashboardForm::addNotification(const QString &message) {
    m_notifList->insertItem(0,
        QString("[%1] %2")
            .arg(QTime::currentTime().toString("hh:mm"))
            .arg(message)
    );
    // keep only last 50 items
    while (m_notifList->count() > 50) {
        delete m_notifList->item(m_notifList->count() - 1);
    }
}
