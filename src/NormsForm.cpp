#include "NormsForm.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlRelationalDelegate>
#include <QDebug>

NormsForm::NormsForm(QWidget *parent)
    : QWidget(parent), m_model(nullptr)
{
    setupUI();
    loadNorms();
}

void NormsForm::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_tableView = new QTableView(this);
    layout->addWidget(m_tableView);

    QHBoxLayout *btnRow = new QHBoxLayout();
    m_addBtn    = new QPushButton("Добавить норму",  this);
    m_deleteBtn = new QPushButton("Удалить норму",   this);
    m_saveBtn   = new QPushButton("Сохранить",        this);
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_deleteBtn);
    btnRow->addWidget(m_saveBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    connect(m_addBtn,    &QPushButton::clicked, this, &NormsForm::addNorm);
    connect(m_deleteBtn, &QPushButton::clicked, this, &NormsForm::deleteNorm);
    connect(m_saveBtn,   &QPushButton::clicked, this, &NormsForm::saveNorms);

    setLayout(layout);
}

void NormsForm::loadNorms() {
    QSqlDatabase db = DatabaseManager::instance().database();

    // norms table: id | product | material | quantity
    // We show product and material as plain text (no FK in SQLite schema),
    // so use a simple QSqlTableModel. Dropdowns via combobox delegate would
    // require QSqlRelationalTableModel with FK columns — keeping plain for now.
    m_model = new QSqlRelationalTableModel(this, db);
    m_model->setTable("norms");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    if (!m_model->select()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить нормы расхода");
        qDebug() << m_model->lastError().text();
        return;
    }

    m_model->setHeaderData(0, Qt::Horizontal, "ID");
    m_model->setHeaderData(1, Qt::Horizontal, "Изделие");
    m_model->setHeaderData(2, Qt::Horizontal, "Материал");
    m_model->setHeaderData(3, Qt::Horizontal, "Норма на 1 ед.");

    m_tableView->setModel(m_model);
    m_tableView->setItemDelegate(new QSqlRelationalDelegate(m_tableView));
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setColumnHidden(0, true); // hide ID
}

void NormsForm::addNorm() {
    int row = m_model->rowCount();
    m_model->insertRow(row);
    m_tableView->scrollToBottom();
}

void NormsForm::deleteNorm() {
    QModelIndex idx = m_tableView->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Выберите строку для удаления");
        return;
    }
    m_model->removeRow(idx.row());
}

void NormsForm::saveNorms() {
    if (m_model->submitAll()) {
        QMessageBox::information(this, "Успех", "Нормы расхода сохранены");
    } else {
        QMessageBox::critical(this, "Ошибка",
            QString("Не удалось сохранить: ") + m_model->lastError().text());
    }
}
