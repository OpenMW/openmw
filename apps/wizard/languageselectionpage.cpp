#include "languageselectionpage.hpp"

#include "mainwizard.hpp"

#include <QDebug>

Wizard::LanguageSelectionPage::LanguageSelectionPage(QWidget *parent) :
    QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

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
    if (field(QLatin1String("installation.retailDisc")).toBool() == true) {
        return MainWizard::Page_ComponentSelection;
    } else {
        QString path(field(QLatin1String("installation.path")).toString());

        if (path.isEmpty())
            return MainWizard::Page_ComponentSelection;

        // Check if we have to install something
        if (mWizard->mInstallations[path].hasMorrowind == true &&
                mWizard->mInstallations[path].hasTribunal == true &&
                mWizard->mInstallations[path].hasBloodmoon == true)
        {
            return MainWizard::Page_Import;
        } else {
            return MainWizard::Page_ComponentSelection;
        }
    }
}
