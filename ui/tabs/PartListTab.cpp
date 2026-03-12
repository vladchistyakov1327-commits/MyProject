#include "PartListTab.h"
#include "core/geometry/DxfImporter.h"
#include "core/logging/AppLogger.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QFileDialog>
#include <QFileInfo>
#include <QSpinBox>
#include <QMessageBox>
#include <QLabel>

static const int COL_NAME   = 0;
static const int COL_AREA   = 1;
static const int COL_QTY    = 2;
static const int COL_FLIP   = 3;
static const int COL_ROT    = 4;

PartListTab::PartListTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
}

void PartListTab::setNestJob(NestJob* job)
{
    m_job = job;
    if (m_job) {
        m_parts = m_job->parts;
        refreshTable();
    }
}

std::vector<PartEntry> PartListTab::partEntries() const
{
    return m_parts;
}

void PartListTab::buildUi()
{
    // Заголовок + кнопки
    auto* btnBar  = new QWidget(this);
    auto* btnLay  = new QHBoxLayout(btnBar);
    btnLay->setContentsMargins(4, 4, 4, 4);
    btnLay->setSpacing(6);

    auto* addBtn = new QPushButton("+ Добавить DXF", this);
    auto* delBtn = new QPushButton("Удалить", this);
    delBtn->setProperty("role", "danger");
    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* cntLabel = new QLabel("Деталей: 0", this);
    cntLabel->setObjectName("cntLabel");

    btnLay->addWidget(addBtn);
    btnLay->addWidget(delBtn);
    btnLay->addWidget(spacer);
    btnLay->addWidget(cntLabel);

    // Таблица
    m_model = new QStandardItemModel(0, 5, this);
    m_model->setHorizontalHeaderLabels({"Имя", "Площадь, мм²", "Кол-во", "Переворот", "Вращение"});

    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setAlternatingRowColors(true);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setEditTriggers(QAbstractItemView::AllEditTriggers);
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->horizontalHeader()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    m_view->setColumnWidth(COL_AREA,  90);
    m_view->setColumnWidth(COL_QTY,   70);
    m_view->setColumnWidth(COL_FLIP,  80);
    m_view->verticalHeader()->hide();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(btnBar);
    layout->addWidget(m_view);

    // Connections
    connect(addBtn, &QPushButton::clicked, this, &PartListTab::onAddDxf);
    connect(delBtn, &QPushButton::clicked, this, &PartListTab::onRemoveSelected);

    connect(m_model, &QStandardItemModel::itemChanged, [this, cntLabel](QStandardItem* item) {
        if (item->column() == COL_QTY) {
            bool ok;
            const int v = item->text().toInt(&ok);
            if (ok && item->row() < (int)m_parts.size()) {
                m_parts[item->row()].quantity = qBound(1, v, 9999);
            }
        }
        if (item->column() == COL_FLIP) {
            if (item->row() < (int)m_parts.size())
                m_parts[item->row()].allowFlip = (item->checkState() == Qt::Checked);
        }
        // Пересчитать счётчик
        int total = 0;
        for(auto& p : m_parts) total += p.quantity;
        cntLabel->setText(QString("Деталей: %1 (%2 тип.)").arg(total).arg(m_parts.size()));
        emit partsChanged();
    });
}

void PartListTab::onAddDxf()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, "Добавить детали из DXF", {},
        "DXF файлы (*.dxf);;Все файлы (*)");

    for (const QString& path : files) {
        DxfImporter importer;
        const auto geoms = importer.import(path);
        if (geoms.isEmpty()) {
            LOG_WARN(LogChannel::IMPORT,
                     QString("Пустой DXF: %1").arg(path));
            continue;
        }
        for (const auto& g : geoms) {
            PartEntry pe;
            pe.geometry     = g;
            pe.name         = g.sourceName.isEmpty()
                              ? QFileInfo(path).baseName() : g.sourceName;
            pe.quantity     = 1;
            pe.allowFlip    = false;
            pe.rotationMode = RotationMode::STEP_90;
            addPartEntry(pe);
        }
    }
    emit partsChanged();
}

void PartListTab::onRemoveSelected()
{
    const QModelIndexList sel = m_view->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;

    if (QMessageBox::question(this, "Удалить",
        QString("Удалить %1 деталей?").arg(sel.size()),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QList<int> rows;
    for (auto& idx : sel) rows.append(idx.row());
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for (int row : rows) {
        m_model->removeRow(row);
        if (row < (int)m_parts.size())
            m_parts.erase(m_parts.begin() + row);
    }
    emit partsChanged();
}

void PartListTab::addPartEntry(const PartEntry& entry)
{
    m_parts.push_back(entry);

    const int row = m_model->rowCount();
    m_model->insertRow(row);

    auto nameItem = new QStandardItem(entry.name);
    nameItem->setEditable(true);

    auto areaItem = new QStandardItem(
        QString::number(entry.geometry.areaMm2, 'f', 1));
    areaItem->setEditable(false);

    auto qtyItem = new QStandardItem(QString::number(entry.quantity));
    qtyItem->setEditable(true);

    auto flipItem = new QStandardItem();
    flipItem->setCheckable(true);
    flipItem->setCheckState(entry.allowFlip ? Qt::Checked : Qt::Unchecked);

    auto rotItem = new QStandardItem(
        entry.rotationMode == RotationMode::STEP_90 ? "90°" :
        entry.rotationMode == RotationMode::STEP_45 ? "45°" :
        entry.rotationMode == RotationMode::NONE    ? "Нет" : "Своё");
    rotItem->setEditable(false);

    m_model->setItem(row, COL_NAME, nameItem);
    m_model->setItem(row, COL_AREA, areaItem);
    m_model->setItem(row, COL_QTY,  qtyItem);
    m_model->setItem(row, COL_FLIP, flipItem);
    m_model->setItem(row, COL_ROT,  rotItem);
}

void PartListTab::refreshTable()
{
    m_model->setRowCount(0);
    for (const auto& p : m_parts)
        addPartEntry(p);
}

void PartListTab::onQuantityChanged(int row, int qty)
{
    if (row < (int)m_parts.size())
        m_parts[row].quantity = qty;
    emit partsChanged();
}

void PartListTab::onRotationModeChanged(int row, int mode)
{
    if (row < (int)m_parts.size())
        m_parts[row].rotationMode = static_cast<RotationMode>(mode);
    emit partsChanged();
}
