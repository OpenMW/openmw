#include <QtGui>
#include <QDebug>

#include <components/files/path.hpp>

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
    setWindowIcon(QIcon(":/images/openmw-icon.png"));
    setMinimumSize(QSize(575, 575));

    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(play()));

    setupConfig();
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
    playButton->setIcon(QIcon(":/images/openmw-icon.png"));
    playButton->setText(tr("Play"));
    playButton->setTextAlignment(Qt::AlignCenter);
    playButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *graphicsButton = new QListWidgetItem(mIconWidget);
    graphicsButton->setIcon(QIcon::fromTheme("video-display", graphicsIcon));
    graphicsButton->setText(tr("Graphics"));
    graphicsButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom | Qt::AlignAbsolute);
    graphicsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *dataFilesButton = new QListWidgetItem(mIconWidget);
    dataFilesButton->setIcon(QIcon(":/images/openmw-plugin-icon.png"));
    dataFilesButton->setText(tr("Data Files"));
    dataFilesButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    dataFilesButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(mIconWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

}

void MainDialog::createPages()
{
    // Various pages
    mPlayPage = new PlayPage(this);
    mGraphicsPage = new GraphicsPage(this);
    mDataFilesPage = new DataFilesPage(this);

    QString dataDir = mGameConfig->value("data").toString();
    mDataFilesPage->setupDataFiles(dataDir);

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
    if (currentPage == QString("Play")) {
        mDataFilesPage->mProfilesComboBox->setCurrentIndex(index);
    }

    if (currentPage == QString("Data Files")) {
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
    qDebug() << "Close event";

    // Now write all config files
    writeConfig();
    event->accept();

}

void MainDialog::play()
{
    // First do a write of all the configs, just to be sure
    writeConfig();

#ifdef Q_WS_WIN
    // Windows TODO: proper install path handling
    QString game = "./openmw.exe";
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
                        Please make sure OpenMW is installed and try again.<br>"));
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

void MainDialog::setupConfig()
{
    // First we read the OpenMW config
    QString config = "./openmw.cfg";
    QFile file(config);

    if (!file.exists()) {
        config = QString::fromStdString(Files::getPath(Files::Path_ConfigUser,
                                                       "openmw", "launcher.cfg"));
    }

    file.setFileName(config); // Just for displaying information
    qDebug() << "Using config file from " << file.fileName();
    file.close();

    // Open our config file
    mGameConfig = new QSettings(config, QSettings::IniFormat);
}

void MainDialog::writeConfig()
{
    // Write the profiles
    mDataFilesPage->writeConfig();
    mDataFilesPage->mLauncherConfig->sync();

    // Write the graphics settings
    mGraphicsPage->writeConfig();
    mGraphicsPage->mOgreConfig->sync();

    // Write to the openmw.cfg
    //QString dataPath = mGameConfig->value("data").toString();
    //dataPath.append("/");

    QStringList dataFiles = mDataFilesPage->selectedMasters();
    dataFiles.append(mDataFilesPage->checkedPlugins());

    qDebug() << "Writing to openmw.cfg";

    // Open the config as a QFile

    QString cfgfile = "./openmw.cfg";

    QFile file(cfgfile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // File cannot be opened or created TODO: throw error
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
        // File cannot be opened or created TODO: throw error
    }

    file.write(buffer);
    QTextStream out(&file);

    // Write the list of game files to the config
    foreach (const QString &currentFile, dataFiles) {
        //QFileInfo dataFile = QFileInfo(QString(currentFile).prepend(dataPath));

        //if (dataFile.exists()) {
        if (currentFile.endsWith(QString(".esm"), Qt::CaseInsensitive)) {
            out << "master=" << currentFile << endl;
        } else if (currentFile.endsWith(QString(".esp"), Qt::CaseInsensitive)) {
            out << "plugin=" << currentFile << endl;
        }
        //}
    }

    file.close();
    qDebug() << "Writing done!";
}
