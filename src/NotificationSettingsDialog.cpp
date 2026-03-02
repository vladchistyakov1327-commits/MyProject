#include "NotificationSettingsDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>

NotificationSettingsDialog::NotificationSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Настройки уведомлений");
    setMinimumWidth(380);

    QVBoxLayout *mainLay = new QVBoxLayout(this);

    // ---- Enabled types ----
    QGroupBox *typesBox = new QGroupBox("Типы уведомлений", this);
    QVBoxLayout *typesLay = new QVBoxLayout(typesBox);

    m_salaryMismatchCb  = new QCheckBox("⚠️ Несоответствия в зарплате",  typesBox);
    m_payrollReminderCb = new QCheckBox("💰 Напоминание о выплате зарплаты", typesBox);
    m_lowMaterialCb     = new QCheckBox("⚡ Критический остаток материалов",  typesBox);
    m_negativeMarginCb  = new QCheckBox("📉 Отрицательная маржинальность",    typesBox);
    m_birthdayCb        = new QCheckBox("🎂 Дни рождения сотрудников",         typesBox);

    m_salaryMismatchCb->setChecked(true);
    m_payrollReminderCb->setChecked(true);
    m_lowMaterialCb->setChecked(true);
    m_negativeMarginCb->setChecked(true);
    m_birthdayCb->setChecked(true);

    typesLay->addWidget(m_salaryMismatchCb);
    typesLay->addWidget(m_payrollReminderCb);
    typesLay->addWidget(m_lowMaterialCb);
    typesLay->addWidget(m_negativeMarginCb);
    typesLay->addWidget(m_birthdayCb);
    mainLay->addWidget(typesBox);

    // ---- Parameters ----
    QGroupBox *paramBox = new QGroupBox("Параметры", this);
    QFormLayout *formLay = new QFormLayout(paramBox);

    m_thresholdSpin = new QDoubleSpinBox(paramBox);
    m_thresholdSpin->setRange(0, 1000000);
    m_thresholdSpin->setValue(500.0);
    m_thresholdSpin->setSuffix(" руб.");
    m_thresholdSpin->setDecimals(0);
    formLay->addRow("Порог разницы в зарплате:", m_thresholdSpin);

    m_payrollDaySpin = new QSpinBox(paramBox);
    m_payrollDaySpin->setRange(1, 31);
    m_payrollDaySpin->setValue(20);
    formLay->addRow("Число выплаты зарплаты:", m_payrollDaySpin);

    mainLay->addWidget(paramBox);

    // ---- Startup ----
    m_startupCb = new QCheckBox("Показывать уведомления при запуске программы", this);
    m_startupCb->setChecked(true);
    mainLay->addWidget(m_startupCb);

    // ---- Buttons ----
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLay->addWidget(btns);

    setLayout(mainLay);
}

bool NotificationSettingsDialog::salaryMismatchEnabled()  const { return m_salaryMismatchCb->isChecked(); }
bool NotificationSettingsDialog::payrollReminderEnabled() const { return m_payrollReminderCb->isChecked(); }
bool NotificationSettingsDialog::lowMaterialEnabled()     const { return m_lowMaterialCb->isChecked(); }
bool NotificationSettingsDialog::negativeMarginEnabled()  const { return m_negativeMarginCb->isChecked(); }
bool NotificationSettingsDialog::birthdayEnabled()        const { return m_birthdayCb->isChecked(); }
bool NotificationSettingsDialog::showOnStartup()          const { return m_startupCb->isChecked(); }
double NotificationSettingsDialog::salaryThreshold()      const { return m_thresholdSpin->value(); }
int    NotificationSettingsDialog::payrollDay()           const { return m_payrollDaySpin->value(); }
