#pragma once
#include <QDialog>
#include <QList>
#include "core/nesting/NestJob.h"

class QListWidget;
class QSpinBox;
class QLabel;
class QComboBox;

/**
 * @brief Диалог импорта DXF-файлов деталей.
 *
 * Позволяет добавить несколько DXF, задать количество и параметры вращения
 * для каждого файла, видеть предварительную информацию о геометрии.
 */
class ImportDxfDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImportDxfDialog(QWidget* parent = nullptr);

    /** Список деталей после подтверждения диалога. */
    std::vector<PartEntry> importedParts() const;

private slots:
    void onAddFiles();
    void onRemoveSelected();
    void onImport();
    void onFileSelected(int row);

private:
    void buildUi();

    QListWidget* m_fileList = nullptr;
    QLabel*      m_infoLabel = nullptr;
    QSpinBox*    m_qtySpin   = nullptr;
    QComboBox*   m_rotCb     = nullptr;

    struct FileItem {
        QString path;
        int     quantity = 1;
        RotationMode rotMode = RotationMode::STEP_90;
    };
    QList<FileItem>        m_files;
    std::vector<PartEntry> m_result;
};
