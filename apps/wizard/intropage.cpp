#include "intropage.hpp"

#include "mainwizard.hpp"

Wizard::IntroPage::IntroPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)

{
    setupUi(this);
}

int Wizard::IntroPage::nextId() const
{
    return MainWizard::Page_MethodSelection;
}
