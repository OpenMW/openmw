#include "languageselectionpage.hpp"

#include "mainwizard.hpp"

Wizard::LanguageSelectionPage::LanguageSelectionPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    registerField(QLatin1String("installation.language"), languageComboBox);
}

void Wizard::LanguageSelectionPage::initializePage()
{
    QStringList languages;
    languages << "English"
              << "French"
              << "German"
              << "Italian"
              << "Polish"
              << "Russian"
              << "Spanish";

    languageComboBox->addItems(languages);
}

int Wizard::LanguageSelectionPage::nextId() const
{
    return MainWizard::Page_ComponentSelection;
}
