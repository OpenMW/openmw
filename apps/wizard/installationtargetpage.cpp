#include "installationtargetpage.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QDesktopServices>

#include "mainwizard.hpp"

Wizard::InstallationTargetPage::InstallationTargetPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    registerField(QLatin1String("installation.path*"), targetLineEdit);
}

void Wizard::InstallationTargetPage::initializePage()
{
    qDebug() << mWizard->field("installation.language");

#ifdef Q_OS_WIN
    QString path = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif

#ifdef Q_OS_MAC
    QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
    QString path = QFile::decodeName(qgetenv("XDG_DATA_HOME"));

    if (path.isEmpty())
        path = QDir::homePath() + QLatin1String("/.local/share");
#endif

    path.append(QLatin1String("/openmw/data"));

    if (!QFile::exists(path)) {
        QDir dir;
        dir.mkpath(path);
    }

    QDir dir(path);
    targetLineEdit->setText(QDir::toNativeSeparators(dir.absolutePath()));

    qDebug() << path;
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
