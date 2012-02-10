#include <QtGui>

#include "maindialog.hpp"
#include "playpage.hpp"
#include "graphicspage.hpp"
#include "datafilespage.hpp"

MainDialog::MainDialog()
{
    mIconWidget = new QListWidget;
    mIconWidget->setObjectName("IconWidget");
    mIconWidget->setViewMode(QListView::IconMode);
    mIconWidget->setWrapping(false);
    mIconWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Just to be sure
    mIconWidget->setIconSize(QSize(48, 48));
    mIconWidget->setMovement(QListView::Static);

    mIconWidget->setMinimumWidth(400);
    mIconWidget->setFixedHeight(80);
    mIconWidget->setSpacing(4);
    mIconWidget->setCurrentRow(0);
    mIconWidget->setFlow(QListView::LeftToRight);

    QGroupBox *groupBox = new QGroupBox(this);
    QVBoxLayout *groupLayout = new QVBoxLayout(groupBox);

    mPagesWidget = new QStackedWidget(groupBox);
    groupLayout->addWidget(mPagesWidget);

    QPushButton *playButton = new QPushButton(tr("Play"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    buttonBox->addButton(playButton, QDialogButtonBox::AcceptRole);

    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(mIconWidget);
    dialogLayout->addWidget(groupBox);
    dialogLayout->addWidget(buttonBox);


    setWindowTitle(tr("OpenMW Launcher"));
    setWindowIcon(QIcon(":/images/openmw.png"));
    // Remove what's this? button
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(QSize(575, 575));

    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(play()));

    createIcons();
    createPages();
}

void MainDialog::createIcons()
{
    if (!QIcon::hasThemeIcon("document-new")) {
        QIcon::setThemeName("tango");
    }

    // We create a fallback icon because the default fallback doesn't work
    QIcon graphicsIcon = QIcon(":/icons/tango/video-display.png");

    QListWidgetItem *playButton = new QListWidgetItem(mIconWidget);
    playButton->setIcon(QIcon(":/images/openmw.png"));
    playButton->setText(tr("Play"));
    playButton->setTextAlignment(Qt::AlignCenter);
    playButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *graphicsButton = new QListWidgetItem(mIconWidget);
    graphicsButton->setIcon(QIcon::fromTheme("video-display", graphicsIcon));
    graphicsButton->setText(tr("Graphics"));
    graphicsButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom | Qt::AlignAbsolute);
    graphicsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *dataFilesButton = new QListWidgetItem(mIconWidget);
    dataFilesButton->setIcon(QIcon(":/images/openmw-plugin.png"));
    dataFilesButton->setText(tr("Data Files"));
    dataFilesButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    dataFilesButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(mIconWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

}

QStringList MainDialog::readConfig(const QString &fileName)
{
    // We can't use QSettings directly because it
    // does not support multiple keys with the same name
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error opening OpenMW configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open %0</b><br><br> \
        Please make sure you have the right permissions and try again.<br>").arg(file.fileName()));
        msgBox.exec();

        QApplication::exit(); // File cannot be opened or created
    }

    QTextStream in(&file);
    QStringList dataDirs;
    QString dataLocal;

    // Read the config line by line
    while (!in.atEnd()) {
        QString line = in.readLine();

        if (line.startsWith("data=")) {
            dataDirs.append(line.remove("data="));
        }

        // Read the data-local key, if more than one are found only the last is used
        if (line.startsWith("data-local=")) {
            dataLocal = line.remove("data-local=");
        }

        // Read fs-strict key
        if (line.startsWith("fs-strict=")) {
            QString value = line.remove("fs-strict=");

            (value.toLower() == QLatin1String("true"))
            ? mStrict = true
            : mStrict = false;

        }

    }

    // Add the data-local= key to the end of the dataDirs for priority reasons
    if (!dataLocal.isEmpty()) {
        dataDirs.append(dataLocal);
    }

    if (!dataDirs.isEmpty())
    {
        // Reset the global datadirs to the newly read entries
        // Else return the previous dataDirs because nothing was found in this file;
        mDataDirs = dataDirs;
    }

    file.close();

    return mDataDirs;
}

void MainDialog::createPages()
{
    mPlayPage = new PlayPage(this);
    mGraphicsPage = new GraphicsPage(mCfgMgr, this);
    mDataFilesPage = new DataFilesPage(mCfgMgr, this);

    // Retrieve all data entries from the configs
    QStringList dataDirs;

    // Global location
    QFile file(QString::fromStdString((mCfgMgr.getGlobalPath()/"openmw.cfg").string()));
    if (file.exists()) {
        dataDirs = readConfig(file.fileName());
    }

    // Local location
    file.setFileName("./openmw.cfg");

    if (file.exists()) {
        dataDirs = readConfig(file.fileName());
    }

    // User location
    file.setFileName(QString::fromStdString((mCfgMgr.getUserPath()/"openmw.cfg").string()));
    if (file.exists()) {
        dataDirs = readConfig(file.fileName());
    }

    file.close();

    if (!dataDirs.isEmpty()) {
        // Now pass the datadirs on to the DataFilesPage
        mDataFilesPage->setupDataFiles(dataDirs, mStrict);
    } else {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error reading OpenMW configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not read the location of the data files</b><br><br> \
                        Please make sure OpenMW is correctly configured and try again.<br>"));
        msgBox.exec();

        QApplication::exit(); // No data or data-local entries in openmw.cfg
    }

    // Set the combobox of the play page to imitate the comobox on the datafilespage
    mPlayPage->mProfilesComboBox->setModel(mDataFilesPage->mProfilesComboBox->model());
    mPlayPage->mProfilesComboBox->setCurrentIndex(mDataFilesPage->mProfilesComboBox->currentIndex());

    // Add the pages to the stacked widget
    mPagesWidget->addWidget(mPlayPage);
    mPagesWidget->addWidget(mGraphicsPage);
    mPagesWidget->addWidget(mDataFilesPage);

    // Select the first page
    mIconWidget->setCurrentItem(mIconWidget->item(0), QItemSelectionModel::Select);

    connect(mPlayPage->mPlayButton, SIGNAL(clicked()), this, SLOT(play()));

    connect(mPlayPage->mProfilesComboBox,
            SIGNAL(currentIndexChanged(int)),
            this, SLOT(profileChanged(int)));

    connect(mDataFilesPage->mProfilesComboBox,
            SIGNAL(currentIndexChanged(int)),
            this, SLOT(profileChanged(int)));

}

void MainDialog::profileChanged(int index)
{
    // Just to be sure, should always have a selection
    if (!mIconWidget->selectionModel()->hasSelection()) {
        return;
    }

    QString currentPage = mIconWidget->currentItem()->data(Qt::DisplayRole).toString();
    if (currentPage == QLatin1String("Play")) {
        mDataFilesPage->mProfilesComboBox->setCurrentIndex(index);
    }

    if (currentPage == QLatin1String("Data Files")) {
        mPlayPage->mProfilesComboBox->setCurrentIndex(index);
    }
}

void MainDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    mPagesWidget->setCurrentIndex(mIconWidget->row(current));
}

void MainDialog::closeEvent(QCloseEvent *event)
{
    // Now write all config files
    writeConfig();
    event->accept();
}

void MainDialog::play()
{
    // First do a write of all the configs, just to be sure
    writeConfig();

#ifdef Q_WS_WIN
    QString game = "./openmw.exe";
    QFile file(game);
#elif defined(Q_WS_MAC)
    QDir dir(QCoreApplication::applicationDirPath());
    QString game = dir.absoluteFilePath("openmw");
    QFile file(game);
#else
    QString game = "./openmw";
    QFile file(game);
#endif

    QProcess process;
    QFileInfo info(file);

    if (!file.exists()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error starting OpenMW");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not find OpenMW</b><br><br> \
                        The OpenMW application is not found.<br> \
                        Please make sure OpenMW is installed correctly and try again.<br>"));
        msgBox.exec();

        return;
    }

    if (!info.isExecutable()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error starting OpenMW");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not start OpenMW</b><br><br> \
                        The OpenMW application is not executable.<br> \
                        Please make sure you have the right permissions and try again.<br>"));
        msgBox.exec();

        return;
    }

    // Start the game
    if (!process.startDetached(game)) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error starting OpenMW");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not start OpenMW</b><br><br> \
                        An error occurred while starting OpenMW.<br><br> \
                        Press \"Show Details...\" for more information.<br>"));
        msgBox.setDetailedText(process.errorString());
        msgBox.exec();

        return;
    } else {
        close();
    }
}

void MainDialog::writeConfig()
{
    // Write the profiles
    mDataFilesPage->writeConfig();
    mDataFilesPage->mLauncherConfig->sync();

    // Write the graphics settings
    mGraphicsPage->writeConfig();
    mGraphicsPage->mOgreConfig->sync();

    QStringList dataFiles = mDataFilesPage->selectedMasters();
    dataFiles.append(mDataFilesPage->checkedPlugins());

    // Open the config as a QFile
    QFile file(QString::fromStdString((mCfgMgr.getUserPath()/"openmw.cfg").string()));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error writing OpenMW configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0</b><br><br> \
                        Please make sure you have the right permissions and try again.<br>").arg(file.fileName()));
        msgBox.exec();

        QApplication::exit(1);
    }

    QTextStream in(&file);
    QByteArray buffer;

    // Remove all previous master/plugin entries from config
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.contains("master") && !line.contains("plugin")) {
            buffer += line += "\n";
        }
    }

    file.close();

    // Now we write back the other config entries
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error writing OpenMW configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not write to %0</b><br><br> \
                        Please make sure you have the right permissions and try again.<br>").arg(file.fileName()));
        msgBox.exec();

        QApplication::exit(1);
    }

    file.write(buffer);

    QTextStream out(&file);

    // Write the list of game files to the config
    foreach (const QString &currentFile, dataFiles) {

        if (currentFile.endsWith(QString(".esm"), Qt::CaseInsensitive)) {
            out << "master=" << currentFile << endl;
        } else if (currentFile.endsWith(QString(".esp"), Qt::CaseInsensitive)) {
            out << "plugin=" << currentFile << endl;
        }
    }

    file.close();
}
