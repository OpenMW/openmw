#ifndef EXISTINGINSTALLATIONPAGE_HPP
#define EXISTINGINSTALLATIONPAGE_HPP

#include <QWizardPage>

#include "ui_existinginstallationpage.h"

namespace Wizard
{
    class MainWizard;

    class ExistingInstallationPage : public QWizardPage, private Ui::ExistingInstallationPage
    {
        Q_OBJECT
    public:
        ExistingInstallationPage(QWidget *parent);

        int nextId() const;
        virtual bool isComplete() const;
        virtual bool validatePage();

    private slots:
        void on_browseButton_clicked();
        void textChanged(const QString &text);


    private:
        MainWizard *mWizard;

    protected:
        void initializePage();

    };

}

#endif // EXISTINGINSTALLATIONPAGE_HPP
