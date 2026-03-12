#include "SheetManager.h"
#include "core/geometry/GeomUtils.h"
#include <cmath>

SheetManager::SheetManager(const SheetDefinition& sheetDef)
    : m_def(sheetDef)
{
    if (sheetDef.type == SheetDefinition::Type::RECTANGLE) {
        buildRectContour();
    } else if (!sheetDef.customShape.outerContour.isEmpty()) {
        m_outerContour = sheetDef.customShape.outerContour.vertices;
    }
}

void SheetManager::buildRectContour()
{
    const double w = m_def.widthMm;
    const double h = m_def.heightMm;
    m_outerContour = {
        {0.0, 0.0},
        {w,   0.0},
        {w,   h  },
        {0.0, h  },
        {0.0, 0.0}  // замкнуть
    };
}

std::vector<QPointF> SheetManager::sheetContourWithMargin(double marginMm) const
{
    if (marginMm <= 0.0) return m_outerContour;
    return GeomUtils::deflateSheet(m_outerContour, marginMm);
}

double SheetManager::sheetTotalAreaMm2() const
{
    return GeomUtils::area(m_outerContour);
}

double SheetManager::sheetUsableAreaMm2(double marginMm) const
{
    auto c = sheetContourWithMargin(marginMm);
    return GeomUtils::area(c);
}

bool SheetManager::canAddSheet(int currentSheetCount) const
{
    if (m_def.quantity == 0) return true;          // Бесконечно
    return currentSheetCount < m_def.quantity;
}

SheetResult SheetManager::createSheetResult(int index) const
{
    SheetResult sr;
    sr.sheetIndex     = index;
    sr.totalAreaMm2   = sheetTotalAreaMm2();
    sr.sheetBounds    = QRectF(0, 0, m_def.widthMm, m_def.heightMm);

    if (m_def.type == SheetDefinition::Type::CUSTOM_DXF &&
        !m_def.customShape.outerContour.isEmpty()) {
        sr.sheetBounds = m_def.customShape.boundingBox;
    }

    return sr;
}
