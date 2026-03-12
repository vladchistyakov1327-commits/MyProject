#include "ProgressOverlay.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QPainter>
#include <QResizeEvent>

ProgressOverlay::ProgressOverlay(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setWindowFlags(Qt::Widget);
    buildUi();
    hide();
}

void ProgressOverlay::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);

    m_label = new QLabel("Выполняется расчёт…", this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;")
                           .arg(AppPalette::TEXT_BRIGHT));

    m_bar = new QProgressBar(this);
    m_bar->setRange(0, 100);
    m_bar->setValue(0);
    m_bar->setFixedWidth(320);
    m_bar->setFixedHeight(18);

    m_cancelBtn = new QPushButton("Отмена", this);
    m_cancelBtn->setFixedWidth(120);
    m_cancelBtn->setProperty("role", "danger");

    layout->addStretch(1);
    layout->addWidget(m_label, 0, Qt::AlignCenter);
    layout->addWidget(m_bar,   0, Qt::AlignCenter);
    layout->addWidget(m_cancelBtn, 0, Qt::AlignCenter);
    layout->addStretch(1);

    connect(m_cancelBtn, &QPushButton::clicked,
            this, &ProgressOverlay::cancelRequested);
}

void ProgressOverlay::updateProgress(int percent, const QString& statusText)
{
    m_bar->setValue(qBound(0, percent, 100));
    if (!statusText.isEmpty())
        m_label->setText(statusText);
}

void ProgressOverlay::showOverlay()
{
    // Растянуть на весь родительский виджет
    if (parentWidget())
        setGeometry(0, 0, parentWidget()->width(), parentWidget()->height());
    m_bar->setValue(0);
    m_label->setText("Выполняется расчёт…");
    show();
    raise();
}

void ProgressOverlay::hideOverlay()
{
    hide();
}

void ProgressOverlay::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    // Полупрозрачный тёмный фон
    QColor bg(AppPalette::BG_VOID);
    bg.setAlpha(200);
    p.fillRect(rect(), bg);

    // Центральная карточка
    const int w = 360, h = 160;
    const QRect card(width() / 2 - w / 2, height() / 2 - h / 2, w, h);
    QColor cardBg(AppPalette::BG_PANEL);
    cardBg.setAlpha(240);
    p.setBrush(cardBg);
    p.setPen(QColor(AppPalette::BORDER_SUBTLE));
    p.drawRoundedRect(card, 8, 8);
}

void ProgressOverlay::resizeEvent(QResizeEvent*)
{
    // Обновить позицию при изменении размера родителя
}
