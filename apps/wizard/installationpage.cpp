#include "installationpage.hpp"

#include <QDebug>
#include <QTextCodec>
#include <QFileInfo>
#include <QFileDialog>

#include "mainwizard.hpp"
#include "inisettings.hpp"
#include "unshield/unshieldworker.hpp"

Wizard::InstallationPage::InstallationPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    mFinished = false;
}

void Wizard::InstallationPage::initializePage()
{
    QString path(field("installation.path").toString());
    QStringList components(field("installation.components").toStringList());

    logTextEdit->append(QString("Installing to %1").arg(path));
    logTextEdit->append(QString("Installing %1.").arg(components.join(", ")));

    installProgressBar->setMinimum(0);

    // Set the progressbar maximum to a multiple of 100
    // That way installing all three components would yield 300%
    // When one component is done the bar will be filled by 33%

    if (field("installation.new").toBool() == true)
    {
        installProgressBar->setMaximum((components.count() * 100));
    }
    else
    {
        if (components.contains(QLatin1String("Tribunal"))
                && mWizard->mInstallations[path]->hasTribunal == false)
            installProgressBar->setMaximum(100);

        if (components.contains(QLatin1String("Bloodmoon"))
                && mWizard->mInstallations[path]->hasBloodmoon == false)
            installProgressBar->setMaximum(installProgressBar->maximum() + 100);
    }

    startInstallation();
}

void Wizard::InstallationPage::startInstallation()
{
    QStringList components(field("installation.components").toStringList());
    QString path(field("installation.path").toString());

    QThread *thread = new QThread();
    mUnshield = new UnshieldWorker();

    mUnshield->moveToThread(thread);

    connect(thread, SIGNAL(started()),
            mUnshield, SLOT(extract()));

    connect(mUnshield, SIGNAL(finished()),
            thread, SLOT(quit()));

    connect(mUnshield, SIGNAL(finished()),
            mUnshield, SLOT(deleteLater()));

    connect(mUnshield, SIGNAL(finished()),
            thread, SLOT(deleteLater()));

    connect(mUnshield, SIGNAL(finished()),
            this, SLOT(installationFinished()), Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(error(QString)),
            this, SLOT(installationError(QString)), Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(textChanged(QString)),
            installProgressLabel, SLOT(setText(QString)), Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(textChanged(QString)),
            logTextEdit, SLOT(append(QString)),  Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(progressChanged(int)),
            installProgressBar, SLOT(setValue(int)),  Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(requestFileDialog(QString)),
            this, SLOT(showFileDialog(QString)), Qt::QueuedConnection);

    if (field("installation.new").toBool() == true)
    {
        // Always install Morrowind
        mUnshield->setInstallMorrowind(true);

        if (components.contains(QLatin1String("Tribunal")))
            mUnshield->setInstallTribunal(true);

        if (components.contains(QLatin1String("Bloodmoon")))
            mUnshield->setInstallBloodmoon(true);
    } else {
        // Morrowind should already be installed
        mUnshield->setInstallMorrowind(false);

        if (components.contains(QLatin1String("Tribunal"))
                && mWizard->mInstallations[path]->hasTribunal == false)
            mUnshield->setInstallTribunal(true);

        if (components.contains(QLatin1String("Bloodmoon"))
                && mWizard->mInstallations[path]->hasBloodmoon == false)
            mUnshield->setInstallBloodmoon(true);

        // Set the location of the Morrowind.ini to update
        mUnshield->setIniPath(mWizard->mInstallations[path]->iniPath);
    }

    // Set the installation target path
    mUnshield->setPath(path);

    // Set the right codec to use for Morrowind.ini
    QString language(field("installation.language").toString());

    if (language == QLatin1String("Polish")) {
        mUnshield->setIniCodec(QTextCodec::codecForName("windows-1250"));
    }
    else if (language == QLatin1String("Russian")) {
        mUnshield->setIniCodec(QTextCodec::codecForName("windows-1251"));
    }
    else {
        mUnshield->setIniCodec(QTextCodec::codecForName("windows-1252"));
    }

    thread->start();
}



//void Wizard::InstallationPage::installAddons()
//{
//    qDebug() << "component finished";

//    QStringList components(field("installation.components").toStringList());

//    if (components.contains(QLatin1String("Tribunal")) && !mUnshield->tribunalDone())
//    {
//        QString fileName = QFileDialog::getOpenFileName(
//                    this,
//                    tr("Select Tribunal installation file"),
//                    QDir::rootPath(),
//                    tr("InstallShield header files (*.hdr)"));

//        if (fileName.isEmpty()) {
//            qDebug() << "Cancel was clicked!";
//            return;
//        }

//        QFileInfo info(fileName);
//        mUnshield->installTribunal(info.absolutePath());
//    }

//    if (components.contains(QLatin1String("Bloodmoon")) && !mUnshield->bloodmoonDone())
//    {
//        QString fileName = QFileDialog::getOpenFileName(
//                    this,
//                    tr("Select Bloodmoon installation file"),
//                    QDir::rootPath(),
//                    tr("InstallShield header files (*.hdr)"));

//        if (fileName.isEmpty()) {
//            qDebug() << "Cancel was clicked!";
//            return;
//        }

//        QFileInfo info(fileName);
//        mUnshield->installBloodmoon(info.absolutePath());
//    }
//}

void Wizard::InstallationPage::showFileDialog(const QString &component)
{
    QString fileName;

    if (field("installation.new").toBool() == true)
    {
        fileName = QFileDialog::getOpenFileName(
                    this,
                    tr("Select %0 installation file").arg(component),
                    QDir::rootPath(),
                    tr("InstallShield header files (*.hdr)"));

        if (fileName.isEmpty()) {
            qDebug() << "Cancel was clicked!";
            return;
        }

        QFileInfo info(fileName);

        if (component == QLatin1String("Morrowind"))
        {
            mUnshield->setMorrowindPath(info.absolutePath());
        }
        else if (component == QLatin1String("Tribunal"))
        {
            mUnshield->setTribunalPath(info.absolutePath());
        }
        else if (component == QLatin1String("Bloodmoon"))
        {
            mUnshield->setBloodmoonPath(info.absolutePath());
        }
    }
}

void Wizard::InstallationPage::installationFinished()
{
    qDebug() << "finished!";
    mFinished = true;
    emit completeChanged();

}

void Wizard::InstallationPage::installationError(const QString &text)
{
    qDebug() << "error: " << text;
}

bool Wizard::InstallationPage::isComplete() const
{
    return mFinished;
}

int Wizard::InstallationPage::nextId() const
{
    return MainWizard::Page_Import;
}
