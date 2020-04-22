#include "installationpage.hpp"

#include <QDebug>
#include <QTextCodec>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

#include "mainwizard.hpp"

Wizard::InstallationPage::InstallationPage(QWidget *parent) :
    QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    mFinished = false;

    mThread = new QThread();
    mUnshield = new UnshieldWorker();
    mUnshield->moveToThread(mThread);

    connect(mThread, SIGNAL(started()),
            mUnshield, SLOT(extract()));

    connect(mUnshield, SIGNAL(finished()),
            mThread, SLOT(quit()));

    connect(mUnshield, SIGNAL(finished()),
            this, SLOT(installationFinished()), Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(error(QString, QString)),
            this, SLOT(installationError(QString, QString)), Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(textChanged(QString)),
            installProgressLabel, SLOT(setText(QString)), Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(textChanged(QString)),
            logTextEdit, SLOT(appendPlainText(QString)),  Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(textChanged(QString)),
            mWizard, SLOT(addLogText(QString)),  Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(progressChanged(int)),
            installProgressBar, SLOT(setValue(int)),  Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(requestFileDialog(Wizard::Component)),
            this, SLOT(showFileDialog(Wizard::Component)), Qt::QueuedConnection);
}

Wizard::InstallationPage::~InstallationPage()
{
    if (mThread->isRunning()) {
        mUnshield->stopWorker();
        mThread->quit();
        mThread->wait();
    }

    delete mUnshield;
    delete mThread;
}

void Wizard::InstallationPage::initializePage()
{
    QString path(field(QLatin1String("installation.path")).toString());
    QStringList components(field(QLatin1String("installation.components")).toStringList());

    logTextEdit->appendPlainText(QString("Installing to %1").arg(path));
    logTextEdit->appendPlainText(QString("Installing %1.").arg(components.join(", ")));

    installProgressBar->setMinimum(0);

    // Set the progressbar maximum to a multiple of 100
    // That way installing all three components would yield 300%
    // When one component is done the bar will be filled by 33%

    if (field(QLatin1String("installation.retailDisc")).toBool() == true) {
        installProgressBar->setMaximum((components.count() * 100));
    } else {
        if (components.contains(QLatin1String("Tribunal"))
                && !mWizard->mInstallations[path].hasTribunal)
            installProgressBar->setMaximum(100);

        if (components.contains(QLatin1String("Bloodmoon"))
                && !mWizard->mInstallations[path].hasBloodmoon)
            installProgressBar->setMaximum(installProgressBar->maximum() + 100);
    }

    startInstallation();
}

void Wizard::InstallationPage::startInstallation()
{
    QStringList components(field(QLatin1String("installation.components")).toStringList());
    QString path(field(QLatin1String("installation.path")).toString());

    if (field(QLatin1String("installation.retailDisc")).toBool() == true)
    {
        // Always install Morrowind
        mUnshield->setInstallComponent(Wizard::Component_Morrowind, true);

        if (components.contains(QLatin1String("Tribunal")))
            mUnshield->setInstallComponent(Wizard::Component_Tribunal, true);

        if (components.contains(QLatin1String("Bloodmoon")))
            mUnshield->setInstallComponent(Wizard::Component_Bloodmoon, true);
    } else {
        // Morrowind should already be installed
        mUnshield->setInstallComponent(Wizard::Component_Morrowind, false);

        if (components.contains(QLatin1String("Tribunal"))
                && !mWizard->mInstallations[path].hasTribunal)
            mUnshield->setInstallComponent(Wizard::Component_Tribunal, true);

        if (components.contains(QLatin1String("Bloodmoon"))
                && !mWizard->mInstallations[path].hasBloodmoon)
            mUnshield->setInstallComponent(Wizard::Component_Bloodmoon, true);

        // Set the location of the Morrowind.ini to update
        mUnshield->setIniPath(mWizard->mInstallations[path].iniPath);
        mUnshield->setupSettings();
    }

    // Set the installation target path
    mUnshield->setPath(path);

    // Set the right codec to use for Morrowind.ini
    QString language(field(QLatin1String("installation.language")).toString());

    if (language == QLatin1String("Polish")) {
        mUnshield->setIniCodec(QTextCodec::codecForName("windows-1250"));
    } else if (language == QLatin1String("Russian")) {
        mUnshield->setIniCodec(QTextCodec::codecForName("windows-1251"));
    }  else {
        mUnshield->setIniCodec(QTextCodec::codecForName("windows-1252"));
    }

    mThread->start();
}

void Wizard::InstallationPage::showFileDialog(Wizard::Component component)
{
    QString name;
    switch (component) {

    case Wizard::Component_Morrowind:
        name = QLatin1String("Morrowind");
        break;
    case Wizard::Component_Tribunal:
        name = QLatin1String("Tribunal");
        break;
    case Wizard::Component_Bloodmoon:
        name = QLatin1String("Bloodmoon");
        break;
    }
    logTextEdit->appendHtml(tr("<p>Attempting to install component %1.</p>").arg(name));
    mWizard->addLogText(tr("Attempting to install component %1.").arg(name));

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("%1 Installation").arg(name));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(QObject::tr("Select a valid %1 installation media.<br><b>Hint</b>: make sure that it contains at least one <b>.cab</b> file.").arg(name));
    msgBox.exec();

    QString path = QFileDialog::getExistingDirectory(this,
                                                     tr("Select %1 installation media").arg(name),
                                                     QDir::rootPath());

    if (path.isEmpty()) {
        logTextEdit->appendHtml(tr("<p><br/><span style=\"color:red;\"> \
                                    <b>Error: The installation was aborted by the user</b></span></p>"));

        mWizard->addLogText(QLatin1String("Error: The installation was aborted by the user"));
        mWizard->mError = true;

        emit completeChanged();
        return;
    }

    mUnshield->setDiskPath(path);
}

void Wizard::InstallationPage::installationFinished()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Installation finished"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(tr("Installation completed successfully!"));

    msgBox.exec();

    mFinished = true;
    emit completeChanged();
}

void Wizard::InstallationPage::installationError(const QString &text, const QString &details)
{
    installProgressLabel->setText(tr("Installation failed!"));

    logTextEdit->appendHtml(tr("<p><br/><span style=\"color:red;\"> \
                               <b>Error: %1</b></p>").arg(text));
    logTextEdit->appendHtml(tr("<p><span style=\"color:red;\"> \
                               <b>%1</b></p>").arg(details));

    mWizard->addLogText(QLatin1String("Error: ") + text);
    mWizard->addLogText(details);

    mWizard->mError = true;
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("An error occurred"));
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(tr("<html><head/><body><p><b>The Wizard has encountered an error</b></p> \
                      <p>The error reported was:</p><p>%1</p> \
                      <p>Press &quot;Show Details...&quot; for more information.</p></body></html>").arg(text));

    msgBox.setDetailedText(details);
    msgBox.exec();


    emit completeChanged();
}

bool Wizard::InstallationPage::isComplete() const
{
    if (!mWizard->mError) {
        return mFinished;
    } else {
        return true;
    }
}

int Wizard::InstallationPage::nextId() const
{
    if (field(QLatin1String("installation.retailDisc")).toBool() == true) {
        return MainWizard::Page_Conclusion;
    } else {
        if (!mWizard->mError) {
            return MainWizard::Page_Import;
        } else {
            return MainWizard::Page_Conclusion;
        }
    }
}
