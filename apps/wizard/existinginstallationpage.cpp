#include "existinginstallationpage.hpp"

#include "mainwizard.hpp"

Wizard::ExistingInstallationPage::ExistingInstallationPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}

int Wizard::ExistingInstallationPage::nextId() const
{
    return MainWizard::Page_ComponentSelection;
}
