#pragma once
#include "NestJob.h"
#include "NestResult.h"
#include <vector>

/**
 * Менеджер листов.
 * Отслеживает, сколько листов использовано и создаёт новые при необходимости.
 */
class SheetManager
{
public:
    explicit SheetManager(const SheetDefinition& sheetDef);

    /// Получить контур листа (после отступа margin).
    std::vector<QPointF> sheetContourWithMargin(double marginMm) const;

    /// Площадь листа (без отступа).
    double sheetTotalAreaMm2() const;

    /// Площадь листа (с отступом).
    double sheetUsableAreaMm2(double marginMm) const;

    /// Проверить, доступен ли новый лист.
    bool canAddSheet(int currentSheetCount) const;

    /// Создать новую SheetResult для листа с заданным индексом.
    SheetResult createSheetResult(int index) const;

    const SheetDefinition& definition() const { return m_def; }

private:
    SheetDefinition m_def;
    mutable std::vector<QPointF> m_outerContour; // Кэш контура
    void buildRectContour();
};
