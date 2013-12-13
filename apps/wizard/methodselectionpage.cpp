#include "methodselectionpage.hpp"
#include <QDebug>
#include "mainwizard.hpp"

Wizard::MethodSelectionPage::MethodSelectionPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    registerField("installation.new", newLocationRadioButton);
}

int Wizard::MethodSelectionPage::nextId() const
{
    if (newLocationRadioButton->isChecked()) {
        //wizard()->setField("installation.new", true);
        return MainWizard::Page_InstallationTarget;
    } else {
        //wizard()->setField("installation.new", false);
        return MainWizard::Page_ExistingInstallation;
    }
}
