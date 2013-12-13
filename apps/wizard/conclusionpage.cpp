#include "conclusionpage.hpp"

#include <QDebug>

#include "mainwizard.hpp"

Wizard::ConclusionPage::ConclusionPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);
}

void Wizard::ConclusionPage::initializePage()
{
    if (field("installation.new").toBool() == true)
    {
        textLabel->setText(tr("The OpenMW Wizard successfully installed Morrowind on your computer.\n\n") +
                           tr("Click Finish to close the Wizard."));
    } else {
        textLabel->setText(tr("The OpenMW Wizard successfully modified your existing Morrowind installation.\n\n") +
                           tr("Click Finish to close the Wizard."));
    }
}

int Wizard::ConclusionPage::nextId() const
{
    return -1;
}
