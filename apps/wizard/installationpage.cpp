#include "installationpage.hpp"

#include "mainwizard.hpp"

Wizard::InstallationPage::InstallationPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}

int Wizard::InstallationPage::nextId() const
{
    return MainWizard::Page_Import;
}
