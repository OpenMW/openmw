#include "languageselectionpage.hpp"

#include "mainwizard.hpp"

Wizard::LanguageSelectionPage::LanguageSelectionPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    registerField(QLatin1String("installation.language"), languagesComboBox);
}

int Wizard::LanguageSelectionPage::nextId() const
{
    if (field("installation.new").toBool() == true)
        return MainWizard::Page_InstallationTarget;

    return MainWizard::Page_ExistingInstallation;
}
