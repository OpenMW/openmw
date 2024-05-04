#include "languageselectionpage.hpp"

#include "mainwizard.hpp"

Wizard::LanguageSelectionPage::LanguageSelectionPage(QWidget* parent)
    : QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    flagIconLabel->setPixmap(QIcon(":preferences-desktop-locale").pixmap(QSize(48, 48)));

    registerField(QLatin1String("installation.language"), languageComboBox, "currentData", "currentDataChanged");
}

void Wizard::LanguageSelectionPage::initializePage()
{
    QVector<std::pair<QString, QString>> languages = { { "English", tr("English") }, { "French", tr("French") },
        { "German", tr("German") }, { "Italian", tr("Italian") }, { "Polish", tr("Polish") },
        { "Russian", tr("Russian") }, { "Spanish", tr("Spanish") } };

    for (auto lang : languages)
    {
        languageComboBox->addItem(lang.second, lang.first);
    }
}

int Wizard::LanguageSelectionPage::nextId() const
{
    if (field(QLatin1String("installation.retailDisc")).toBool() == true)
    {
        return MainWizard::Page_ComponentSelection;
    }
    else
    {
        QString path(field(QLatin1String("installation.path")).toString());

        if (path.isEmpty())
            return MainWizard::Page_ComponentSelection;

        // Check if we have to install something
        if (mWizard->mInstallations[path].hasMorrowind == true && mWizard->mInstallations[path].hasTribunal == true
            && mWizard->mInstallations[path].hasBloodmoon == true)
        {
            return MainWizard::Page_Import;
        }
        else
        {
            return MainWizard::Page_ComponentSelection;
        }
    }
}
