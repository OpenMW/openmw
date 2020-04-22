#include "maindialog.hpp"

#include <components/version/version.hpp>
#include <components/misc/helpviewer.hpp>

#include <QDate>
#include <QMessageBox>
#include <QPushButton>
#include <QFontDatabase>
#include <QInputDialog>
#include <QFileDialog>
#include <QCloseEvent>
#include <QTextCodec>

#include <QDebug>

#include "playpage.hpp"
#include "graphicspage.hpp"
#include "datafilespage.hpp"
#include "settingspage.hpp"
#include "advancedpage.hpp"

using namespace Process;

void cfgError(const QString& title, const QString& msg) {
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(msg);
    msgBox.exec();
}

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

    QPushButton *helpButton = new QPushButton(tr("Help"));
    QPushButton *playButton = new QPushButton(tr("Play"));
    buttonBox->button(QDialogButtonBox::Close)->setText(tr("Close"));
    buttonBox->addButton(helpButton, QDialogButtonBox::HelpRole);
    buttonBox->addButton(playButton, QDialogButtonBox::AcceptRole);

    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(play()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(help()));

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
    graphicsButton->setIcon(QIcon(":/images/preferences-video.png"));
    graphicsButton->setText(tr("Graphics"));
    graphicsButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom | Qt::AlignAbsolute);
    graphicsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *settingsButton = new QListWidgetItem(iconWidget);
    settingsButton->setIcon(QIcon(":/images/preferences.png"));
    settingsButton->setText(tr("Settings"));
    settingsButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    settingsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *advancedButton = new QListWidgetItem(iconWidget);
    advancedButton->setIcon(QIcon(":/images/preferences-advanced.png"));
    advancedButton->setText(tr("Advanced"));
    advancedButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    advancedButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(iconWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

}

void Launcher::MainDialog::createPages()
{
    // Avoid creating the widgets twice
    if (pagesWidget->count() != 0)
        return;

    mPlayPage = new PlayPage(this);
    mDataFilesPage = new DataFilesPage(mCfgMgr, mGameSettings, mLauncherSettings, this);
    mGraphicsPage = new GraphicsPage(mCfgMgr, mEngineSettings, this);
    mSettingsPage = new SettingsPage(mCfgMgr, mGameSettings, mLauncherSettings, this);
    mAdvancedPage = new AdvancedPage(mCfgMgr, mGameSettings, mEngineSettings, this);

    // Set the combobox of the play page to imitate the combobox on the datafilespage
    mPlayPage->setProfilesModel(mDataFilesPage->profilesModel());
    mPlayPage->setProfilesIndex(mDataFilesPage->profilesIndex());

    // Add the pages to the stacked widget
    pagesWidget->addWidget(mPlayPage);
    pagesWidget->addWidget(mDataFilesPage);
    pagesWidget->addWidget(mGraphicsPage);
    pagesWidget->addWidget(mSettingsPage);
    pagesWidget->addWidget(mAdvancedPage);

    // Select the first page
    iconWidget->setCurrentItem(iconWidget->item(0), QItemSelectionModel::Select);

    connect(mPlayPage, SIGNAL(playButtonClicked()), this, SLOT(play()));

    connect(mPlayPage, SIGNAL(signalProfileChanged(int)), mDataFilesPage, SLOT(slotProfileChanged(int)));
    connect(mDataFilesPage, SIGNAL(signalProfileChanged(int)), mPlayPage, SLOT(setProfilesIndex(int)));
    // Using Qt::QueuedConnection because signal is emitted in a subthread and slot is in the main thread
    connect(mDataFilesPage, SIGNAL(signalLoadedCellsChanged(QStringList)), mAdvancedPage, SLOT(slotLoadedCellsChanged(QStringList)), Qt::QueuedConnection);

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

        msgBox.exec();

        if (msgBox.clickedButton() == wizardButton)
        {
            if (mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false))
                return FirstRunDialogResultWizard;
        }
        else if (msgBox.clickedButton() == skipButton)
        {
            // Don't bother setting up absent game data.
            if (setup())
                return FirstRunDialogResultContinue;
        }
        return FirstRunDialogResultFailure;
    }

    if (!setup() || !setupGameData()) {
        return FirstRunDialogResultFailure;
    }
    return FirstRunDialogResultContinue;
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

    if (!mAdvancedPage->loadSettings())
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
    mLauncherSettings.clear();

    mLauncherSettings.setMultiValueEnabled(true);

    QString userPath = QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str());

    QStringList paths;
    paths.append(QString(Config::LauncherSettings::sLauncherConfigFileName));
    paths.append(userPath + QString(Config::LauncherSettings::sLauncherConfigFileName));

    for (const QString &path : paths)
    {
        qDebug() << "Loading config file:" << path.toUtf8().constData();
        QFile file(path);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                cfgError(tr("Error opening OpenMW configuration file"),
                         tr("<br><b>Could not open %0 for reading</b><br><br> \
                             Please make sure you have the right permissions \
                             and try again.<br>").arg(file.fileName()));
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
    mGameSettings.clear();

    QString localPath = QString::fromUtf8(mCfgMgr.getLocalPath().string().c_str());
    QString userPath = QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str());
    QString globalPath = QString::fromUtf8(mCfgMgr.getGlobalPath().string().c_str());

    // Load the user config file first, separately
    // So we can write it properly, uncontaminated
    QString path = userPath + QLatin1String("openmw.cfg");
    QFile file(path);

    qDebug() << "Loading config file:" << path.toUtf8().constData();

    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            cfgError(tr("Error opening OpenMW configuration file"),
                     tr("<br><b>Could not open %0 for reading</b><br><br> \
                         Please make sure you have the right permissions \
                         and try again.<br>").arg(file.fileName()));
            return false;
        }
        QTextStream stream(&file);
        stream.setCodec(QTextCodec::codecForName("UTF-8"));

        mGameSettings.readUserFile(stream);
        file.close();
    }

    // Now the rest - priority: user > local > global
    QStringList paths;
    paths.append(globalPath + QString("openmw.cfg"));
    paths.append(localPath + QString("openmw.cfg"));
    paths.append(userPath + QString("openmw.cfg"));

    for (const QString &path2 : paths)
    {
        qDebug() << "Loading config file:" << path2.toUtf8().constData();

        file.setFileName(path2);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                cfgError(tr("Error opening OpenMW configuration file"),
                         tr("<br><b>Could not open %0 for reading</b><br><br> \
                             Please make sure you have the right permissions \
                             and try again.<br>").arg(file.fileName()));
                return false;
            }
            QTextStream stream(&file);
            stream.setCodec(QTextCodec::codecForName("UTF-8"));

            mGameSettings.readFile(stream);
            file.close();
        }
    }

    return true;
}

bool Launcher::MainDialog::setupGameData()
{
    QStringList dataDirs;

    // Check if the paths actually contain data files
    for (const QString& path3 : mGameSettings.getDataDirs())
    {
        QDir dir(path3);
        QStringList filters;
        filters << "*.esp" << "*.esm" << "*.omwgame" << "*.omwaddon";

        if (!dir.entryList(filters).isEmpty())
            dataDirs.append(path3);
    }

    if (dataDirs.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error detecting Morrowind installation"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::NoButton);
        msgBox.setText(tr("<br><b>Could not find the Data Files location</b><br><br> \
                                   The directory containing the data files was not found."));

        QAbstractButton *wizardButton =
                msgBox.addButton(tr("Run &Installation Wizard..."), QMessageBox::ActionRole);
        QAbstractButton *skipButton =
                msgBox.addButton(tr("Skip"), QMessageBox::RejectRole);

        Q_UNUSED(skipButton); // Supress compiler unused warning

        msgBox.exec();

        if (msgBox.clickedButton() == wizardButton)
        {
            if (!mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false))
                return false;
        }
    }

    return true;
}

bool Launcher::MainDialog::setupGraphicsSettings()
{
    // This method is almost a copy of OMW::Engine::loadSettings().  They should definitely
    // remain consistent, and possibly be merged into a shared component.  At the very least
    // the filenames should be in the CfgMgr component.

    // Ensure to clear previous settings in case we had already loaded settings.
    mEngineSettings.clear();

    // Create the settings manager and load default settings file
    const std::string localDefault = (mCfgMgr.getLocalPath() / "settings-default.cfg").string();
    const std::string globalDefault = (mCfgMgr.getGlobalPath() / "settings-default.cfg").string();
    std::string defaultPath;

    // Prefer the settings-default.cfg in the current directory.
    if (boost::filesystem::exists(localDefault))
        defaultPath = localDefault;
    else if (boost::filesystem::exists(globalDefault))
        defaultPath = globalDefault;
    // Something's very wrong if we can't find the file at all.
    else {
        cfgError(tr("Error reading OpenMW configuration file"),
                 tr("<br><b>Could not find settings-default.cfg</b><br><br> \
                     The problem may be due to an incomplete installation of OpenMW.<br> \
                     Reinstalling OpenMW may resolve the problem."));
        return false;
    }

    // Load the default settings, report any parsing errors.
    try {
        mEngineSettings.loadDefault(defaultPath);
    }
    catch (std::exception& e) {
        std::string msg = std::string("<br><b>Error reading settings-default.cfg</b><br><br>") + e.what();
        cfgError(tr("Error reading OpenMW configuration file"), tr(msg.c_str()));
        return false;
    }

    // Load user settings if they exist
    const std::string userPath = (mCfgMgr.getUserConfigPath() / "settings.cfg").string();
    // User settings are not required to exist, so if they don't we're done.
    if (!boost::filesystem::exists(userPath)) return true;

    try {
        mEngineSettings.loadUser(userPath);
    }
    catch (std::exception& e) {
        std::string msg = std::string("<br><b>Error reading settings.cfg</b><br><br>") + e.what();
        cfgError(tr("Error reading OpenMW configuration file"), tr(msg.c_str()));
        return false;
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
    mAdvancedPage->saveSettings();

    QString userPath = QString::fromUtf8(mCfgMgr.getUserConfigPath().string().c_str());
    QDir dir(userPath);

    if (!dir.exists()) {
        if (!dir.mkpath(userPath)) {
            cfgError(tr("Error creating OpenMW configuration directory"),
                     tr("<br><b>Could not create %0</b><br><br> \
                         Please make sure you have the right permissions \
                         and try again.<br>").arg(userPath));
            return false;
        }
    }

    // Game settings
    QFile file(userPath + QString("openmw.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        // File cannot be opened or created
        cfgError(tr("Error writing OpenMW configuration file"),
                 tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                     Please make sure you have the right permissions \
                     and try again.<br>").arg(file.fileName()));
        return false;
    }


    mGameSettings.writeFileWithComments(file);
    file.close();

    // Graphics settings
    const std::string settingsPath = (mCfgMgr.getUserConfigPath() / "settings.cfg").string();
    try {
        mEngineSettings.saveUser(settingsPath);
    }
    catch (std::exception& e) {
        std::string msg = "<br><b>Error writing settings.cfg</b><br><br>" +
            settingsPath + "<br><br>" + e.what();
        cfgError(tr("Error writing user settings file"), tr(msg.c_str()));
        return false;
    }

    // Launcher settings
    file.setFileName(userPath + QString(Config::LauncherSettings::sLauncherConfigFileName));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        // File cannot be opened or created
        cfgError(tr("Error writing Launcher configuration file"),
                 tr("<br><b>Could not open or create %0 for writing</b><br><br> \
                     Please make sure you have the right permissions \
                     and try again.<br>").arg(file.fileName()));
        return false;
    }

    QTextStream stream(&file);
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

    if (setupGameData() && reloadSettings())
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

void Launcher::MainDialog::help()
{
    Misc::HelpViewer::openHelp("reference/index.html");
}
