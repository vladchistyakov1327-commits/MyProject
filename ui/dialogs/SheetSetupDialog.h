#pragma once
#include <QDialog>
#include "core/nesting/NestJob.h"

/**
 * @brief Диалог настройки листа-заготовки.
 *
 * Создаёт / редактирует SheetDefinition.
 */
class SheetSetupDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SheetSetupDialog(QWidget* parent = nullptr,
                               const SheetDefinition& initial = {});
    ~SheetSetupDialog();

    SheetDefinition result() const;

private:
    void buildUi(const SheetDefinition& initial);
    void onBrowseDxf();
    void onAccept();
    void onTypeToggled();

    class Impl;
    Impl* d = nullptr;
};
