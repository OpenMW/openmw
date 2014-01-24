#ifndef INSTALLATIONPAGE_HPP
#define INSTALLATIONPAGE_HPP

#include <QWizardPage>

#include "ui_installationpage.h"
#include "inisettings.hpp"

class QThread;

namespace Wizard
{
    class MainWizard;
    class IniSettings;
    class UnshieldWorker;

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

        QThread* mThread;
        UnshieldWorker *mUnshield;

        void startInstallation();

    private slots:
        void showFileDialog(const QString &component);

        void installationFinished();
        void installationError(const QString &text);

    protected:
        void initializePage();


    };

}

#endif // INSTALLATIONPAGE_HPP
