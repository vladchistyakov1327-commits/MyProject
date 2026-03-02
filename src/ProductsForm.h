#ifndef PRODUCTSFORM_H
#define PRODUCTSFORM_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QSqlQueryModel>
#include <QSqlTableModel>

// Справочник изделий с автоматическим расчётом себестоимости и маржинальности.
class ProductsForm : public QWidget {
    Q_OBJECT
public:
    explicit ProductsForm(QWidget *parent = nullptr);

public slots:
    void refresh();       ///< Reload the read-only cost view
    void addProduct();
    void deleteProduct();
    void saveProducts();

private:
    void setupUI();
    void loadProducts();

    QTableView     *m_editView;
    QSqlTableModel *m_editModel;

    QTableView     *m_costView;
    QSqlQueryModel *m_costModel;

    QPushButton *m_addBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_refreshBtn;
};

#endif // PRODUCTSFORM_H
