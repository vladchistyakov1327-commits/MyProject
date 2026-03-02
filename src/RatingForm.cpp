#include "RatingForm.h"
#include "EmployeeRating.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QDate>
#include <QColor>
#include <QBrush>

static const QStringList kHeaders = {
    "Место", "ФИО", "Должность", "Выработка, руб",
    "Кол-во операций", "Ср. чек, руб", "% плана", "Премия, руб"
};

RatingForm::RatingForm(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    refresh();
}

void RatingForm::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);

    // ---- Tab 1: Current month ----
    QWidget *currentTab = new QWidget();
    QVBoxLayout *curLay = new QVBoxLayout(currentTab);

    m_currentTable = new QTableWidget(0, kHeaders.size(), currentTab);
    m_currentTable->setHorizontalHeaderLabels(kHeaders);
    m_currentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_currentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_currentTable->horizontalHeader()->setStretchLastSection(true);
    m_currentTable->verticalHeader()->setVisible(false);
    connect(m_currentTable, &QTableWidget::cellDoubleClicked,
            this, &RatingForm::onRowDoubleClicked);
    curLay->addWidget(m_currentTable);

    QHBoxLayout *curBtns = new QHBoxLayout();
    m_refreshBtn = new QPushButton("Обновить", currentTab);
    m_exportBtn  = new QPushButton("Копировать рейтинг", currentTab);
    curBtns->addWidget(m_refreshBtn);
    curBtns->addWidget(m_exportBtn);
    curBtns->addStretch();
    curLay->addLayout(curBtns);
    connect(m_refreshBtn, &QPushButton::clicked, this, &RatingForm::refresh);
    connect(m_exportBtn,  &QPushButton::clicked, this, &RatingForm::exportRating);

    m_tabs->addTab(currentTab, "Текущий месяц");

    // ---- Tab 2: History ----
    QWidget *histTab = new QWidget();
    QVBoxLayout *histLay = new QVBoxLayout(histTab);

    QHBoxLayout *selRow = new QHBoxLayout();
    selRow->addWidget(new QLabel("Месяц:", histTab));
    m_monthCombo = new QComboBox(histTab);
    const QStringList months = {
        "Январь","Февраль","Март","Апрель","Май","Июнь",
        "Июль","Август","Сентябрь","Октябрь","Ноябрь","Декабрь"
    };
    m_monthCombo->addItems(months);
    m_monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);

    selRow->addWidget(m_monthCombo);
    selRow->addWidget(new QLabel("Год:", histTab));
    m_yearCombo = new QComboBox(histTab);
    int curYear = QDate::currentDate().year();
    for (int y = curYear - 3; y <= curYear; ++y)
        m_yearCombo->addItem(QString::number(y), y);
    m_yearCombo->setCurrentIndex(m_yearCombo->count() - 1);

    m_histRefreshBtn = new QPushButton("Показать", histTab);
    selRow->addWidget(m_yearCombo);
    selRow->addWidget(m_histRefreshBtn);
    selRow->addStretch();
    histLay->addLayout(selRow);

    m_historyTable = new QTableWidget(0, kHeaders.size(), histTab);
    m_historyTable->setHorizontalHeaderLabels(kHeaders);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->verticalHeader()->setVisible(false);
    connect(m_historyTable, &QTableWidget::cellDoubleClicked,
            this, &RatingForm::onHistoryRowDoubleClicked);
    histLay->addWidget(m_historyTable);

    connect(m_histRefreshBtn, &QPushButton::clicked, this, &RatingForm::refreshHistory);

    m_tabs->addTab(histTab, "История рейтингов");

    layout->addWidget(m_tabs);
    setLayout(layout);
}

void RatingForm::fillTable(QTableWidget *table, int month, int year) {
    EmployeeRating rating;
    auto entries = (month == QDate::currentDate().month() && year == QDate::currentDate().year())
        ? rating.calculateCurrentMonth()
        : rating.historyForMonth(month, year);

    table->setRowCount(entries.size());

    static const char *medals[] = {"🥇", "🥈", "🥉"};

    for (int i = 0; i < entries.size(); ++i) {
        const auto &e = entries[i];

        QString place = (i < 3) ? QString(medals[i]) + QString(" %1").arg(i + 1)
                                 : QString::number(i + 1);

        table->setItem(i, 0, new QTableWidgetItem(place));
        table->setItem(i, 1, new QTableWidgetItem(e.name));
        table->setItem(i, 2, new QTableWidgetItem(e.position));
        table->setItem(i, 3, new QTableWidgetItem(
            QString::number(e.earnings, 'f', 2)));
        table->setItem(i, 4, new QTableWidgetItem(
            QString::number(e.operationCount)));
        table->setItem(i, 5, new QTableWidgetItem(
            QString::number(e.avgOperation, 'f', 2)));

        QString planStr = (e.monthlyPlan > 0)
            ? QString("%1 %").arg(e.planPercent, 0, 'f', 1)
            : "—";
        auto *planItem = new QTableWidgetItem(planStr);
        if (e.planPercent >= 100.0)
            planItem->setBackground(QBrush(QColor(150, 255, 150)));
        else if (e.planPercent < 50.0 && e.monthlyPlan > 0)
            planItem->setBackground(QBrush(QColor(255, 150, 150)));
        table->setItem(i, 6, planItem);

        table->setItem(i, 7, new QTableWidgetItem(
            e.bonus > 0 ? QString::number(e.bonus, 'f', 2) : "—"));

        // Store employee id in user data of column 1
        table->item(i, 1)->setData(Qt::UserRole, e.employeeId);
    }
    table->resizeColumnsToContents();
}

void RatingForm::refresh() {
    QDate today = QDate::currentDate();
    fillTable(m_currentTable, today.month(), today.year());
}

void RatingForm::refreshHistory() {
    int month = m_monthCombo->currentIndex() + 1;
    int year  = m_yearCombo->currentData().toInt();
    fillTable(m_historyTable, month, year);
}

void RatingForm::onRowDoubleClicked(int row, int /*col*/) {
    if (row < 0 || row >= m_currentTable->rowCount()) return;
    auto *item = m_currentTable->item(row, 1);
    if (!item) return;
    int empId = item->data(Qt::UserRole).toInt();
    showAchievements(empId, item->text());
}

void RatingForm::onHistoryRowDoubleClicked(int row, int /*col*/) {
    if (row < 0 || row >= m_historyTable->rowCount()) return;
    auto *item = m_historyTable->item(row, 1);
    if (!item) return;
    int empId = item->data(Qt::UserRole).toInt();
    showAchievements(empId, item->text());
}

void RatingForm::showAchievements(int employeeId, const QString &name) {
    EmployeeRating rating;
    QStringList badges = rating.badgesFor(employeeId);

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Достижения: %1").arg(name));
    dlg.resize(350, 300);

    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    auto *list = new QListWidget(&dlg);

    if (badges.isEmpty()) {
        list->addItem("Достижений пока нет");
    } else {
        for (const QString &b : badges) {
            list->addItem(b);
        }
    }

    lay->addWidget(list);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    lay->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    dlg.exec();
}

void RatingForm::exportRating() {
    QDate today = QDate::currentDate();
    EmployeeRating rating;
    auto entries = rating.calculateCurrentMonth();

    QString text = QString("📊 РЕЙТИНГ: %1 %2\n\n")
        .arg(today.toString("MMMM")).arg(today.year());

    static const char *medals[] = {"🥇", "🥈", "🥉"};
    for (int i = 0; i < entries.size(); ++i) {
        const auto &e = entries[i];
        QString medal = (i < 3) ? QString(medals[i]) : QString("   ");
        text += QString("%1 %2. %3 — %4 руб.\n")
            .arg(medal).arg(i + 1).arg(e.name)
            .arg(e.earnings, 0, 'f', 0);
    }
    text += "\nВсем спасибо за работу! 🚀";

    QApplication::clipboard()->setText(text);
    QMessageBox::information(this, "Рейтинг скопирован",
        "Текст рейтинга скопирован в буфер обмена.\nВставьте его в Telegram или Email.");
}
