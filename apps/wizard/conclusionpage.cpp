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
    if (!mWizard->mError)
    {
        if (field("installation.new").toBool() == true)
        {
            textLabel->setText(tr("<html><head/><body><p>The OpenMW Wizard successfully installed Morrowind on your computer.</p> \
                                  <p>Click Finish to close the Wizard.</p></body></html>"));
        } else {
            textLabel->setText(tr("<html><head/><body><p>The OpenMW Wizard successfully modified your existing Morrowind installation.</p> \
                                  <p>Click Finish to close the Wizard.</p></body></html>"));
        }
    } else {
        textLabel->setText(tr("<html><head/><body><p>The OpenMW Wizard failed to install Morrowind on your computer.</p> \
                              <p>Please report any bugs you might have encountered to our \
                              <a href=\"https://bugs.openmw.org\">bug tracker</a>.<br/>Make sure to include the installation log.</p><br/></body></html>"));
    }
}

int Wizard::ConclusionPage::nextId() const
{
    return -1;
}
