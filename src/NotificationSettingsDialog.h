#ifndef NOTIFICATIONSETTINGSDIALOG_H
#define NOTIFICATIONSETTINGSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>

// Диалог настройки системы уведомлений
class NotificationSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit NotificationSettingsDialog(QWidget *parent = nullptr);

    // Getters for configured values
    bool salaryMismatchEnabled() const;
    bool payrollReminderEnabled() const;
    bool lowMaterialEnabled() const;
    bool negativeMarginEnabled() const;
    bool birthdayEnabled() const;
    bool showOnStartup() const;

    double salaryThreshold() const;
    int    payrollDay() const;

private:
    QCheckBox     *m_salaryMismatchCb;
    QCheckBox     *m_payrollReminderCb;
    QCheckBox     *m_lowMaterialCb;
    QCheckBox     *m_negativeMarginCb;
    QCheckBox     *m_birthdayCb;
    QCheckBox     *m_startupCb;

    QDoubleSpinBox *m_thresholdSpin;
    QSpinBox       *m_payrollDaySpin;
};

#endif // NOTIFICATIONSETTINGSDIALOG_H
