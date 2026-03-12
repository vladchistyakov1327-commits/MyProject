#pragma once
#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include "core/nesting/NestJob.h"

class PartListTab : public QWidget
{
    Q_OBJECT
public:
    explicit PartListTab(QWidget* parent = nullptr);

    void setNestJob(NestJob* job);

    /** Возвращает актуальный список деталей с учётом количества. */
    std::vector<PartEntry> partEntries() const;

signals:
    void partsChanged();

private slots:
    void onAddDxf();
    void onRemoveSelected();
    void onQuantityChanged(int row, int qty);
    void onRotationModeChanged(int row, int mode);

private:
    void buildUi();
    void refreshTable();
    void addPartEntry(const PartEntry& entry);

    QTableView*         m_view  = nullptr;
    QStandardItemModel* m_model = nullptr;
    NestJob*            m_job   = nullptr;

    std::vector<PartEntry> m_parts;
};
