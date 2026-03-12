#include "TopBar.h"
#include "AppStyle.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

TopBar::TopBar(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(48);
    setObjectName("topBar");
    buildUi();
}

void TopBar::buildUi()
{
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 4, 12, 4);
    lay->setSpacing(8);

    // Логотип / название
    auto* logo = new QLabel("<b>NestingPro</b>", this);
    logo->setStyleSheet(QString("color: %1; font-size: 16px; letter-spacing: 1px;")
                        .arg(AppPalette::CYAN_TEXT));
    lay->addWidget(logo);
    lay->addSpacing(16);

    // Кнопка «Импортировать DXF»
    auto* importBtn = new QPushButton("↓ Импорт DXF", this);
    importBtn->setProperty("role", "blue");
    connect(importBtn, &QPushButton::clicked,
            this, &TopBar::importDxfRequested);
    lay->addWidget(importBtn);

    // Кнопка «Лист»
    auto* sheetBtn = new QPushButton("⬛ Лист", this);
    connect(sheetBtn, &QPushButton::clicked,
            this, &TopBar::sheetSetupRequested);
    lay->addWidget(sheetBtn);

    lay->addStretch(1);

    // Кнопка «Запустить»
    m_runBtn = new QPushButton("▶ Рассчитать", this);
    m_runBtn->setProperty("role", "green");
    connect(m_runBtn, &QPushButton::clicked,
            this, &TopBar::startNestRequested);
    lay->addWidget(m_runBtn);

    // Кнопка «Отмена»
    m_cancelBtn = new QPushButton("✖ Отмена", this);
    m_cancelBtn->setProperty("role", "danger");
    m_cancelBtn->setVisible(false);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &TopBar::cancelNestRequested);
    lay->addWidget(m_cancelBtn);

    // Кнопка «Экспорт»
    m_exportBtn = new QPushButton("⬆ Экспорт", this);
    m_exportBtn->setEnabled(false);
    connect(m_exportBtn, &QPushButton::clicked,
            this, &TopBar::exportRequested);
    lay->addWidget(m_exportBtn);
}

void TopBar::setRunning(bool running)
{
    m_runBtn->setVisible(!running);
    m_cancelBtn->setVisible(running);
    m_exportBtn->setEnabled(!running);
}
