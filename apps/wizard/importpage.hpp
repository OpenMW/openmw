#ifndef IMPORTPAGE_HPP
#define IMPORTPAGE_HPP

#include <QWizardPage>

#include "ui_importpage.h"

namespace Wizard
{

    class ImportPage : public QWizardPage, private Ui::ImportPage
    {
        Q_OBJECT
    public:
        ImportPage(QWidget *parent = 0);

    };

}

#endif // IMPORTPAGE_HPP
