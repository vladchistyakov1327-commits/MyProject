#include "SheetsTab.h"
#include "ui/widgets/UtilizationBar.h"
#include "core/geometry/DxfImporter.h"
#include "core/logging/AppLogger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QScrollArea>
#include <QLabel>
#include <QFrame>
#include <QFileDialog>
#include <QFileInfo>

class SheetsTab::Impl {
public:
    // Тип листа
    QRadioButton*   rectRadio     = nullptr;
    QRadioButton*   dxfRadio      = nullptr;

    // Прямоугольный лист
    QDoubleSpinBox* widthSpin     = nullptr;
    QDoubleSpinBox* heightSpin    = nullptr;
    QGroupBox*      rectGroup     = nullptr;

    // DXF лист
    QLineEdit*      dxfPathEdit   = nullptr;
    QGroupBox*      dxfGroup      = nullptr;
    PartGeometry    customShape;

    // Общие
    QSpinBox*       qtySpin       = nullptr;
    QLineEdit*      materialEdit  = nullptr;
    QDoubleSpinBox* thickSpin     = nullptr;

    // Карточки результатов
    QWidget*        cardsWidget   = nullptr;
    QVBoxLayout*    cardsLayout   = nullptr;
};

SheetsTab::SheetsTab(QWidget* parent) : QWidget(parent), d(new Impl)
{
    buildUi();
}

SheetsTab::~SheetsTab()
{
    delete d;
}

void SheetsTab::buildUi()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget();
    auto* mainLay = new QVBoxLayout(container);
    mainLay->setContentsMargins(8, 8, 8, 8);
    mainLay->setSpacing(12);

    // ── Тип листа ─────────────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Тип листа", container);
        auto* lay = new QVBoxLayout(grp);

        auto* radioRow = new QWidget(grp);
        auto* radioLay = new QHBoxLayout(radioRow);
        radioLay->setContentsMargins(0,0,0,0);
        d->rectRadio = new QRadioButton("Прямоугольник", radioRow);
        d->dxfRadio  = new QRadioButton("Контур из DXF", radioRow);
        d->rectRadio->setChecked(true);
        radioLay->addWidget(d->rectRadio);
        radioLay->addWidget(d->dxfRadio);
        radioLay->addStretch(1);
        lay->addWidget(radioRow);

        // Подгруппа: Прямоугольник
        d->rectGroup = new QGroupBox("Размеры", grp);
        auto* rectForm = new QFormLayout(d->rectGroup);
        d->widthSpin  = new QDoubleSpinBox(d->rectGroup);
        d->widthSpin->setRange(100.0, 20000.0);
        d->widthSpin->setSuffix(" мм");
        d->widthSpin->setValue(3000.0);
        d->heightSpin = new QDoubleSpinBox(d->rectGroup);
        d->heightSpin->setRange(100.0, 20000.0);
        d->heightSpin->setSuffix(" мм");
        d->heightSpin->setValue(1500.0);
        rectForm->addRow("Ширина:", d->widthSpin);
        rectForm->addRow("Высота:", d->heightSpin);
        lay->addWidget(d->rectGroup);

        // Подгруппа: DXF
        d->dxfGroup = new QGroupBox("Файл контура", grp);
        auto* dxfLay = new QHBoxLayout(d->dxfGroup);
        d->dxfPathEdit = new QLineEdit(d->dxfGroup);
        d->dxfPathEdit->setReadOnly(true);
        d->dxfPathEdit->setPlaceholderText("Выберите DXF файл…");
        auto* browseBtn = new QPushButton("…", d->dxfGroup);
        browseBtn->setFixedWidth(30);
        dxfLay->addWidget(d->dxfPathEdit);
        dxfLay->addWidget(browseBtn);
        d->dxfGroup->setVisible(false);
        lay->addWidget(d->dxfGroup);

        mainLay->addWidget(grp);

        connect(d->rectRadio, &QRadioButton::toggled, this, &SheetsTab::onTypeToggled);
        connect(browseBtn, &QPushButton::clicked, this, &SheetsTab::onLoadSheetDxf);
    }

    // ── Общие параметры ───────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Параметры листа", container);
        auto* form = new QFormLayout(grp);

        d->qtySpin = new QSpinBox(grp);
        d->qtySpin->setRange(0, 9999);
        d->qtySpin->setValue(0);
        d->qtySpin->setSpecialValueText("Без ограничений");
        form->addRow("Кол-во листов:", d->qtySpin);

        d->materialEdit = new QLineEdit(grp);
        d->materialEdit->setPlaceholderText("Например: AISI 304");
        form->addRow("Материал:", d->materialEdit);

        d->thickSpin = new QDoubleSpinBox(grp);
        d->thickSpin->setRange(0.1, 100.0);
        d->thickSpin->setSuffix(" мм");
        d->thickSpin->setValue(3.0);
        form->addRow("Толщина:", d->thickSpin);

        mainLay->addWidget(grp);
    }

    // ── Карточки результатов ──────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Результаты по листам", container);
        d->cardsLayout = new QVBoxLayout(grp);
        d->cardsLayout->setSpacing(4);
        d->cardsWidget = grp;
        mainLay->addWidget(grp);
    }

    mainLay->addStretch(1);
    scroll->setWidget(container);

    auto* outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->addWidget(scroll);

    // Сигналы изменений
    auto changed = [this] { emit sheetChanged(); };
    connect(d->widthSpin,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, changed);
    connect(d->heightSpin,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, changed);
    connect(d->qtySpin,      QOverload<int>::of(&QSpinBox::valueChanged),          this, changed);
    connect(d->materialEdit, &QLineEdit::textChanged,                              this, changed);
    connect(d->thickSpin,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, changed);
}

void SheetsTab::onTypeToggled()
{
    const bool isRect = d->rectRadio->isChecked();
    d->rectGroup->setVisible(isRect);
    d->dxfGroup->setVisible(!isRect);
    emit sheetChanged();
}

void SheetsTab::onLoadSheetDxf()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Выбрать контур листа (DXF)", {},
        "DXF файлы (*.dxf);;Все файлы (*)");
    if (path.isEmpty()) return;

    DxfImporter importer;
    const auto geoms = importer.import(path);
    if (geoms.isEmpty()) {
        LOG_WARN(LogChannel::IMPORT,
                 QString("Не удалось импортировать контур листа: %1").arg(path));
        return;
    }
    d->customShape = geoms.first();
    d->dxfPathEdit->setText(QFileInfo(path).fileName());
    emit sheetChanged();
}

void SheetsTab::setSheetDefinition(const SheetDefinition& def)
{
    d->widthSpin->setValue(def.widthMm);
    d->heightSpin->setValue(def.heightMm);
    d->qtySpin->setValue(def.quantity);
    d->materialEdit->setText(def.materialId);
    d->thickSpin->setValue(def.thicknessMm);

    const bool isRect = (def.type == SheetDefinition::Type::RECTANGLE);
    d->rectRadio->setChecked(isRect);
    d->dxfRadio->setChecked(!isRect);
    d->rectGroup->setVisible(isRect);
    d->dxfGroup->setVisible(!isRect);

    if (def.type == SheetDefinition::Type::CUSTOM_DXF) {
        d->customShape = def.customShape;
        d->dxfPathEdit->setText(def.customShape.sourceName);
    }
}

SheetDefinition SheetsTab::sheetDefinition() const
{
    SheetDefinition def;
    def.widthMm     = d->widthSpin->value();
    def.heightMm    = d->heightSpin->value();
    def.quantity    = d->qtySpin->value();
    def.materialId  = d->materialEdit->text();
    def.thicknessMm = d->thickSpin->value();

    if (d->dxfRadio->isChecked()) {
        def.type        = SheetDefinition::Type::CUSTOM_DXF;
        def.customShape = d->customShape;
    } else {
        def.type = SheetDefinition::Type::RECTANGLE;
    }
    return def;
}

void SheetsTab::updateResults(const NestResult& result)
{
    // Удалить старые карточки
    while (QLayoutItem* item = d->cardsLayout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Создать карточку для каждого листа
    for (int i = 0; i < (int)result.sheets.size(); ++i) {
        const auto& sr = result.sheets[i];

        auto* card = new QFrame(d->cardsWidget);
        card->setFrameShape(QFrame::StyledPanel);
        auto* cardLay = new QHBoxLayout(card);

        auto* lbl = new QLabel(
            QString("Лист %1  (%2 дет.)").arg(i + 1).arg(sr.placedParts.size()),
            card);
        lbl->setMinimumWidth(140);

        auto* bar = new UtilizationBar(card);
        bar->setValue(static_cast<int>(sr.utilizationPercent));
        bar->setMinimumWidth(160);

        auto* pctLbl = new QLabel(
            QString("%1%").arg(sr.utilizationPercent, 0, 'f', 1), card);
        pctLbl->setFixedWidth(46);

        cardLay->addWidget(lbl);
        cardLay->addWidget(bar, 1);
        cardLay->addWidget(pctLbl);

        d->cardsLayout->addWidget(card);
    }

    if (result.sheets.empty()) {
        d->cardsLayout->addWidget(new QLabel("Нет результатов", d->cardsWidget));
    }
}
