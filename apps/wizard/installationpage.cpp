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

    qDebug() << "installing to: " << field("installation.path").toString();
    logTextEdit->setText(QString("Installing to %1").arg(field("installation.path").toString()));
}

int Wizard::InstallationPage::nextId() const
{
    return MainWizard::Page_Import;
}
