#pragma once
#include <QWidget>
#include "core/nesting/NestJob.h"

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QGroupBox;

/**
 * @brief Вкладка параметров раскладки: зазоры, вращение, coedge, алгоритм.
 */
class NestSettingsTab : public QWidget
{
    Q_OBJECT
public:
    explicit NestSettingsTab(QWidget* parent = nullptr);

    /** Заполнить виджеты из существующих параметров. */
    void setParameters(const NestParameters& params);

    /** Получить текущие параметры из виджетов. */
    NestParameters parameters() const;

signals:
    void parametersChanged(const NestParameters& params);

private:
    void buildUi();
    void emitChanged();

    // Зазоры
    QDoubleSpinBox* m_spacingSpin  = nullptr;
    QDoubleSpinBox* m_marginSpin   = nullptr;

    // Стратегия
    QComboBox*      m_strategyCb   = nullptr;

    // Вращение
    QCheckBox*      m_enableRotChk = nullptr;
    QComboBox*      m_rotStepCb    = nullptr;

    // Coedge
    QCheckBox*      m_coedgeChk    = nullptr;
    QDoubleSpinBox* m_coedgeTolSpin = nullptr;

    // Алгоритм
    QComboBox*      m_algorithmCb  = nullptr;
    QSpinBox*       m_iterSpin     = nullptr;
    QSpinBox*       m_timeSpin     = nullptr;
};
