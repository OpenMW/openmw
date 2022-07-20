#ifndef EXISTINGINSTALLATIONPAGE_HPP
#define EXISTINGINSTALLATIONPAGE_HPP

#include "ui_existinginstallationpage.h"

#include <components/config/gamesettings.hpp>

namespace Wizard
{
    class MainWizard;

    class ExistingInstallationPage : public QWizardPage, private Ui::ExistingInstallationPage
    {
        Q_OBJECT
    public:
        ExistingInstallationPage(QWidget *parent);

        int nextId() const override;
        bool isComplete() const override;
        bool validatePage() override;

    private slots:
        void on_browseButton_clicked();
        void textChanged(const QString &text);


    private:
        MainWizard *mWizard;

        bool versionIsOK(QString directory_name);

    protected:
        void initializePage() override;
    };

}

#endif // EXISTINGINSTALLATIONPAGE_HPP
