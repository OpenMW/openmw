#include "methodselectionpage.hpp"
#include <QDebug>
#include "mainwizard.hpp"

Wizard::MethodSelectionPage::MethodSelectionPage(QWidget *parent) :
    QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    registerField(QLatin1String("installation.new"), newLocationRadioButton);
}

int Wizard::MethodSelectionPage::nextId() const
{
    if (field(QLatin1String("installation.new")).toBool() == true) {
        return MainWizard::Page_InstallationTarget;
    } else {
        return MainWizard::Page_ExistingInstallation;
    }
}
