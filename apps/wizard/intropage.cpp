#include "intropage.hpp"

#include "mainwizard.hpp"

Wizard::IntroPage::IntroPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}

int Wizard::IntroPage::nextId() const
{
    return MainWizard::Page_MethodSelection;
}
