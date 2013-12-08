#include "componentselectionpage.hpp"

#include "mainwizard.hpp"

Wizard::ComponentSelectionPage::ComponentSelectionPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}

int Wizard::ComponentSelectionPage::nextId() const
{
    return MainWizard::Page_Installation;
}
