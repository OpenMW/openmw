#include "importpage.hpp"

#include "mainwizard.hpp"

Wizard::ImportPage::ImportPage(QWidget* parent)
    : QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    registerField(QStringLiteral("installation.import-settings"), importCheckBox);
    registerField(QStringLiteral("installation.import-addons"), addonsCheckBox);
    registerField(QStringLiteral("installation.import-fonts"), fontsCheckBox);
}

int Wizard::ImportPage::nextId() const
{
    return MainWizard::Page_Conclusion;
}
