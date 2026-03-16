#include "conclusionpage.hpp"

#include <QDebug>

#include "mainwizard.hpp"

Wizard::ConclusionPage::ConclusionPage(QWidget* parent)
    : QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);
    setPixmap(QWizard::WatermarkPixmap, QPixmap(QStringLiteral(":/images/intropage-background.png")));
}

void Wizard::ConclusionPage::initializePage()
{
    const bool retailDisc = field(QStringLiteral("installation.retailDisc")).toBool();
    // Write the path to openmw.cfg
    if (retailDisc)
    {
        const QString path(field(QStringLiteral("installation.path")).toString());
        mWizard->addInstallation(path);
    }

    if (!mWizard->mError)
    {
        if (retailDisc || field(QStringLiteral("installation.import-settings")).toBool())
        {
            mWizard->runSettingsImporter();
        }
    }

    if (!mWizard->mError)
    {
        if (retailDisc)
        {
            textLabel->setText(
                tr("<html><head/><body><p>The OpenMW Wizard successfully installed Morrowind on your "
                   "computer.</p></body></html>"));
        }
        else
        {
            textLabel->setText(
                tr("<html><head/><body><p>The OpenMW Wizard successfully modified your existing Morrowind "
                   "installation.</body></html>"));
        }
    }
    else
    {
        textLabel->setText(
            tr("<html><head/><body><p>The OpenMW Wizard failed to install Morrowind on your computer.</p>"
               "<p>Please report any bugs you might have encountered to our "
               "<a href=\"https://gitlab.com/OpenMW/openmw/issues\">bug tracker</a>.<br/>Make sure to include the "
               "installation log.</p><br/></body></html>"));
    }
}

int Wizard::ConclusionPage::nextId() const
{
    return -1;
}
