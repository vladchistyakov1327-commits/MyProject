#ifndef NORMSFORM_H
#define NORMSFORM_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>

// Нормы расхода материалов на единицу изделия.
// Связывает таблицу norms с products и materials через выпадающие списки.
class NormsForm : public QWidget {
    Q_OBJECT
public:
    explicit NormsForm(QWidget *parent = nullptr);

private slots:
    void addNorm();
    void deleteNorm();
    void saveNorms();

private:
    void setupUI();
    void loadNorms();

    QTableView              *m_tableView;
    QSqlRelationalTableModel *m_model;
    QPushButton *m_addBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_saveBtn;
};

#endif // NORMSFORM_H
