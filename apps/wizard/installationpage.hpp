#ifndef INSTALLATIONPAGE_HPP
#define INSTALLATIONPAGE_HPP

#include <QWizardPage>

#include "unshield/unshieldworker.hpp"
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
        InstallationPage(QWidget *parent);
        ~InstallationPage() override;

        int nextId() const override;
         bool isComplete() const override;

    private:
        MainWizard *mWizard;
        bool mFinished;

        QThread* mThread;
        UnshieldWorker *mUnshield;

        void startInstallation();

    private slots:
        void showFileDialog(Wizard::Component component);

        void installationFinished();
        void installationError(const QString &text, const QString &details);

    protected:
        void initializePage() override;

    };

}

#endif // INSTALLATIONPAGE_HPP
