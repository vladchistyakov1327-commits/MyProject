#include "ImportDxfDialog.h"
#include "core/geometry/DxfImporter.h"
#include "core/logging/AppLogger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFileInfo>

ImportDxfDialog::ImportDxfDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Импорт деталей из DXF");
    setMinimumSize(600, 420);
    buildUi();
}

std::vector<PartEntry> ImportDxfDialog::importedParts() const { return m_result; }

void ImportDxfDialog::buildUi()
{
    auto* dlgLay = new QVBoxLayout(this);
    dlgLay->setContentsMargins(8, 8, 8, 8);
    dlgLay->setSpacing(8);

    // ── Горизонтальная компоновка: список + настройки ─────────────────
    auto* hlay = new QHBoxLayout();
    hlay->setSpacing(8);

    // Левая панель: список файлов
    auto* leftPanel = new QWidget(this);
    auto* leftLay   = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(4);

    m_fileList = new QListWidget(this);
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    auto* btnBar = new QWidget(this);
    auto* btnLay = new QHBoxLayout(btnBar);
    btnLay->setContentsMargins(0, 0, 0, 0);
    auto* addBtn = new QPushButton("+ Добавить DXF", btnBar);
    auto* delBtn = new QPushButton("Удалить",        btnBar);
    delBtn->setProperty("role", "danger");
    btnLay->addWidget(addBtn);
    btnLay->addWidget(delBtn);
    btnLay->addStretch(1);

    leftLay->addWidget(new QLabel("Файлы DXF:", leftPanel));
    leftLay->addWidget(m_fileList);
    leftLay->addWidget(btnBar);

    // Правая панель: настройки + информация
    auto* rightPanel = new QGroupBox("Параметры файла", this);
    auto* rightLay   = new QVBoxLayout(rightPanel);
    rightLay->setSpacing(8);

    auto* paramForm = new QWidget(rightPanel);
    auto* formLay   = new QHBoxLayout(paramForm);
    formLay->setContentsMargins(0, 0, 0, 0);

    auto* qtyLabel = new QLabel("Количество:", paramForm);
    m_qtySpin = new QSpinBox(paramForm);
    m_qtySpin->setRange(1, 9999);
    m_qtySpin->setValue(1);
    m_qtySpin->setEnabled(false);

    auto* rotLabel = new QLabel("Вращение:", paramForm);
    m_rotCb = new QComboBox(paramForm);
    m_rotCb->addItem("90° (0/90/180/270)",  static_cast<int>(RotationMode::STEP_90));
    m_rotCb->addItem("45° (0/45/90/…)",     static_cast<int>(RotationMode::STEP_45));
    m_rotCb->addItem("Без вращения",        static_cast<int>(RotationMode::NONE));
    m_rotCb->setEnabled(false);

    formLay->addWidget(qtyLabel);
    formLay->addWidget(m_qtySpin);
    formLay->addSpacing(16);
    formLay->addWidget(rotLabel);
    formLay->addWidget(m_rotCb);
    formLay->addStretch(1);

    m_infoLabel = new QLabel("Выберите файл из списка", rightPanel);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_infoLabel->setStyleSheet("color: #aaa; font-size: 11px;");

    rightLay->addWidget(paramForm);
    rightLay->addWidget(m_infoLabel);
    rightLay->addStretch(1);

    hlay->addWidget(leftPanel,  2);
    hlay->addWidget(rightPanel, 1);

    // ── Кнопки OK/Cancel ──────────────────────────────────────────────
    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText("Импортировать");

    dlgLay->addLayout(hlay);
    dlgLay->addWidget(btns);

    // ── Соединения ────────────────────────────────────────────────────
    connect(addBtn,     &QPushButton::clicked,
            this, &ImportDxfDialog::onAddFiles);
    connect(delBtn,     &QPushButton::clicked,
            this, &ImportDxfDialog::onRemoveSelected);
    connect(btns,       &QDialogButtonBox::accepted,
            this, &ImportDxfDialog::onImport);
    connect(btns,       &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    connect(m_fileList, &QListWidget::currentRowChanged,
            this, &ImportDxfDialog::onFileSelected);

    connect(m_qtySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int v) {
                const int row = m_fileList->currentRow();
                if (row >= 0 && row < m_files.size())
                    m_files[row].quantity = v;
            });
    connect(m_rotCb, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int idx) {
                const int row = m_fileList->currentRow();
                if (row >= 0 && row < m_files.size())
                    m_files[row].rotMode =
                        static_cast<RotationMode>(m_rotCb->itemData(idx).toInt());
            });
}

void ImportDxfDialog::onAddFiles()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, "Выбрать DXF файлы", {},
        "DXF файлы (*.dxf);;Все файлы (*)");

    for (const QString& path : paths) {
        // Проверить дубликаты
        bool dup = false;
        for (auto& fi : m_files)
            if (fi.path == path) { dup = true; break; }
        if (dup) continue;

        FileItem fi;
        fi.path = path;
        m_files.append(fi);
        m_fileList->addItem(QFileInfo(path).fileName());
    }
}

void ImportDxfDialog::onRemoveSelected()
{
    const QList<QListWidgetItem*> sel = m_fileList->selectedItems();
    for (auto* item : sel) {
        const int row = m_fileList->row(item);
        delete item;
        if (row < m_files.size())
            m_files.removeAt(row);
    }
    m_qtySpin->setEnabled(false);
    m_rotCb->setEnabled(false);
    m_infoLabel->setText("Выберите файл из списка");
}

void ImportDxfDialog::onFileSelected(int row)
{
    if (row < 0 || row >= m_files.size()) {
        m_qtySpin->setEnabled(false);
        m_rotCb->setEnabled(false);
        m_infoLabel->setText("Выберите файл из списка");
        return;
    }
    m_qtySpin->setEnabled(true);
    m_rotCb->setEnabled(true);

    const auto& fi = m_files[row];
    m_qtySpin->setValue(fi.quantity);
    m_rotCb->setCurrentIndex(m_rotCb->findData(static_cast<int>(fi.rotMode)));

    // Предпросмотр: импортировать и показать статистику
    DxfImporter importer;
    const auto geoms = importer.import(fi.path);
    if (geoms.isEmpty()) {
        m_infoLabel->setText("Ошибка: геометрия не найдена");
    } else {
        double totalArea = 0;
        for (const auto& g : geoms) totalArea += g.areaMm2;
        m_infoLabel->setText(
            QString("Файл: %1\nКонтуров: %2\nОбщая площадь: %3 мм²\n"
                    "Bounding box: %4 × %5 мм")
            .arg(QFileInfo(fi.path).fileName())
            .arg(geoms.size())
            .arg(totalArea, 0, 'f', 1)
            .arg(geoms.first().boundingBox.width(), 0, 'f', 1)
            .arg(geoms.first().boundingBox.height(), 0, 'f', 1));
    }
}

void ImportDxfDialog::onImport()
{
    m_result.clear();
    bool hasErrors = false;

    for (const auto& fi : m_files) {
        DxfImporter importer;
        const auto geoms = importer.import(fi.path);
        if (geoms.isEmpty()) {
            LOG_WARN(LogChannel::IMPORT,
                     QString("Пустой DXF при импорте: %1").arg(fi.path));
            hasErrors = true;
            continue;
        }
        for (const auto& g : geoms) {
            PartEntry pe;
            pe.geometry     = g;
            pe.name         = g.sourceName.isEmpty()
                              ? QFileInfo(fi.path).baseName() : g.sourceName;
            pe.quantity     = fi.quantity;
            pe.rotationMode = fi.rotMode;
            m_result.push_back(pe);
        }
    }

    if (m_result.empty()) {
        QMessageBox::warning(this, "Импорт",
            "Не удалось импортировать ни одной детали. Проверьте DXF-файлы.");
        return;
    }

    if (hasErrors) {
        QMessageBox::information(this, "Импорт",
            QString("Импортировано %1 деталей. Некоторые файлы не удалось прочитать "
                    "(проверьте лог).").arg(m_result.size()));
    }

    accept();
}
