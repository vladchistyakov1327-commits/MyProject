#include "NotificationsForm.h"
#include "DatabaseManager.h"
#include "NotificationSystem.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QColor>
#include <QBrush>
#include <QMessageBox>
#include <QDebug>

static const QStringList kTypeNames = {
    "Зарплата ⚠️", "Выплата 💰", "Материал ⚡", "Маржа 📉", "День рождения 🎂"
};

NotificationsForm::NotificationsForm(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    refresh();
}

void NotificationsForm::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_countLabel = new QLabel("Уведомлений: 0", this);
    layout->addWidget(m_countLabel);

    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({"Дата/Время", "Тип", "Приоритет", "Сообщение", "Прочитано"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setWordWrap(true);
    layout->addWidget(m_table);

    QHBoxLayout *btnRow = new QHBoxLayout();
    m_refreshBtn  = new QPushButton("Обновить",         this);
    m_markReadBtn = new QPushButton("Отметить прочитанным", this);
    m_clearBtn    = new QPushButton("Очистить журнал",   this);
    btnRow->addWidget(m_refreshBtn);
    btnRow->addWidget(m_markReadBtn);
    btnRow->addWidget(m_clearBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    connect(m_refreshBtn,  &QPushButton::clicked, this, &NotificationsForm::refresh);
    connect(m_markReadBtn, &QPushButton::clicked, this, &NotificationsForm::markSelectedRead);
    connect(m_clearBtn,    &QPushButton::clicked, this, &NotificationsForm::clearAll);

    setLayout(layout);
}

void NotificationsForm::refresh() {
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery q(db);
    if (!q.exec("SELECT id, created_at, type, priority, message, read_at "
                "FROM notifications ORDER BY created_at DESC")) {
        qDebug() << "NotificationsForm::refresh error:" << q.lastError().text();
        return;
    }

    m_table->setRowCount(0);
    int row = 0;
    while (q.next()) {
        m_table->insertRow(row);
        int    id       = q.value(0).toInt();
        QString created = q.value(1).toString();
        int    type     = q.value(2).toInt();
        int    priority = q.value(3).toInt();
        QString msg     = q.value(4).toString();
        bool   isRead   = !q.value(5).isNull();

        m_table->setItem(row, 0, new QTableWidgetItem(created));
        m_table->setItem(row, 1, new QTableWidgetItem(
            (type >= 0 && type < kTypeNames.size()) ? kTypeNames[type] : "—"));
        m_table->setItem(row, 2, new QTableWidgetItem(
            priority == 1 ? "Высокий" : priority == 2 ? "Средний" : "Низкий"));
        m_table->setItem(row, 3, new QTableWidgetItem(msg));
        m_table->setItem(row, 4, new QTableWidgetItem(isRead ? "Да" : "Нет"));

        // Store id for mark-read
        m_table->item(row, 0)->setData(Qt::UserRole, id);

        // Colour unread rows by priority
        if (!isRead) {
            QColor bg = (priority == 1) ? QColor(255, 230, 230)
                      : (priority == 2) ? QColor(255, 255, 220)
                                        : QColor(230, 245, 255);
            for (int c = 0; c < m_table->columnCount(); ++c) {
                if (m_table->item(row, c))
                    m_table->item(row, c)->setBackground(QBrush(bg));
            }
        }
        ++row;
    }

    m_table->resizeRowsToContents();
    int unread = 0;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        if (m_table->item(r, 4) && m_table->item(r, 4)->text() == "Нет") ++unread;
    }
    m_countLabel->setText(
        QString("Всего: %1 | Непрочитанных: %2").arg(m_table->rowCount()).arg(unread));
}

void NotificationsForm::appendNotification(int type, const QString &message, int priority) {
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery q(db);
    q.prepare("INSERT INTO notifications(type, message, priority, created_at) "
              "VALUES(:type, :msg, :prio, :dt)");
    q.bindValue(":type",  type);
    q.bindValue(":msg",   message);
    q.bindValue(":prio",  priority);
    q.bindValue(":dt",    QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qDebug() << "appendNotification error:" << q.lastError().text();
        return;
    }
    refresh();
}

void NotificationsForm::markSelectedRead() {
    QList<QTableWidgetItem*> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите строку");
        return;
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE notifications SET read_at = :dt WHERE id = :id");

    QSet<int> rows;
    for (auto *item : selected) rows.insert(item->row());

    for (int r : rows) {
        auto *cell = m_table->item(r, 0);
        if (!cell) continue;
        int id = cell->data(Qt::UserRole).toInt();
        q.bindValue(":dt",  QDateTime::currentDateTime().toString(Qt::ISODate));
        q.bindValue(":id",  id);
        q.exec();
    }
    refresh();
}

void NotificationsForm::clearAll() {
    if (QMessageBox::question(this, "Очистка",
            "Удалить все уведомления из журнала?",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.exec("DELETE FROM notifications");
    refresh();
}
