#include "importpage.hpp"

#include "mainwizard.hpp"

Wizard::ImportPage::ImportPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    registerField(QLatin1String("installation.import-settings"), importCheckBox);
    registerField(QLatin1String("installation.import-addons"), addonsCheckBox);
}

int Wizard::ImportPage::nextId() const
{
    return MainWizard::Page_Conclusion;
}
