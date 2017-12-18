#include "methodselectionpage.hpp"
#include "mainwizard.hpp"

Wizard::MethodSelectionPage::MethodSelectionPage(QWidget *parent) :
    QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

#ifndef OPENMW_USE_UNSHIELD
    retailDiscRadioButton->setEnabled(false);
    existingLocationRadioButton->setChecked(true);
#endif

    registerField(QLatin1String("installation.retailDisc"), retailDiscRadioButton);
    registerField(QLatin1String("installation.existingLocation"), existingLocationRadioButton);
}

int Wizard::MethodSelectionPage::nextId() const
{
    if (field(QLatin1String("installation.retailDisc")).toBool() == true) {
        return MainWizard::Page_InstallationTarget;
    } else if (field(QLatin1String("installation.existingLocation")).toBool() == true) {
        return MainWizard::Page_ExistingInstallation;
    } else {
        return MainWizard::Page_Buy;
    }
}
