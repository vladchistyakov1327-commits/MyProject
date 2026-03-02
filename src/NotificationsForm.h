#ifndef NOTIFICATIONSFORM_H
#define NOTIFICATIONSFORM_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>

// Журнал всех уведомлений из таблицы notifications (только чтение + отметка прочтения)
class NotificationsForm : public QWidget {
    Q_OBJECT
public:
    explicit NotificationsForm(QWidget *parent = nullptr);

    /// Append a new notification row (called from MainWindow when signal fires)
    void appendNotification(int type, const QString &message, int priority);

public slots:
    void refresh();
    void markSelectedRead();
    void clearAll();

private:
    void setupUI();

    QTableWidget *m_table;
    QPushButton  *m_refreshBtn;
    QPushButton  *m_markReadBtn;
    QPushButton  *m_clearBtn;
    QLabel       *m_countLabel;
};

#endif // NOTIFICATIONSFORM_H
