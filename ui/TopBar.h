#pragma once
#include <QWidget>

/**
 * @brief Верхняя панель приложения: логотип + кнопки действий.
 */
class TopBar : public QWidget
{
    Q_OBJECT
public:
    explicit TopBar(QWidget* parent = nullptr);

    void setRunning(bool running);

signals:
    void importDxfRequested();
    void sheetSetupRequested();
    void startNestRequested();
    void cancelNestRequested();
    void exportRequested();

private:
    void buildUi();

    class QPushButton* m_runBtn    = nullptr;
    class QPushButton* m_cancelBtn = nullptr;
    class QPushButton* m_exportBtn = nullptr;
};
