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
    if (field("installation.new").toBool() == true) {
        return MainWizard::Page_InstallationTarget;
    } else {
        return MainWizard::Page_ExistingInstallation;
    }
}
