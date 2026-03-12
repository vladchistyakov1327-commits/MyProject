#pragma once
#include <QWidget>
#include <QListWidget>
#include "core/nesting/NestJob.h"
#include "core/nesting/NestResult.h"

/**
 * @brief Вкладка управления листами-заготовками.
 *
 * Отображает карточки листов с UtilizationBar.
 * Позволяет задать прямоугольный лист или загрузить контур из DXF.
 */
class SheetsTab : public QWidget
{
    Q_OBJECT
public:
    explicit SheetsTab(QWidget* parent = nullptr);
    ~SheetsTab();

    void setSheetDefinition(const SheetDefinition& def);
    SheetDefinition sheetDefinition() const;

    /** Обновить утилизацию (вызывается после расчёта). */
    void updateResults(const NestResult& result);

signals:
    void sheetChanged();

private slots:
    void onTypeToggled();
    void onLoadSheetDxf();

private:
    void buildUi();

    class Impl;
    Impl* d = nullptr;
};
