#ifndef DASHBOARDFORM_H
#define DASHBOARDFORM_H

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QTimer>

// Главный дашборд: доска почёта (топ-3 сотрудников) + сводные метрики
class DashboardForm : public QWidget {
    Q_OBJECT
public:
    explicit DashboardForm(QWidget *parent = nullptr);

public slots:
    void refresh();         ///< Reload all data from DB
    void addNotification(const QString &message); ///< Show a notification in the feed

private:
    void setupUI();
    void updateHonorBoard();
    void updateSummary();

    QLabel      *m_honorTitle;
    QLabel      *m_place1;
    QLabel      *m_place2;
    QLabel      *m_place3;

    QLabel      *m_summaryLabel;
    QListWidget *m_notifList;   ///< Recent notifications feed

    QTimer      *m_refreshTimer;
};

#endif // DASHBOARDFORM_H
