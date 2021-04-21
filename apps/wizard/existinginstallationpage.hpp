#ifndef EXISTINGINSTALLATIONPAGE_HPP
#define EXISTINGINSTALLATIONPAGE_HPP

#include "ui_existinginstallationpage.h"

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

    protected:
        void initializePage() override;

    };

}

#endif // EXISTINGINSTALLATIONPAGE_HPP
