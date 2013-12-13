#include "languageselectionpage.hpp"

#include "mainwizard.hpp"

Wizard::LanguageSelectionPage::LanguageSelectionPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);
}

int Wizard::LanguageSelectionPage::nextId() const
{
    if (field("installation.new").toBool() == true)
        return MainWizard::Page_InstallationTarget;

    return MainWizard::Page_ExistingInstallation;
}
