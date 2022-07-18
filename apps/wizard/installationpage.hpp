#ifndef INSTALLATIONPAGE_HPP
#define INSTALLATIONPAGE_HPP

#include <memory>

#include <QWizardPage>

#include "unshield/unshieldworker.hpp"
#include "ui_installationpage.h"
#include "inisettings.hpp"
#include <components/config/gamesettings.hpp>

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
        InstallationPage(QWidget *parent, Config::GameSettings &gameSettings);
        ~InstallationPage() override;

        int nextId() const override;
         bool isComplete() const override;

    private:
        MainWizard *mWizard;
        bool mFinished;

        std::unique_ptr<QThread> mThread;
        std::unique_ptr<UnshieldWorker> mUnshield;

        void startInstallation();

        Config::GameSettings &mGameSettings;

    private slots:
        void showFileDialog(Wizard::Component component);
        void showOldVersionDialog();

        void installationFinished();
        void installationError(const QString &text, const QString &details);

    protected:
        void initializePage() override;

    };

}

#endif // INSTALLATIONPAGE_HPP
