#include "installationtargetpage.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#include <components/files/configurationmanager.hpp>
#include <components/files/qtconversion.hpp>
#include <components/misc/scalableicon.hpp>

#include "mainwizard.hpp"

Wizard::InstallationTargetPage::InstallationTargetPage(QWidget* parent, const Files::ConfigurationManager& cfg)
    : QWizardPage(parent)
    , mCfgMgr(cfg)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);
    connect(browseButton, &QPushButton::clicked, this, &InstallationTargetPage::browseButtonClicked);

    folderIcon->setIcon(Misc::ScalableIcon::load(":folder"));

    registerField(QLatin1String("installation.path*"), targetLineEdit);
}

void Wizard::InstallationTargetPage::initializePage()
{
    const QDir dir(Files::pathToQString(mCfgMgr.getUserDataPath() / "basedata"));
    targetLineEdit->setText(QDir::toNativeSeparators(dir.absolutePath()));
}

bool Wizard::InstallationTargetPage::validatePage()
{
    const QString path(field(QLatin1String("installation.path")).toString());

    qDebug() << "Validating path: " << path;

    if (!QFile::exists(path))
    {
        QDir dir;

        if (!dir.mkpath(path))
        {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(tr("Error creating destination"));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(
                tr("<html><head/><body><p><b>Could not create the destination directory</b></p>"
                   "<p>Please make sure you have the right permissions "
                   "and try again, or specify a different location.</p></body></html>"));
            msgBox.exec();
            return false;
        }
    }

    const QFileInfo info(path);

    if (!info.isWritable())
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Insufficient permissions"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(
            tr("<html><head/><body><p><b>Could not write to the destination directory</b></p>"
               "<p>Please make sure you have the right permissions "
               "and try again, or specify a different location.</p></body></html>"));
        msgBox.exec();
        return false;
    }

    if (mWizard->findFiles(QLatin1String("Morrowind"), path))
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Destination not empty"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(
            tr("<html><head/><body><p><b>The destination directory is not empty</b></p>"
               "<p>An existing Morrowind installation is present in the specified location.</p>"
               "<p>Please specify a different location, or go back and select the location as an existing "
               "installation.</p></body></html>"));
        msgBox.exec();
        return false;
    }

    return true;
}

void Wizard::InstallationTargetPage::browseButtonClicked()
{
    const QString selectedPath = QFileDialog::getExistingDirectory(this, tr("Select where to install Morrowind"),
        QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    qDebug() << selectedPath;
    const QFileInfo info(selectedPath);
    if (info.exists() && info.isWritable())
        targetLineEdit->setText(info.absoluteFilePath());
}

int Wizard::InstallationTargetPage::nextId() const
{
    return MainWizard::Page_LanguageSelection;
}
