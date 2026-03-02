#include "ProductsForm.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QColor>
#include <QBrush>
#include <QPainter>
#include <QDebug>

// Delegate that highlights margin cells based on value
class MarginDelegate : public QStyledItemDelegate {
public:
    explicit MarginDelegate(int marginCol, QObject *parent = nullptr)
        : QStyledItemDelegate(parent), m_col(marginCol) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        if (index.column() == m_col) {
            double margin = index.data().toDouble();
            if (margin < 0) {
                opt.backgroundBrush = QBrush(QColor(255, 150, 150)); // red
            } else if (margin < 10.0) {
                opt.backgroundBrush = QBrush(QColor(255, 150, 150));
            } else if (margin < 20.0) {
                opt.backgroundBrush = QBrush(QColor(255, 255, 150)); // yellow
            }
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
private:
    int m_col;
};

ProductsForm::ProductsForm(QWidget *parent)
    : QWidget(parent), m_editModel(nullptr), m_costModel(nullptr)
{
    setupUI();
    loadProducts();
}

void ProductsForm::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    // --- Editable table (product base data) ---
    layout->addWidget(new QLabel("Базовые данные изделия (редактируемые):"));
    m_editView = new QTableView(this);
    layout->addWidget(m_editView);

    QHBoxLayout *btnRow = new QHBoxLayout();
    m_addBtn     = new QPushButton("Добавить изделие", this);
    m_deleteBtn  = new QPushButton("Удалить изделие",  this);
    m_saveBtn    = new QPushButton("Сохранить",         this);
    m_refreshBtn = new QPushButton("Обновить расчёт",   this);
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_deleteBtn);
    btnRow->addWidget(m_saveBtn);
    btnRow->addWidget(m_refreshBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    // --- Read-only cost table ---
    layout->addWidget(new QLabel("Себестоимость и маржинальность (только для чтения):"));
    m_costView = new QTableView(this);
    m_costView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_costView);

    connect(m_addBtn,     &QPushButton::clicked, this, &ProductsForm::addProduct);
    connect(m_deleteBtn,  &QPushButton::clicked, this, &ProductsForm::deleteProduct);
    connect(m_saveBtn,    &QPushButton::clicked, this, &ProductsForm::saveProducts);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ProductsForm::refresh);

    setLayout(layout);
}

void ProductsForm::loadProducts() {
    QSqlDatabase db = DatabaseManager::instance().database();

    // Editable model
    m_editModel = new QSqlTableModel(this, db);
    m_editModel->setTable("products");
    m_editModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    if (!m_editModel->select()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить справочник изделий");
        qDebug() << m_editModel->lastError().text();
        return;
    }
    m_editModel->setHeaderData(0, Qt::Horizontal, "ID");
    m_editModel->setHeaderData(1, Qt::Horizontal, "Наименование");
    m_editModel->setHeaderData(2, Qt::Horizontal, "Описание");
    m_editModel->setHeaderData(3, Qt::Horizontal, "Цена заказчика, руб");
    m_editModel->setHeaderData(4, Qt::Horizontal, "Себест. производства, руб");

    m_editView->setModel(m_editModel);
    m_editView->horizontalHeader()->setStretchLastSection(true);

    // Cost model (computed)
    refresh();
}

void ProductsForm::refresh() {
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    delete m_costModel;
    m_costModel = new QSqlQueryModel(this);

    m_costModel->setQuery(
        "SELECT p.name AS 'Изделие', "
        "p.price AS 'Цена заказчика', "
        "COALESCE((SELECT SUM(n.quantity * m.price) "
        "          FROM norms n JOIN materials m ON n.material = m.name "
        "          WHERE n.product = p.name), 0) AS 'Себест. материалов', "
        "p.production_cost AS 'Себест. производства', "
        "COALESCE((SELECT SUM(n.quantity * m.price) "
        "          FROM norms n JOIN materials m ON n.material = m.name "
        "          WHERE n.product = p.name), 0) + p.production_cost AS 'Полная себест.', "
        "CASE WHEN p.price > 0 THEN "
        "  ROUND((p.price - (COALESCE((SELECT SUM(n.quantity * m.price) "
        "          FROM norms n JOIN materials m ON n.material = m.name "
        "          WHERE n.product = p.name), 0) + p.production_cost)) / p.price * 100, 2) "
        "ELSE 0 END AS 'Маржинальность, %' "
        "FROM products p",
        db
    );

    if (m_costModel->lastError().isValid()) {
        qDebug() << "Cost query error:" << m_costModel->lastError().text();
    }

    m_costView->setModel(m_costModel);
    // Apply margin highlighting on last column (index 5)
    m_costView->setItemDelegate(new MarginDelegate(5, m_costView));
    m_costView->horizontalHeader()->setStretchLastSection(true);
}

void ProductsForm::addProduct() {
    int row = m_editModel->rowCount();
    m_editModel->insertRow(row);
    m_editView->scrollToBottom();
}

void ProductsForm::deleteProduct() {
    QModelIndex idx = m_editView->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Выберите строку для удаления");
        return;
    }
    m_editModel->removeRow(idx.row());
}

void ProductsForm::saveProducts() {
    if (m_editModel->submitAll()) {
        QMessageBox::information(this, "Успех", "Изделия сохранены");
        refresh();
    } else {
        QMessageBox::critical(this, "Ошибка",
            QString("Не удалось сохранить: ") + m_editModel->lastError().text());
    }
}
