#ifndef INSTALLATIONPAGE_HPP
#define INSTALLATIONPAGE_HPP

#include <QWizardPage>

#include "ui_installationpage.h"
#include "inisettings.hpp"

namespace Wizard
{
    class MainWizard;
    class IniSettings;

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

        void startInstallation();

    private slots:
        void installationFinished();

    protected:
        void initializePage();


    };

}

#endif // INSTALLATIONPAGE_HPP
