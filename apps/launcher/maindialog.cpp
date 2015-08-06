#include "maindialog.hpp"

#include <components/version/version.hpp>

#include <QLabel>
#include <QDate>
#include <QTime>
#include <QMessageBox>
#include <QPushButton>
#include <QFontDatabase>
#include <QInputDialog>
#include <QFileDialog>
#include <QCloseEvent>
#include <QTextCodec>
#include <QFile>
#include <QDir>

#include <QDebug>

#include "playpage.hpp"
#include "graphicspage.hpp"
#include "datafilespage.hpp"
#include "settingspage.hpp"

using namespace Process;

Launcher::MainDialog::MainDialog(QWidget *parent)
    : QMainWindow(parent), mGameSettings (mCfgMgr)
{
    setupUi(this);

    mGameInvoker = new ProcessInvoker();
    mWizardInvoker = new ProcessInvoker();

    connect(mWizardInvoker->getProcess(), SIGNAL(started()),
            this, SLOT(wizardStarted()));

    connect(mWizardInvoker->getProcess(), SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(wizardFinished(int,QProcess::ExitStatus)));

    iconWidget->setViewMode(QListView::IconMode);
    iconWidget->setWrapping(false);
    iconWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Just to be sure
    iconWidget->setIconSize(QSize(48, 48));
    iconWidget->setMovement(QListView::Static);

    iconWidget->setSpacing(4);
    iconWidget->setCurrentRow(0);
    iconWidget->setFlow(QListView::LeftToRight);

    QPushButton *playButton = new QPushButton(tr("Play"));
    buttonBox->addButton(playButton, QDialogButtonBox::AcceptRole);

    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(play()));

    // Remove what's this? button
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    createIcons();
}

Launcher::MainDialog::~MainDialog()
{
    delete mGameInvoker;
    delete mWizardInvoker;
}

void Launcher::MainDialog::createIcons()
{
    if (!QIcon::hasThemeIcon("document-new"))
        QIcon::setThemeName("tango");

    QListWidgetItem *playButton = new QListWidgetItem(iconWidget);
    playButton->setIcon(QIcon(":/images/openmw.png"));
    playButton->setText(tr("Play"));
    playButton->setTextAlignment(Qt::AlignCenter);
    playButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *dataFilesButton = new QListWidgetItem(iconWidget);
    dataFilesButton->setIcon(QIcon(":/images/openmw-plugin.png"));
    dataFilesButton->setText(tr("Data Files"));
    dataFilesButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    dataFilesButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *graphicsButton = new QListWidgetItem(iconWidget);
    graphicsButton->setIcon(QIcon::fromTheme("video-display"));
    graphicsButton->setText(tr("Graphics"));
    graphicsButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom | Qt::AlignAbsolute);
    graphicsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *settingsButton = new QListWidgetItem(iconWidget);
    settingsButton->setIcon(QIcon::fromTheme("preferences-system"));
    settingsButton->setText(tr("Settings"));
    settingsButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    settingsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(iconWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

}

void Launcher::MainDialog::createPages()
{
    mPlayPage = new PlayPage(this);
    mDataFilesPage = new DataFilesPage(mCfgMgr, mGameSettings, mLauncherSettings, this);
    mGraphicsPage = new GraphicsPage(mCfgMgr, mGraphicsSettings, this);
    mSettingsPage = new SettingsPage(mCfgMgr, mGameSettings, mLauncherSettings, this);

    // Set the combobox of the play page to imitate the combobox on the datafilespage
    mPlayPage->setProfilesModel(mDataFilesPage->profilesModel());
    mPlayPage->setProfilesIndex(mDataFilesPage->profilesIndex());

    // Add the pages to the stacked widget
    pagesWidget->addWidget(mPlayPage);
    pagesWidget->addWidget(mDataFilesPage);
    pagesWidget->addWidget(mGraphicsPage);
    pagesWidget->addWidget(mSettingsPage);

    // Select the first page
    iconWidget->setCurrentItem(iconWidget->item(0), QItemSelectionModel::Select);

    connect(mPlayPage, SIGNAL(playButtonClicked()), this, SLOT(play()));

    connect(mPlayPage, SIGNAL(signalProfileChanged(int)), mDataFilesPage, SLOT(slotProfileChanged(int)));
    connect(mDataFilesPage, SIGNAL(signalProfileChanged(int)), mPlayPage, SLOT(setProfilesIndex(int)));

}

Launcher::FirstRunDialogResult Launcher::MainDialog::showFirstRunDialog()
{
    if (!setupLauncherSettings())
        return FirstRunDialogResultFailure;

    if (mLauncherSettings.value(QString("General/firstrun"), QString("true")) == QLatin1String("true"))
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("First run"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::NoButton);
        msgBox.setText(tr("<html><head/><body><p><b>Welcome to OpenMW!</b></p> \
                          <p>It is recommended to run the Installation Wizard.</p> \
                          <p>The Wizard will let you select an existing Morrowind installation, \
                          or install Morrowind for OpenMW to use.</p></body></html>"));

        QAbstractButton *wizardButton =
                msgBox.addButton(tr("Run &Installation Wizard"), QMessageBox::AcceptRole); // ActionRole doesn't work?!
        QAbstractButton *skipButton =
                msgBox.addButton(tr("Skip"), QMessageBox::RejectRole);

        Q_UNUSED(skipButton); // Surpress compiler unused warning

        msgBox.exec();

        if (msgBox.clickedButton() == wizardButton)
        {
            if (!mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false)) {
                return FirstRunDialogResultFailure;
            } else {
                return FirstRunDialogResultWizard;
            }
        }
    }

    return setup() ? FirstRunDialogResultContinue : FirstRunDialogResultFailure;
}

void Launcher::MainDialog::setVersionLabel()
{
    // Add version information to bottom of the window
    Version::Version v = Version::getOpenmwVersion(mGameSettings.value("resources").toUtf8().constData());

    QString revision(QString::fromUtf8(v.mCommitHash.c_str()));
    QString tag(QString::fromUtf8(v.mTagHash.c_str()));

    versionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    if (!v.mVersion.empty() && (revision.isEmpty() || revision == tag))
        versionLabel->setText(tr("OpenMW %1 release").arg(QString::fromUtf8(v.mVersion.c_str())));
    else
        versionLabel->setText(tr("OpenMW development (%1)").arg(revision.left(10)));

    // Add the compile date and time
    versionLabel->setToolTip(tr("Compiled on %1 %2").arg(QLocale(QLocale::C).toDate(QString(__DATE__).simplified(),
                                                                                    QLatin1String("MMM d yyyy")).toString(Qt::SystemLocaleLongDate),
                                                         QLocale(QLocale::C).toTime(QString(__TIME__).simplified(),
                                                                                    QLatin1String("hh:mm:ss")).toString(Qt::SystemLocaleShortDate)));
}

bool Launcher::MainDialog::setup()
{
    if (!setupGameSettings())
        return false;

    setVersionLabel();

    mLauncherSettings.setContentList(mGameSettings);

    if (!setupGraphicsSettings())
        return false;

    // Now create the pages as they need the settings
    createPages();

    // Call this so we can exit on SDL errors before mainwindow is shown
    if (!mGraphicsPage->loadSettings())
        return false;

    loadSettings();

    return true;
}

bool Launcher::MainDialog::reloadSettings()
{
    if (!setupLauncherSettings())
        return false;

    if (!setupGameSettings())
        return false;

    mLauncherSettings.setContentList(mGameSettings);

    if (!setupGraphicsSettings())
        return false;

    if (!mSettingsPage->loadSettings())
        return false;

    if (!mDataFilesPage->loadSettings())
        return false;

    if (!mGraphicsPage->loadSettings())
        return false;

    return true;
}

void Launcher::MainDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    int currentIndex = iconWidget->row(current);
    pagesWidget->setCurrentIndex(currentIndex);
    mSettingsPage->resetProgressBar();
}

bool Launcher::MainDialog::setupLauncherSettings()
{
    mLauncherSettings.setMultiValueEnabled(true);

    QString userPath = QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str());

    QStringList paths;
    paths.append(QString(Config::LauncherSettings::sLauncherConfigFileName));
    paths.append(userPath + QString(Config::LauncherSettings::sLauncherConfigFileName));

    foreach (const QString &path, paths) {
        qDebug() << "Loading config file:" << qPrintable(path);
        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(tr("<br><b>Could not open %0 for reading</b><br><br> \
                                           Please make sure you have the right permissions \
                                           and try again.<br>").arg(file.fileName()));
                                           msgBox.exec();
                return false;
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mLauncherSettings.readFile(stream);
        }
        file.close();
    }

    return true;
}

bool Launcher::MainDialog::setupGameSettings()
{
    QString userPath = QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str());
    QString globalPath = QString::fromUtf8(mCfgMgr.getGlobalPath().string().c_str());

    // Load the user config file first, separately
    // So we can write it properly, uncontaminated
    QString path = userPath + QLatin1String("openmw.cfg");
    QFile file(path);

    qDebug() << "Loading config file:" << qPrintable(path);

    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<br><b>Could not open %0 for reading</b><br><br> \
                                       Please make sure you have the right permissions \
                                       and try again.<br>").arg(file.fileName()));
                                       msgBox.exec();
            return false;
        }
        QTextStream stream(&file);
        stream.setCodec(QTextCodec::codecForName("UTF-8"));

        mGameSettings.readUserFile(stream);
    }

    // Now the rest - priority: user > local > global
    QStringList paths;
    paths.append(globalPath + QString("openmw.cfg"));
    paths.append(QString("openmw.cfg"));
    paths.append(userPath + QString("openmw.cfg"));

    foreach (const QString &path, paths) {
        qDebug() << "Loading config file:" << qPrintable(path);

        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(tr("<br><b>Could not open %0 for reading</b><br><br> \
                                           Please make sure you have the right permissions \
                                           and try again.<br>").arg(file.fileName()));
                                           msgBox.exec();
                return false;
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mGameSettings.readFile(stream);
        }
        file.close();
    }

    QStringList dataDirs;

    // Check if the paths actually contain data files
    foreach (const QString path, mGameSettings.getDataDirs()) {
        QDir dir(path);
        QStringList filters;
        filters << "*.esp" << "*.esm" << "*.omwgame" << "*.omwaddon";

        if (!dir.entryList(filters).isEmpty())
            dataDirs.append(path);
    }

    if (dataDirs.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error detecting Morrowind installation"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setText(tr("<br><b>Could not find the Data Files location</b><br><br> \
                                   The directory containing the data files was not found."));

        QAbstractButton *wizardButton =
                msgBox.addButton(tr("Run &Installation Wizard..."), QMessageBox::ActionRole);

        msgBox.exec();

        if (msgBox.clickedButton() == wizardButton)
        {
            if (!mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false)) {
                return false;
            } else {
                return true;
            }
        }
    }

    return true;
}

bool Launcher::MainDialog::setupGraphicsSettings()
{
    mGraphicsSettings.setMultiValueEnabled(false);

    QString userPath = QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str());
    QString globalPath = QString::fromUtf8(mCfgMgr.getGlobalPath().string().c_str());

    QFile localDefault(QString("settings-default.cfg"));
    QFile globalDefault(globalPath + QString("settings-default.cfg"));

    if (!localDefault.exists() && !globalDefault.exists()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error reading OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not find settings-default.cfg</b><br><br> \
                                   The problem may be due to an incomplete installation of OpenMW.<br> \
                                   Reinstalling OpenMW may resolve the problem."));
                                   msgBox.exec();
        return false;
    }


    QStringList paths;
    paths.append(globalPath + QString("settings-default.cfg"));
    paths.append(QString("settings-default.cfg"));
    paths.append(userPath + QString("settings.cfg"));

    foreach (const QString &path, paths) {
        qDebug() << "Loading config file:" << qPrintable(path);
        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Error opening OpenMW configuration file"));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setText(tr("<br><b>Could not open %0 for reading</b><br><br> \
                                           Please make sure you have the right permissions \
                                           and try again.<br>").arg(file.fileName()));
                                           msgBox.exec();
                return false;
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mGraphicsSettings.readFile(stream);
        }
        file.close();
    }

    return true;
}

void Launcher::MainDialog::loadSettings()
{
    int width = mLauncherSettings.value(QString("General/MainWindow/width")).toInt();
    int height = mLauncherSettings.value(QString("General/MainWindow/height")).toInt();

    int posX = mLauncherSettings.value(QString("General/MainWindow/posx")).toInt();
    int posY = mLauncherSettings.value(QString("General/MainWindow/posy")).toInt();

    resize(width, height);
    move(posX, posY);
}

void Launcher::MainDialog::saveSettings()
{
    QString width = QString::number(this->width());
    QString height = QString::number(this->height());

    mLauncherSettings.setValue(QString("General/MainWindow/width"), width);
    mLauncherSettings.setValue(QString("General/MainWindow/height"), height);

    QString posX = QString::number(this->pos().x());
    QString posY = QString::number(this->pos().y());

    mLauncherSettings.setValue(QString("General/MainWindow/posx"), posX);
    mLauncherSettings.setValue(QString("General/MainWindow/posy"), posY);

    mLauncherSettings.setValue(QString("General/firstrun"), QString("false"));

}

bool Launcher::MainDialog::writeSettings()
{
    // Now write all config files
    saveSettings();
    mDataFilesPage->saveSettings();
    mGraphicsPage->saveSettings();
    mSettingsPage->saveSettings();

    QString userPath = QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str());
    QDir dir(userPath);

    if (!dir.exists()) {
        if (!dir.mkpath(userPath)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error creating OpenMW configuration directory"));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<br><b>Could not create %0</b><br><br> \
                              Please make sure you have the right permissions \
                              and try again.<br>").arg(userPath));
                              msgBox.exec();
                           return false;
        }
    }

    // Game settings
    QFile file(userPath + QString("openmw.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                          Please make sure you have the right permissions \
                          and try again.<br>").arg(file.fileName()));
                          msgBox.exec();
                       return false;
    }


    mGameSettings.writeFileWithComments(file);
    file.close();

    // Graphics settings
    file.setFileName(userPath + QString("settings.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing OpenMW configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                          Please make sure you have the right permissions \
                          and try again.<br>").arg(file.fileName()));
                          msgBox.exec();
                       return false;
    }

    QTextStream stream(&file);
    stream.setDevice(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    mGraphicsSettings.writeFile(stream);
    file.close();

    // Launcher settings
    file.setFileName(userPath + QString(Config::LauncherSettings::sLauncherConfigFileName));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error writing Launcher configuration file"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                          Please make sure you have the right permissions \
                          and try again.<br>").arg(file.fileName()));
                          msgBox.exec();
                       return false;
    }

    stream.setDevice(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    mLauncherSettings.writeFile(stream);
    file.close();

    return true;
}

void Launcher::MainDialog::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

void Launcher::MainDialog::wizardStarted()
{
    hide();
}

void Launcher::MainDialog::wizardFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus == QProcess::CrashExit)
        return qApp->quit();

    // HACK: Ensure the pages are created, else segfault
    setup();

    if (reloadSettings())
        show();
}

void Launcher::MainDialog::play()
{
    if (!writeSettings())
        return qApp->quit();

    if (!mGameSettings.hasMaster()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("No game file selected"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>You do not have a game file selected.</b><br><br> \
                          OpenMW will not start without a game file selected.<br>"));
                          msgBox.exec();
        return;
    }

    // Launch the game detached

    if (mGameInvoker->startProcess(QLatin1String("openmw"), true))
        return qApp->quit();
}
