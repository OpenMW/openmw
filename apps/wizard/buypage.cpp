#include "buypage.hpp"

#include "mainwizard.hpp"

Wizard::BuyPage::BuyPage(QWidget *parent) :
    QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);
}

int Wizard::BuyPage::nextId() const
{
    return -1;
}
