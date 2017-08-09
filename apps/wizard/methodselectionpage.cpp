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
}

int Wizard::MethodSelectionPage::nextId() const
{
    if (field(QLatin1String("installation.retailDisc")).toBool() == true) {
        return MainWizard::Page_InstallationTarget;
    } else {
        return MainWizard::Page_ExistingInstallation;
    }
}
