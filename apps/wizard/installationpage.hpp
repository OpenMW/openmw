#ifndef INSTALLATIONPAGE_HPP
#define INSTALLATIONPAGE_HPP

#include <QWizardPage>

#include "ui_installationpage.h"

namespace Wizard
{

    class InstallationPage : public QWizardPage, private Ui::InstallationPage
    {
        Q_OBJECT
    public:
        InstallationPage(QWidget *parent = 0);

    };

}

#endif // INSTALLATIONPAGE_HPP
