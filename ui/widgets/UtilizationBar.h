#pragma once
#include <QWidget>

/**
 * Виджет прогресс-бара утилизации листа.
 * Рисует заполненный прямоугольник и процент.
 */
class UtilizationBar : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue)
public:
    explicit UtilizationBar(QWidget* parent = nullptr);

    double value() const { return m_value; }
    void   setValue(double pct);  // 0.0 .. 100.0

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    double m_value = 0.0;
};
