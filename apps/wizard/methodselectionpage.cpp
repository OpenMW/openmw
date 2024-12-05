#include "methodselectionpage.hpp"
#include "mainwizard.hpp"

#include <components/misc/scalableicon.hpp>

#include <QDesktopServices>
#include <QUrl>

Wizard::MethodSelectionPage::MethodSelectionPage(QWidget* parent)
    : QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    installerIcon->setIcon(Misc::ScalableIcon::load(":system-installer"));
    folderIcon->setIcon(Misc::ScalableIcon::load(":folder"));
    buyLinkButton->setIcon(Misc::ScalableIcon::load(":dollar"));

#ifndef OPENMW_USE_UNSHIELD
    retailDiscRadioButton->setEnabled(false);
    existingLocationRadioButton->setChecked(true);
    buyLinkButton->released();
#endif

    QFont font = existingLocationRadioButton->font();
    font.setBold(true);
    existingLocationRadioButton->setFont(font);
    retailDiscRadioButton->setFont(font);

    registerField(QLatin1String("installation.retailDisc"), retailDiscRadioButton);

    connect(buyLinkButton, &QPushButton::released, this, &MethodSelectionPage::handleBuyButton);
}

int Wizard::MethodSelectionPage::nextId() const
{
    if (field(QLatin1String("installation.retailDisc")).toBool() == true)
    {
        return MainWizard::Page_InstallationTarget;
    }
    else
    {
        return MainWizard::Page_ExistingInstallation;
    }
}

void Wizard::MethodSelectionPage::handleBuyButton()
{
    QDesktopServices::openUrl(QUrl("https://openmw.org/faq/#do_i_need_morrowind"));
}
