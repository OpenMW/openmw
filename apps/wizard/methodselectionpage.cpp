#include "methodselectionpage.hpp"
#include "mainwizard.hpp"

#include <QUrl>
#include <QDesktopServices>

Wizard::MethodSelectionPage::MethodSelectionPage(QWidget *parent) :
    QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

#ifndef OPENMW_USE_UNSHIELD
    retailDiscRadioButton->setEnabled(false);
    existingLocationRadioButton->setChecked(true);
    buyLinkButton->released();
#endif
    
    registerField(QLatin1String("installation.retailDisc"), retailDiscRadioButton);
    
    connect(buyLinkButton, SIGNAL(released()), this, SLOT(handleBuyButton()));
}

int Wizard::MethodSelectionPage::nextId() const
{
    if (field(QLatin1String("installation.retailDisc")).toBool() == true) {
        return MainWizard::Page_InstallationTarget;
    } else {
        return MainWizard::Page_ExistingInstallation;
    }
}

void Wizard::MethodSelectionPage::handleBuyButton()
{
    QDesktopServices::openUrl(QUrl("https://openmw.org/faq/#do_i_need_morrowind"));
}
