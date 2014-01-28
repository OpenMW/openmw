#include "installationtargetpage.hpp"

#include <QDebug>
#include <QFileDialog>

#include "mainwizard.hpp"

Wizard::InstallationTargetPage::InstallationTargetPage(MainWizard *wizard, const Files::ConfigurationManager &cfg) :
    QWizardPage(wizard),
    mWizard(wizard),
    mCfgMgr(cfg)
{
    setupUi(this);

    registerField(QLatin1String("installation.path*"), targetLineEdit);
}

void Wizard::InstallationTargetPage::initializePage()
{
    QString path(QFile::decodeName(mCfgMgr.getUserDataPath().string().c_str()));
    path.append(QDir::separator() + QLatin1String("data"));

    if (!QFile::exists(path)) {
        QDir dir;
        dir.mkpath(path);
    }

    QDir dir(path);
    targetLineEdit->setText(QDir::toNativeSeparators(dir.absolutePath()));
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
