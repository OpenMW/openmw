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
         virtual bool isComplete() const;

    private:
        MainWizard *mWizard;
        bool mFinished;

        void setupSettings();
        void startInstallation();

    private slots:
        void installationFinished();

    protected:
        void initializePage();


    };

}

#endif // INSTALLATIONPAGE_HPP
