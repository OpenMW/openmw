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
    languages << QLatin1String("English")
              << QLatin1String("French")
              << QLatin1String("German")
              << QLatin1String("Italian")
              << QLatin1String("Polish")
              << QLatin1String("Russian")
              << QLatin1String("Spanish");

    languageComboBox->addItems(languages);
}

int Wizard::LanguageSelectionPage::nextId() const
{
    // Check if we have to install something
    QString path(field(QLatin1String("installation.path")).toString());

    if (path.isEmpty())
        return MainWizard::Page_ComponentSelection;

    if (mWizard->mInstallations[path]->hasMorrowind == true &&
            mWizard->mInstallations[path]->hasTribunal == true &&
            mWizard->mInstallations[path]->hasBloodmoon == true)
    {
        return MainWizard::Page_Import;
    } else {
        return MainWizard::Page_ComponentSelection;
    }
}
