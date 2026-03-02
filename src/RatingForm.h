#ifndef RATINGFORM_H
#define RATINGFORM_H

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

// Вкладка "Рейтинг": лидерборд текущего месяца + история по месяцам
class RatingForm : public QWidget {
    Q_OBJECT
public:
    explicit RatingForm(QWidget *parent = nullptr);

public slots:
    void refresh();     ///< Reload current month
    void refreshHistory();

private slots:
    void onRowDoubleClicked(int row, int col);
    void onHistoryRowDoubleClicked(int row, int col);
    void exportRating();

private:
    void setupUI();
    void fillTable(QTableWidget *table, int month, int year);
    void showAchievements(int employeeId, const QString &name);

    QTabWidget   *m_tabs;

    // Tab 1: Current month
    QTableWidget *m_currentTable;
    QPushButton  *m_refreshBtn;
    QPushButton  *m_exportBtn;

    // Tab 2: History
    QComboBox    *m_monthCombo;
    QComboBox    *m_yearCombo;
    QTableWidget *m_historyTable;
    QPushButton  *m_histRefreshBtn;
};

#endif // RATINGFORM_H
