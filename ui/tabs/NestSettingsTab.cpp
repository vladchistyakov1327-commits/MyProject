#include "NestSettingsTab.h"
#include "core/nesting/PlacementStrategy.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>

NestSettingsTab::NestSettingsTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
}

void NestSettingsTab::buildUi()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget();
    auto* mainLay = new QVBoxLayout(container);
    mainLay->setSpacing(12);
    mainLay->setContentsMargins(8, 8, 8, 8);

    // ── Группа: Зазоры ────────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Зазоры", container);
        auto* form = new QFormLayout(grp);

        m_spacingSpin = new QDoubleSpinBox(grp);
        m_spacingSpin->setRange(0.0, 100.0);
        m_spacingSpin->setSuffix(" мм");
        m_spacingSpin->setSingleStep(0.5);
        m_spacingSpin->setValue(2.0);
        form->addRow("Между деталями:", m_spacingSpin);

        m_marginSpin = new QDoubleSpinBox(grp);
        m_marginSpin->setRange(0.0, 200.0);
        m_marginSpin->setSuffix(" мм");
        m_marginSpin->setSingleStep(1.0);
        m_marginSpin->setValue(5.0);
        form->addRow("Отступ от края:", m_marginSpin);

        mainLay->addWidget(grp);
    }

    // ── Группа: Стратегия ─────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Стратегия размещения", container);
        auto* form = new QFormLayout(grp);

        m_strategyCb = new QComboBox(grp);
        for (int i = 0; i <= static_cast<int>(PlacementStrategy::GRAVITY_EDGE); ++i) {
            m_strategyCb->addItem(
                strategyDescription(static_cast<PlacementStrategy>(i)), i);
        }
        form->addRow("Стратегия:", m_strategyCb);

        mainLay->addWidget(grp);
    }

    // ── Группа: Вращение ──────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Вращение", container);
        auto* form = new QFormLayout(grp);

        m_enableRotChk = new QCheckBox("Разрешить вращение", grp);
        m_enableRotChk->setChecked(true);
        form->addRow(m_enableRotChk);

        m_rotStepCb = new QComboBox(grp);
        m_rotStepCb->addItem("90° (0, 90, 180, 270)",   static_cast<int>(RotationMode::STEP_90));
        m_rotStepCb->addItem("45° (0, 45, ..., 315)",    static_cast<int>(RotationMode::STEP_45));
        m_rotStepCb->addItem("Без вращения",             static_cast<int>(RotationMode::NONE));
        form->addRow("Шаг вращения:", m_rotStepCb);

        connect(m_enableRotChk, &QCheckBox::toggled, m_rotStepCb,
                &QComboBox::setEnabled);

        mainLay->addWidget(grp);
    }

    // ── Группа: Совмещение кромок (Coedge) ───────────────────────────────
    {
        auto* grp = new QGroupBox("Совмещение кромок", container);
        auto* form = new QFormLayout(grp);

        m_coedgeChk = new QCheckBox("Включить Coedge", grp);
        m_coedgeChk->setChecked(false);
        form->addRow(m_coedgeChk);

        m_coedgeTolSpin = new QDoubleSpinBox(grp);
        m_coedgeTolSpin->setRange(0.01, 10.0);
        m_coedgeTolSpin->setSuffix(" мм");
        m_coedgeTolSpin->setSingleStep(0.1);
        m_coedgeTolSpin->setValue(0.5);
        m_coedgeTolSpin->setEnabled(false);
        form->addRow("Допуск:", m_coedgeTolSpin);

        connect(m_coedgeChk, &QCheckBox::toggled, m_coedgeTolSpin,
                &QDoubleSpinBox::setEnabled);

        mainLay->addWidget(grp);
    }

    // ── Группа: Алгоритм ──────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Алгоритм", container);
        auto* form = new QFormLayout(grp);

        m_algorithmCb = new QComboBox(grp);
        m_algorithmCb->addItem("Bottom-Left + NFP",          static_cast<int>(NestAlgorithm::BOTTOM_LEFT_NFP));
        m_algorithmCb->addItem("Генетический (прот.)",       static_cast<int>(NestAlgorithm::GENETIC));
        m_algorithmCb->addItem("Имитация отжига (прот.)",    static_cast<int>(NestAlgorithm::SIMULATED_ANNEALING));
        form->addRow("Алгоритм:", m_algorithmCb);

        m_iterSpin = new QSpinBox(grp);
        m_iterSpin->setRange(100, 100000);
        m_iterSpin->setSingleStep(100);
        m_iterSpin->setValue(1000);
        form->addRow("Макс. итераций:", m_iterSpin);

        m_timeSpin = new QSpinBox(grp);
        m_timeSpin->setRange(0, 3600);
        m_timeSpin->setSuffix(" с (0=нет)");
        m_timeSpin->setValue(0);
        form->addRow("Лимит времени:", m_timeSpin);

        mainLay->addWidget(grp);
    }

    mainLay->addStretch(1);
    scroll->setWidget(container);

    auto* outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->addWidget(scroll);

    // Сигналы изменений
    auto changed = [this] { emitChanged(); };
    connect(m_spacingSpin,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, changed);
    connect(m_marginSpin,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, changed);
    connect(m_strategyCb,    QOverload<int>::of(&QComboBox::currentIndexChanged),  this, changed);
    connect(m_enableRotChk,  &QCheckBox::toggled,                                  this, changed);
    connect(m_rotStepCb,     QOverload<int>::of(&QComboBox::currentIndexChanged),  this, changed);
    connect(m_coedgeChk,     &QCheckBox::toggled,                                  this, changed);
    connect(m_coedgeTolSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, changed);
    connect(m_algorithmCb,   QOverload<int>::of(&QComboBox::currentIndexChanged),  this, changed);
    connect(m_iterSpin,      QOverload<int>::of(&QSpinBox::valueChanged),          this, changed);
    connect(m_timeSpin,      QOverload<int>::of(&QSpinBox::valueChanged),          this, changed);
}

void NestSettingsTab::setParameters(const NestParameters& p)
{
    m_spacingSpin->setValue(p.partSpacingMm);
    m_marginSpin->setValue(p.sheetMarginMm);

    const int stIdx = m_strategyCb->findData(static_cast<int>(p.strategy));
    m_strategyCb->setCurrentIndex(stIdx >= 0 ? stIdx : 0);

    m_enableRotChk->setChecked(p.enableRotation);

    const int rotIdx = m_rotStepCb->findData(
        p.globalRotStepDeg <= 0   ? static_cast<int>(RotationMode::NONE)   :
        p.globalRotStepDeg <= 45  ? static_cast<int>(RotationMode::STEP_45) :
                                    static_cast<int>(RotationMode::STEP_90));
    m_rotStepCb->setCurrentIndex(rotIdx >= 0 ? rotIdx : 0);

    m_coedgeChk->setChecked(p.enableCoedge);
    m_coedgeTolSpin->setValue(p.coedgeTolerance);

    const int algIdx = m_algorithmCb->findData(static_cast<int>(p.algorithm));
    m_algorithmCb->setCurrentIndex(algIdx >= 0 ? algIdx : 0);

    m_iterSpin->setValue(p.maxIterations);
    m_timeSpin->setValue(p.timeLimitSec);
}

NestParameters NestSettingsTab::parameters() const
{
    NestParameters p;
    p.partSpacingMm  = m_spacingSpin->value();
    p.sheetMarginMm  = m_marginSpin->value();
    p.strategy       = static_cast<PlacementStrategy>(
        m_strategyCb->currentData().toInt());
    p.enableRotation = m_enableRotChk->isChecked();
    {
        const auto rm = static_cast<RotationMode>(
            m_rotStepCb->currentData().toInt());
        p.globalRotStepDeg = rm == RotationMode::STEP_45 ? 45.0 :
                             rm == RotationMode::NONE     ? 0.0  : 90.0;
    }
    p.enableCoedge   = m_coedgeChk->isChecked();
    p.coedgeTolerance = m_coedgeTolSpin->value();
    p.algorithm      = static_cast<NestAlgorithm>(
        m_algorithmCb->currentData().toInt());
    p.maxIterations  = m_iterSpin->value();
    p.timeLimitSec   = m_timeSpin->value();
    return p;
}

void NestSettingsTab::emitChanged()
{
    emit parametersChanged(parameters());
}
