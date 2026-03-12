#pragma once
#include <QDialog>
#include "core/nesting/NestResult.h"

/**
 * @brief Диалог экспорта результатов раскладки.
 *
 * Поддерживаемые форматы:
 *   - .lxds (ZIP-архив)
 *   - .dxf  (единый файл)
 *   - .csv  (отчёт)
 *   - .txt  (текстовый отчёт)
 */
class ExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExportDialog(const NestResult& result,
                          QWidget* parent = nullptr);

private slots:
    void onBrowse();
    void onExport();
    void onFormatChanged(int idx);

private:
    void buildUi();
    QString suggestedExtension() const;

    const NestResult& m_result;

    class QComboBox* m_formatCb  = nullptr;
    class QLineEdit* m_pathEdit  = nullptr;
    class QLabel*    m_descLabel = nullptr;
};
