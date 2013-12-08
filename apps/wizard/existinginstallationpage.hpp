#ifndef EXISTINGINSTALLATIONPAGE_HPP
#define EXISTINGINSTALLATIONPAGE_HPP

#include <QWizardPage>

#include "ui_existinginstallationpage.h"

namespace Wizard
{

    class ExistingInstallationPage : public QWizardPage, private Ui::ExistingInstallationPage
    {
        Q_OBJECT
    public:
        ExistingInstallationPage(QWidget *parent = 0);

    };

}

#endif // EXISTINGINSTALLATIONPAGE_HPP
