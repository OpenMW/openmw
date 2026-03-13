#include "languageselectionpage.hpp"

#include <components/misc/scalableicon.hpp>

#include "mainwizard.hpp"

Wizard::LanguageSelectionPage::LanguageSelectionPage(QWidget* parent)
    : QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    flagIcon->setIcon(Misc::ScalableIcon::load(":preferences-desktop-locale"));

    registerField(QLatin1String("installation.language"), languageComboBox, "currentData", "currentDataChanged");
}

void Wizard::LanguageSelectionPage::initializePage()
{
    QVector<std::pair<QString, QString>> languages = { { "English", tr("English") }, { "French", tr("French") },
        { "German", tr("German") }, { "Italian", tr("Italian") }, { "Polish", tr("Polish") },
        { "Russian", tr("Russian") }, { "Spanish", tr("Spanish") } };

    for (const auto& [name, localizedName] : languages)
    {
        languageComboBox->addItem(localizedName, name);
    }
}

int Wizard::LanguageSelectionPage::nextId() const
{
    if (!field(QLatin1String("installation.retailDisc")).toBool())
    {
        QString path(field(QLatin1String("installation.path")).toString());
        if (!path.isEmpty())
        {
            const MainWizard::Installation& installation = mWizard->mInstallations[path];
            if (installation.hasMorrowind && installation.hasTribunal && installation.hasBloodmoon)
                return MainWizard::Page_Import;
        }
    }

    return MainWizard::Page_ComponentSelection;
}
