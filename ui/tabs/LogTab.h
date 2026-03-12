#pragma once
#include <QWidget>

/**
 * @brief Вкладка "Лог" — обёртка над LogWidget.
 *
 * Добавляет строку статуса с количеством записей по уровням.
 */
class LogWidget;
class QLabel;

class LogTab : public QWidget
{
    Q_OBJECT
public:
    explicit LogTab(QWidget* parent = nullptr);

    LogWidget* logWidget() const;

private:
    void buildUi();

    LogWidget* m_logWidget  = nullptr;
    QLabel*    m_statusBar  = nullptr;

    void updateStatusBar();
};
