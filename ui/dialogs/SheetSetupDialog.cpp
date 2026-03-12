#include "SheetSetupDialog.h"
#include "core/geometry/DxfImporter.h"
#include "core/logging/AppLogger.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>

class SheetSetupDialog::Impl {
public:
    QRadioButton*   rectRadio    = nullptr;
    QRadioButton*   dxfRadio     = nullptr;
    QGroupBox*      rectGroup    = nullptr;
    QGroupBox*      dxfGroup     = nullptr;
    QDoubleSpinBox* widthSpin    = nullptr;
    QDoubleSpinBox* heightSpin   = nullptr;
    QLineEdit*      dxfEdit      = nullptr;
    QSpinBox*       qtySpin      = nullptr;
    QLineEdit*      materialEdit = nullptr;
    QDoubleSpinBox* thickSpin    = nullptr;
    PartGeometry    customShape;
    SheetDefinition res;
};

SheetSetupDialog::SheetSetupDialog(QWidget* parent, const SheetDefinition& initial)
    : QDialog(parent), d(new Impl)
{
    setWindowTitle("Настройка листа");
    setMinimumWidth(400);
    buildUi(initial);
}

SheetSetupDialog::~SheetSetupDialog()
{
    delete d;
}

void SheetSetupDialog::buildUi(const SheetDefinition& s)
{
    auto* mainLay = new QVBoxLayout(this);
    mainLay->setSpacing(12);

    // Тип листа
    {
        auto* grp = new QGroupBox("Тип", this);
        auto* lay = new QHBoxLayout(grp);
        d->rectRadio = new QRadioButton("Прямоугольник", grp);
        d->dxfRadio  = new QRadioButton("Контур DXF",    grp);
        d->rectRadio->setChecked(s.type == SheetDefinition::Type::RECTANGLE);
        d->dxfRadio->setChecked( s.type == SheetDefinition::Type::CUSTOM_DXF);
        lay->addWidget(d->rectRadio);
        lay->addWidget(d->dxfRadio);
        mainLay->addWidget(grp);
        connect(d->rectRadio, &QRadioButton::toggled, this, &SheetSetupDialog::onTypeToggled);
    }

    // Прямоугольник
    {
        d->rectGroup = new QGroupBox("Размеры листа", this);
        auto* form = new QFormLayout(d->rectGroup);
        d->widthSpin  = new QDoubleSpinBox(d->rectGroup);
        d->widthSpin->setRange(100, 20000); d->widthSpin->setSuffix(" мм");
        d->widthSpin->setValue(s.widthMm);
        d->heightSpin = new QDoubleSpinBox(d->rectGroup);
        d->heightSpin->setRange(100, 20000); d->heightSpin->setSuffix(" мм");
        d->heightSpin->setValue(s.heightMm);
        form->addRow("Ширина:", d->widthSpin);
        form->addRow("Высота:", d->heightSpin);
        mainLay->addWidget(d->rectGroup);
    }

    // DXF контур
    {
        d->dxfGroup = new QGroupBox("Файл контура листа", this);
        auto* lay = new QHBoxLayout(d->dxfGroup);
        d->dxfEdit = new QLineEdit(d->dxfGroup);
        d->dxfEdit->setReadOnly(true);
        d->dxfEdit->setPlaceholderText("Выберите DXF…");
        if (s.type == SheetDefinition::Type::CUSTOM_DXF)
            d->dxfEdit->setText(s.customShape.sourceName);
        auto* btn = new QPushButton("…", d->dxfGroup);
        btn->setFixedWidth(30);
        lay->addWidget(d->dxfEdit);
        lay->addWidget(btn);
        mainLay->addWidget(d->dxfGroup);
        connect(btn, &QPushButton::clicked, this, &SheetSetupDialog::onBrowseDxf);
    }

    onTypeToggled();

    // Общие параметры
    {
        auto* grp  = new QGroupBox("Параметры", this);
        auto* form = new QFormLayout(grp);

        d->qtySpin = new QSpinBox(grp);
        d->qtySpin->setRange(0, 9999);
        d->qtySpin->setSpecialValueText("∞ (без ограничений)");
        d->qtySpin->setValue(s.quantity);
        form->addRow("Кол-во листов:", d->qtySpin);

        d->materialEdit = new QLineEdit(grp);
        d->materialEdit->setText(s.materialId);
        d->materialEdit->setPlaceholderText("Например: AISI 304");
        form->addRow("Материал:", d->materialEdit);

        d->thickSpin = new QDoubleSpinBox(grp);
        d->thickSpin->setRange(0.1, 100.0);
        d->thickSpin->setSuffix(" мм");
        d->thickSpin->setValue(s.thicknessMm);
        form->addRow("Толщина:", d->thickSpin);

        mainLay->addWidget(grp);
    }

    // Кнопки
    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText("ОК");
    mainLay->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, this, &SheetSetupDialog::onAccept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SheetSetupDialog::onTypeToggled()
{
    const bool isRect = d->rectRadio->isChecked();
    d->rectGroup->setEnabled(isRect);
    d->dxfGroup->setEnabled(!isRect);
}

void SheetSetupDialog::onBrowseDxf()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Контур листа (DXF)", {},
        "DXF файлы (*.dxf);;Все файлы (*)");
    if (path.isEmpty()) return;

    DxfImporter imp;
    const auto geoms = imp.import(path);
    if (geoms.isEmpty()) {
        QMessageBox::warning(this, "Ошибка",
            "Не удалось прочитать контур листа из DXF.");
        return;
    }
    d->customShape = geoms.first();
    d->dxfEdit->setText(QFileInfo(path).fileName());
}

void SheetSetupDialog::onAccept()
{
    SheetDefinition def;
    def.quantity    = d->qtySpin->value();
    def.materialId  = d->materialEdit->text();
    def.thicknessMm = d->thickSpin->value();

    if (d->dxfRadio->isChecked()) {
        if (d->customShape.outerContour.vertices.empty()) {
            QMessageBox::warning(this, "Ошибка",
                "Выберите DXF-файл с контуром листа.");
            return;
        }
        def.type        = SheetDefinition::Type::CUSTOM_DXF;
        def.customShape = d->customShape;
    } else {
        def.type      = SheetDefinition::Type::RECTANGLE;
        def.widthMm   = d->widthSpin->value();
        def.heightMm  = d->heightSpin->value();
    }
    d->res = def;
    accept();
}

SheetDefinition SheetSetupDialog::result() const { return d->res; }
