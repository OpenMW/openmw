#include "conclusionpage.hpp"

Wizard::ConclusionPage::ConclusionPage(QWidget *parent) :
    QWizardPage(parent)
{
    setupUi(this);
}

int Wizard::ConclusionPage::nextId() const
{
    return -1;
}
