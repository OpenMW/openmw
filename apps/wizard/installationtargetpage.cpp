#include "installationtargetpage.hpp"

#include <QDebug>
#include <QFileDialog>

#include "mainwizard.hpp"

Wizard::InstallationTargetPage::InstallationTargetPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    registerField("installation.path*", targetLineEdit);
}

void Wizard::InstallationTargetPage::initializePage()
{

}

void Wizard::InstallationTargetPage::on_browseButton_clicked()
{
    QString selectedPath = QFileDialog::getExistingDirectory(
                this,
                tr("Select where to install Morrowind"),
                QDir::currentPath(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    qDebug() << selectedPath;
    QFileInfo info(selectedPath);
    if (!info.exists())
        return;

    if (info.isWritable())
        targetLineEdit->setText(info.absoluteFilePath());

}

int Wizard::InstallationTargetPage::nextId() const
{
    return MainWizard::Page_ComponentSelection;
}
