#include "ExportDialog.h"
#include "core/export/LxdsExporter.h"
#include "core/export/DxfExporter.h"
#include "core/export/ReportGenerator.h"
#include "core/logging/AppLogger.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>

enum class ExportFormat { LXDS = 0, DXF, CSV, TXT };

ExportDialog::ExportDialog(const NestResult& result, QWidget* parent)
    : QDialog(parent), m_result(result)
{
    setWindowTitle("Экспорт результатов");
    setMinimumWidth(450);
    buildUi();
}

void ExportDialog::buildUi()
{
    auto* mainLay = new QVBoxLayout(this);
    mainLay->setSpacing(12);
    mainLay->setContentsMargins(12, 12, 12, 12);

    // ── Формат ────────────────────────────────────────────────────────
    {
        auto* grp  = new QGroupBox("Формат экспорта", this);
        auto* form = new QFormLayout(grp);

        m_formatCb = new QComboBox(grp);
        m_formatCb->addItem(".lxds — CypCut/HypCut (ZIP + DXF)",
                            static_cast<int>(ExportFormat::LXDS));
        m_formatCb->addItem(".dxf — Единый DXF-файл",
                            static_cast<int>(ExportFormat::DXF));
        m_formatCb->addItem(".csv — Отчёт (таблица)",
                            static_cast<int>(ExportFormat::CSV));
        m_formatCb->addItem(".txt — Текстовый отчёт",
                            static_cast<int>(ExportFormat::TXT));
        form->addRow("Формат:", m_formatCb);

        m_descLabel = new QLabel(
            "Архив .lxds содержит layout.xml и DXF-файлы деталей.\n"
            "Совместим с CypCut / HypCut / LazCam.", grp);
        m_descLabel->setWordWrap(true);
        m_descLabel->setStyleSheet("color: #aaa; font-size: 11px;");
        form->addRow(m_descLabel);

        mainLay->addWidget(grp);
    }

    // ── Путь ──────────────────────────────────────────────────────────
    {
        auto* grp = new QGroupBox("Путь к файлу", this);
        auto* lay = new QHBoxLayout(grp);

        m_pathEdit = new QLineEdit(grp);
        m_pathEdit->setPlaceholderText("Укажите путь для сохранения…");

        auto* browseBtn = new QPushButton("…", grp);
        browseBtn->setFixedWidth(30);

        lay->addWidget(m_pathEdit);
        lay->addWidget(browseBtn);
        mainLay->addWidget(grp);

        connect(browseBtn, &QPushButton::clicked, this, &ExportDialog::onBrowse);
    }

    // ── Кнопки ────────────────────────────────────────────────────────
    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText("Экспортировать");
    mainLay->addStretch(1);
    mainLay->addWidget(btns);

    connect(m_formatCb, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportDialog::onFormatChanged);
    connect(btns, &QDialogButtonBox::accepted, this, &ExportDialog::onExport);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString ExportDialog::suggestedExtension() const
{
    switch (static_cast<ExportFormat>(m_formatCb->currentData().toInt())) {
        case ExportFormat::LXDS: return ".lxds";
        case ExportFormat::DXF:  return ".dxf";
        case ExportFormat::CSV:  return ".csv";
        case ExportFormat::TXT:  return ".txt";
    }
    return ".lxds";
}

void ExportDialog::onFormatChanged(int /*idx*/)
{
    switch (static_cast<ExportFormat>(m_formatCb->currentData().toInt())) {
        case ExportFormat::LXDS:
            m_descLabel->setText(
                "Архив .lxds содержит layout.xml и DXF-файлы деталей.\n"
                "Совместим с CypCut / HypCut / LazCam.");
            break;
        case ExportFormat::DXF:
            m_descLabel->setText(
                "Экспорт раскладки в единый DXF-файл (AC1015, LWPOLYLINE).\n"
                "Все листы на отдельных слоях.");
            break;
        case ExportFormat::CSV:
            m_descLabel->setText("Таблица результатов в формате CSV (UTF-8).");
            break;
        case ExportFormat::TXT:
            m_descLabel->setText("Форматированный текстовый отчёт о раскладке.");
            break;
    }
    // Обновить расширение в поле пути
    QString cur = m_pathEdit->text();
    if (!cur.isEmpty()) {
        QFileInfo fi(cur);
        m_pathEdit->setText(fi.absolutePath() + "/" +
                            fi.baseName() + suggestedExtension());
    }
}

void ExportDialog::onBrowse()
{
    const auto fmt = static_cast<ExportFormat>(
        m_formatCb->currentData().toInt());

    QString filter;
    switch (fmt) {
        case ExportFormat::LXDS: filter = "LXDS файлы (*.lxds)"; break;
        case ExportFormat::DXF:  filter = "DXF файлы (*.dxf)";   break;
        case ExportFormat::CSV:  filter = "CSV файлы (*.csv)";   break;
        case ExportFormat::TXT:  filter = "Text files (*.txt)";  break;
    }
    filter += ";;Все файлы (*)";

    const QString path = QFileDialog::getSaveFileName(
        this, "Сохранить как…", {}, filter);
    if (!path.isEmpty())
        m_pathEdit->setText(path);
}

void ExportDialog::onExport()
{
    const QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Экспорт",
            "Укажите путь для сохранения файла.");
        return;
    }

    const auto fmt = static_cast<ExportFormat>(
        m_formatCb->currentData().toInt());

    bool ok = false;
    QString errorMsg;

    try {
        switch (fmt) {
            case ExportFormat::LXDS: {
                LxdsExporter exp;
                ok = exp.exportToFile(m_result, path);
                if (!ok) errorMsg = exp.lastError();
                break;
            }
            case ExportFormat::DXF: {
                DxfExporter exp;
                ok = exp.exportLayoutToFile(m_result, path);
                if (!ok) errorMsg = exp.lastError();
                break;
            }
            case ExportFormat::CSV: {
                ReportGenerator gen;
                ok = gen.generateCsv(m_result, path);
                break;
            }
            case ExportFormat::TXT: {
                ReportGenerator gen;
                ok = gen.generateText(m_result, path);
                break;
            }
        }
    } catch (const std::exception& ex) {
        ok = false;
        errorMsg = ex.what();
    }

    if (ok) {
        LOG_INFO(LogChannel::EXPORT,
                 QString("Экспорт завершён: %1").arg(path));
        QMessageBox::information(this, "Экспорт",
            QString("Файл успешно сохранён:\n%1").arg(path));
        accept();
    } else {
        LOG_ERR(LogChannel::EXPORT,
                QString("Ошибка экспорта: %1").arg(errorMsg));
        QMessageBox::critical(this, "Ошибка экспорта",
            QString("Не удалось сохранить файл.\n%1").arg(errorMsg));
    }
}
