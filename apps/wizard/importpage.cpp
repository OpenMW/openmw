#include "importpage.hpp"

#include "mainwizard.hpp"

Wizard::ImportPage::ImportPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}

int Wizard::ImportPage::nextId() const
{
    return MainWizard::Page_Conclusion;
}
