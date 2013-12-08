#include "installationtargetpage.hpp"

#include "mainwizard.hpp"

Wizard::InstallationTargetPage::InstallationTargetPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}

int Wizard::InstallationTargetPage::nextId() const
{
    return MainWizard::Page_ComponentSelection;
}
