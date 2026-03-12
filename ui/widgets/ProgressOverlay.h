#pragma once
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

/**
 * @brief Полупрозрачный оверлей поверх NestCanvas во время расчёта.
 *
 * Отображает прогресс-бар, счётчик размещённых деталей и
 * кнопку «Отмена».
 */
class ProgressOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit ProgressOverlay(QWidget* parent = nullptr);

    /** Обновить прогресс [0..100] и текст статуса. */
    void updateProgress(int percent, const QString& statusText = {});

    /** Показать/скрыть с анимацией прозрачности (вызывает show/hide). */
    void showOverlay();
    void hideOverlay();

signals:
    void cancelRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel*       m_label   = nullptr;
    QProgressBar* m_bar     = nullptr;
    QPushButton*  m_cancelBtn = nullptr;

    void buildUi();
};
