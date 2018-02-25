#include "installationtargetpage.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#include "mainwizard.hpp"

Wizard::InstallationTargetPage::InstallationTargetPage(QWidget *parent, const Files::ConfigurationManager &cfg) :
    QWizardPage(parent),
    mCfgMgr(cfg)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    registerField(QLatin1String("installation.path*"), targetLineEdit);
}

void Wizard::InstallationTargetPage::initializePage()
{
    QString path(QFile::decodeName(mCfgMgr.getUserDataPath().string().c_str()));
    path.append(QDir::separator() + QLatin1String("basedata"));

    QDir dir(path);
    targetLineEdit->setText(QDir::toNativeSeparators(dir.absolutePath()));
}

bool Wizard::InstallationTargetPage::validatePage()
{
    QString path(field(QLatin1String("installation.path")).toString());

    qDebug() << "Validating path: " << path;

    if (!QFile::exists(path)) {
        QDir dir;

        if (!dir.mkpath(path)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error creating destination"));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<html><head/><body><p><b>Could not create the destination directory</b></p> \
                              <p>Please make sure you have the right permissions \
                              and try again, or specify a different location.</p></body></html>"));
            msgBox.exec();
            return false;
        }
    }

    QFileInfo info(path);

    if (!info.isWritable()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Insufficient permissions"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<html><head/><body><p><b>Could not write to the destination directory</b></p> \
                          <p>Please make sure you have the right permissions \
                          and try again, or specify a different location.</p></body></html>"));
        msgBox.exec();
        return false;
    }

    if (mWizard->findFiles(QLatin1String("Morrowind"), path)) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Destination not empty"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<html><head/><body><p><b>The destination directory is not empty</b></p> \
                          <p>An existing Morrowind installation is present in the specified location.</p> \
                          <p>Please specify a different location, or go back and select the location as an existing installation.</p></body></html>"));
        msgBox.exec();
        return false;
    }

    return true;
}

void Wizard::InstallationTargetPage::on_browseButton_clicked()
{
    QString selectedPath = QFileDialog::getExistingDirectory(
                this,
                tr("Select where to install Morrowind"),
                QDir::homePath(),
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
    return MainWizard::Page_LanguageSelection;
}
