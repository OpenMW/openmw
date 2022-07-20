#ifndef IMPORTPAGE_HPP
#define IMPORTPAGE_HPP

#include "ui_importpage.h"

namespace Wizard
{
    class MainWizard;

    class ImportPage : public QWizardPage, private Ui::ImportPage
    {
        Q_OBJECT
    public:
        ImportPage(QWidget *parent);

        int nextId() const override;

    private:
        MainWizard *mWizard;

    };

}

#endif // IMPORTPAGE_HPP
