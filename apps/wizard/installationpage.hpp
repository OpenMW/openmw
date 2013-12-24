#ifndef INSTALLATIONPAGE_HPP
#define INSTALLATIONPAGE_HPP

#include <QWizardPage>

#include "ui_installationpage.h"

namespace Wizard
{
    class MainWizard;

    class InstallationPage : public QWizardPage, private Ui::InstallationPage
    {
        Q_OBJECT
    public:
        InstallationPage(MainWizard *wizard);

        int nextId() const;

    private:
        MainWizard *mWizard;

        void setupSettings();

    protected:
        void initializePage();


    };

}

#endif // INSTALLATIONPAGE_HPP
