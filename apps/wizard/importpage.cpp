#include "importpage.hpp"

#include "mainwizard.hpp"

Wizard::ImportPage::ImportPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);
}

int Wizard::ImportPage::nextId() const
{
    return MainWizard::Page_Conclusion;
}
