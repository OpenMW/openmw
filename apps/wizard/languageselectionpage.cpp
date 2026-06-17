#include "languageselectionpage.hpp"

#include <components/misc/scalableicon.hpp>

#include "mainwizard.hpp"

Wizard::LanguageSelectionPage::LanguageSelectionPage(QWidget* parent)
    : QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    flagIcon->setIcon(Misc::ScalableIcon::load(":preferences-desktop-locale"));

    registerField(QStringLiteral("installation.language"), languageComboBox, "currentData", "currentDataChanged");

    const QList<std::pair<QString, QString>> languages = { { tr("English"), QStringLiteral("English") },
        { tr("French"), QStringLiteral("French") }, { tr("German"), QStringLiteral("German") },
        { tr("Italian"), QStringLiteral("Italian") }, { tr("Polish"), QStringLiteral("Polish") },
        { tr("Russian"), QStringLiteral("Russian") }, { tr("Spanish"), QStringLiteral("Spanish") } };

    for (const auto& [localizedName, name] : languages)
    {
        languageComboBox->addItem(localizedName, name);
    }
}

int Wizard::LanguageSelectionPage::nextId() const
{
    if (!field(QStringLiteral("installation.retailDisc")).toBool())
    {
        const QString path(field(QStringLiteral("installation.path")).toString());
        if (!path.isEmpty())
        {
            const MainWizard::Installation& installation = mWizard->mInstallations[path];
            if (installation.hasMorrowind && installation.hasTribunal && installation.hasBloodmoon)
                return MainWizard::Page_Import;
        }
    }

    return MainWizard::Page_ComponentSelection;
}
