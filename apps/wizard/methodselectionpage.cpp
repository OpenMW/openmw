#include "methodselectionpage.hpp"

#include "mainwizard.hpp"

Wizard::MethodSelectionPage::MethodSelectionPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}

int Wizard::MethodSelectionPage::nextId() const
{
    if (newLocationRadioButton->isChecked()) {
        return MainWizard::Page_InstallationTarget;
    } else {
        return MainWizard::Page_ExistingInstallation;
    }
}
