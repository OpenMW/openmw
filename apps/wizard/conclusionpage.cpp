#include "conclusionpage.hpp"

#include <QDebug>

#include "mainwizard.hpp"

Wizard::ConclusionPage::ConclusionPage(QWidget *parent) :
    QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);
    setPixmap(QWizard::WatermarkPixmap, QPixmap(QLatin1String(":/images/intropage-background.png")));
}

void Wizard::ConclusionPage::initializePage()
{
    // Write the path to openmw.cfg
    if (field(QLatin1String("installation.retailDisc")).toBool() == true) {
        QString path(field(QLatin1String("installation.path")).toString());
        mWizard->addInstallation(path);
    }

    if (!mWizard->mError)
    {
        if ((field(QLatin1String("installation.retailDisc")).toBool() == true)
                || (field(QLatin1String("installation.import-settings")).toBool() == true))
        {
            qDebug() << "IMPORT SETTINGS";
            mWizard->runSettingsImporter();
        }
    }

    if (!mWizard->mError)
    {
        if (field(QLatin1String("installation.retailDisc")).toBool() == true)
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
                              <a href=\"https://gitlab.com/OpenMW/openmw/issues\">bug tracker</a>.<br/>Make sure to include the installation log.</p><br/></body></html>"));
    }
}

int Wizard::ConclusionPage::nextId() const
{
    return -1;
}
