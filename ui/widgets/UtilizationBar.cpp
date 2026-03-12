#include "UtilizationBar.h"
#include "AppStyle.h"
#include <QPainter>
#include <QPaintEvent>
#include <algorithm>

UtilizationBar::UtilizationBar(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(18);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void UtilizationBar::setValue(double pct)
{
    m_value = std::max(0.0, std::min(pct, 100.0));
    update();
}

QSize UtilizationBar::sizeHint()        const { return {200, 22}; }
QSize UtilizationBar::minimumSizeHint() const { return {60, 18}; }

void UtilizationBar::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF bg(0, 0, width(), height());

    // Background
    p.setBrush(QColor(AppPalette::BG_RAISED));
    p.setPen(QPen(QColor(AppPalette::BORDER_MID), 1));
    p.drawRoundedRect(bg.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);

    // Fill
    const double fillWidth = (m_value / 100.0) * (width() - 2);
    if (fillWidth > 0.0) {
        QRectF fill(1, 1, fillWidth, height() - 2);
        QColor fillColor;
        if      (m_value >= 80.0) fillColor = QColor(AppPalette::GREEN_MID);
        else if (m_value >= 50.0) fillColor = QColor(AppPalette::BLUE_MID);
        else                      fillColor = QColor(AppPalette::AMBER_TEXT);

        p.setPen(Qt::NoPen);
        p.setBrush(QLinearGradient(fill.topLeft(), fill.topRight()));
        QLinearGradient grad(fill.topLeft(), fill.topRight());
        grad.setColorAt(0.0, fillColor);
        grad.setColorAt(1.0, fillColor.darker(80));
        p.setBrush(grad);
        p.drawRoundedRect(fill, 2, 2);
    }

    // Text
    p.setPen(QColor(AppPalette::TEXT_BRIGHT));
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.drawText(bg, Qt::AlignCenter,
               QString("%1%").arg(QString::number(m_value, 'f', 1)));
}
