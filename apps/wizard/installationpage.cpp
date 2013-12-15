#include "installationpage.hpp"

#include <QDebug>

#include "mainwizard.hpp"

Wizard::InstallationPage::InstallationPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);
}

void Wizard::InstallationPage::initializePage()
{
    QString path = field("installation.path").toString();
    QStringList components = field("installation.components").toStringList();

    logTextEdit->append(QString("Installing to %1").arg(path));
    logTextEdit->append(QString("Installing %1.").arg(components.join(", ")));

}

int Wizard::InstallationPage::nextId() const
{
    return MainWizard::Page_Import;
}
