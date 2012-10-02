#include <QtGui>

#include <components/esm/esmreader.hpp>
#include <components/files/configurationmanager.hpp>

#include "datafilespage.hpp"
#include "lineedit.hpp"
#include "filedialog.hpp"
#include "naturalsort.hpp"
#include "pluginsmodel.hpp"
#include "pluginsview.hpp"

#include <boost/version.hpp>
/**
 * Workaround for problems with whitespaces in paths in older versions of Boost library
 */
#if (BOOST_VERSION <= 104600)
namespace boost
{

    template<>
    inline boost::filesystem::path lexical_cast<boost::filesystem::path, std::string>(const std::string& arg)
    {
        return boost::filesystem::path(arg);
    }

} /* namespace boost */
#endif /* (BOOST_VERSION <= 104600) */

using namespace ESM;
using namespace std;

//sort QModelIndexList ascending
bool rowGreaterThan(const QModelIndex &index1, const QModelIndex &index2)
{
    return index1.row() >= index2.row();
}

//sort QModelIndexList descending
bool rowSmallerThan(const QModelIndex &index1, const QModelIndex &index2)
{
    return index1.row() <= index2.row();
}

DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, QWidget *parent)
    : QWidget(parent)
    , mCfgMgr(cfg)
{
    mDataFilesModel = new QStandardItemModel(); // Contains all plugins with masters
    mPluginsModel = new PluginsModel(); // Contains selectable plugins

    mPluginsProxyModel = new QSortFilterProxyModel();
    mPluginsProxyModel->setDynamicSortFilter(true);
    mPluginsProxyModel->setSourceModel(mPluginsModel);

    QLabel *filterLabel = new QLabel(tr("&Filter:"), this);
    LineEdit *filterLineEdit = new LineEdit(this);
    filterLabel->setBuddy(filterLineEdit);

    QToolBar *filterToolBar = new QToolBar(this);
    filterToolBar->setMovable(false);

    // Create a container widget and a layout to get the spacer to work
    QWidget *filterWidget = new QWidget(this);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterWidget);
    QSpacerItem *hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    filterLayout->addItem(hSpacer1);
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(filterLineEdit);

    filterToolBar->addWidget(filterWidget);

    mMastersWidget = new QTableWidget(this); // Contains the available masters
    mMastersWidget->setObjectName("MastersWidget");
    mMastersWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    mMastersWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    mMastersWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mMastersWidget->setAlternatingRowColors(true);
    mMastersWidget->horizontalHeader()->setStretchLastSection(true);
    mMastersWidget->horizontalHeader()->hide();
    mMastersWidget->verticalHeader()->hide();
    mMastersWidget->insertColumn(0);

    mPluginsTable = new PluginsView(this);
    mPluginsTable->setModel(mPluginsProxyModel);
    mPluginsTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

    mPluginsTable->horizontalHeader()->setStretchLastSection(true);
    mPluginsTable->horizontalHeader()->hide();

    // Set the row height to the size of the checkboxes
    QCheckBox checkBox;
    unsigned int height = checkBox.sizeHint().height() + 4;

    mPluginsTable->verticalHeader()->setDefaultSectionSize(height);

    // Add both tables to a splitter
    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);

    splitter->addWidget(mMastersWidget);
    splitter->addWidget(mPluginsTable);

    // Adjust the default widget widths inside the splitter
    QList<int> sizeList;
    sizeList << 100 << 300;
    splitter->setSizes(sizeList);

    // Bottom part with profile options
    QLabel *profileLabel = new QLabel(tr("Current Profile: "), this);

    mProfilesComboBox = new ComboBox(this);
    mProfilesComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
    mProfilesComboBox->setInsertPolicy(QComboBox::InsertAtBottom);

    mProfileToolBar = new QToolBar(this);
    mProfileToolBar->setMovable(false);
    mProfileToolBar->setIconSize(QSize(16, 16));

    mProfileToolBar->addWidget(profileLabel);
    mProfileToolBar->addWidget(mProfilesComboBox);

    QVBoxLayout *pageLayout = new QVBoxLayout(this);

    pageLayout->addWidget(filterToolBar);
    pageLayout->addWidget(splitter);
    pageLayout->addWidget(mProfileToolBar);

    connect(mMastersWidget->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(masterSelectionChanged(const QItemSelection&, const QItemSelection&)));

    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(const QString)));

    connect(mPluginsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));
    connect(mPluginsTable, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));

    connect(mProfilesComboBox, SIGNAL(textChanged(const QString&, const QString&)), this, SLOT(profileChanged(const QString&, const QString&)));

    createActions();
    setupConfig();
    //setupDataFiles();

}

void DataFilesPage::setupConfig()
{
    QString config = QString::fromStdString((mCfgMgr.getLocalPath() / "launcher.cfg").string());
    QFile file(config);

    if (!file.exists()) {
        config = QString::fromStdString((mCfgMgr.getUserPath() / "launcher.cfg").string());
    }

    // Open our config file
    mLauncherConfig = new QSettings(config, QSettings::IniFormat);
    file.close();

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    QStringList profiles = mLauncherConfig->childGroups();

    if (profiles.isEmpty()) {
        // Add a default profile
        profiles.append("Default");
    }

    mProfilesComboBox->addItems(profiles);

    QString currentProfile = mLauncherConfig->value("CurrentProfile").toString();

    if (currentProfile.isEmpty()) {
        // No current profile selected
        currentProfile = "Default";
    }

    const int currentIndex = mProfilesComboBox->findText(currentProfile);
    if (currentIndex != -1) {
        // Profile is found
        mProfilesComboBox->setCurrentIndex(currentIndex);
    }

    mLauncherConfig->endGroup();
}


void DataFilesPage::addDataFiles(Files::Collections &fileCollections, const QString &encoding)
{
     // First we add all the master files
    const Files::MultiDirCollection &esm = fileCollections.getCollection(".esm");
    unsigned int i = 0; // Row number

    for (Files::MultiDirCollection::TIter iter(esm.begin()); iter!=esm.end(); ++iter) {
        QString currentMaster = QString::fromStdString(
                boost::filesystem::path (iter->second.filename()).string());

        const QList<QTableWidgetItem*> itemList = mMastersWidget->findItems(currentMaster, Qt::MatchExactly);

        if (itemList.isEmpty()) { // Master is not yet in the widget
            mMastersWidget->insertRow(i);
            QTableWidgetItem *item = new QTableWidgetItem(currentMaster);
            mMastersWidget->setItem(i, 0, item);
            ++i;
        }
    }

    // Now on to the plugins
    const Files::MultiDirCollection &esp = fileCollections.getCollection(".esp");

    for (Files::MultiDirCollection::TIter iter(esp.begin()); iter!=esp.end(); ++iter) {

        try {
            ESMReader fileReader;
            QStringList availableMasters; // Will contain all found masters

            fileReader.setEncoding(encoding.toStdString());
            fileReader.open(iter->second.string());

            // First we fill the availableMasters and the mMastersWidget
            ESMReader::MasterList mlist = fileReader.getMasters();

            for (unsigned int i = 0; i < mlist.size(); ++i) {
                const QString currentMaster = QString::fromStdString(mlist[i].name);
                availableMasters.append(currentMaster);

                const QList<QTableWidgetItem*> itemList = mMastersWidget->findItems(currentMaster, Qt::MatchExactly);

                if (itemList.isEmpty()) { // Master is not yet in the widget
                    mMastersWidget->insertRow(i);

                    QTableWidgetItem *item = new QTableWidgetItem(currentMaster);
                    item->setForeground(Qt::red);
                    item->setFlags(item->flags() & ~(Qt::ItemIsSelectable));

                    mMastersWidget->setItem(i, 0, item);
                }
            }

            availableMasters.sort(); // Sort the masters alphabetically

            // Now we put the current plugin in the mDataFilesModel under its masters
            QStandardItem *parent = new QStandardItem(availableMasters.join(","));

            QString fileName = QString::fromStdString(boost::filesystem::path (iter->second.filename()).string());
            QStandardItem *child = new QStandardItem(fileName);

            // Tooltip information
            QString author = QString::fromStdString(fileReader.getAuthor());
            float version = fileReader.getFVer();
            QString description = QString::fromStdString(fileReader.getDesc());

            // For the date created/modified
            QFileInfo fi(QString::fromStdString(iter->second.string()));

            QString toolTip= QString("<b>Author:</b> %1<br/> \
                                    <b>Version:</b> %2<br/><br/> \
                                    <b>Description:</b><br/> \
                                    %3<br/><br/> \
                                    <b>Created on:</b> %4<br/> \
                                    <b>Last modified:</b> %5")
                                .arg(author)
                                .arg(version)
                                .arg(description)
                                .arg(fi.created().toString(Qt::TextDate))
                                .arg(fi.lastModified().toString(Qt::TextDate));

            child->setToolTip(toolTip);

            const QList<QStandardItem*> masterList = mDataFilesModel->findItems(availableMasters.join(","));

            if (masterList.isEmpty()) { // Masters node not yet in the mDataFilesModel
                parent->appendRow(child);
                mDataFilesModel->appendRow(parent);
            } else {
                // Masters node exists, append current plugin
                foreach (QStandardItem *currentItem, masterList) {
                    currentItem->appendRow(child);
                }
            }

        } catch(std::runtime_error &e) {
            // An error occurred while reading the .esp
            std::cerr << "Error reading .esp: " << e.what() << std::endl;
            continue;
        }
    }


}


bool DataFilesPage::setupDataFiles()
{
    // We use the Configuration Manager to retrieve the configuration values
    boost::program_options::variables_map variables;
    boost::program_options::options_description desc;

    desc.add_options()
        ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken())
        ("data-local", boost::program_options::value<std::string>()->default_value(""))
        ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
        ("encoding", boost::program_options::value<std::string>()->default_value("win1252"));

    mCfgMgr.readConfiguration(variables, desc);

    // Put the paths in a boost::filesystem vector to use with Files::Collections
    mDataDirs = Files::PathContainer(variables["data"].as<Files::PathContainer>());

     std::string local = variables["data-local"].as<std::string>();
     if (!local.empty()) {
         mDataLocal.push_back(Files::PathContainer::value_type(local));
     }

    mCfgMgr.processPaths(mDataDirs);
    mCfgMgr.processPaths(mDataLocal);

    while (mDataDirs.empty()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error detecting Morrowind installation");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setText(tr("<br><b>Could not find the Data Files location</b><br><br> \
                          The directory containing the data files was not found.<br><br> \
                          Press \"Browse...\" to specify the location manually.<br>"));

        QAbstractButton *dirSelectButton =
                msgBox.addButton(tr("B&rowse..."), QMessageBox::ActionRole);

        msgBox.exec();

        if (msgBox.clickedButton() == dirSelectButton) {

            // Show a custom dir selection dialog which only accepts valid dirs
            QString selectedDir = FileDialog::getExistingDirectory(
                        this, tr("Select Data Files Directory"),
                        QDir::currentPath(),
                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

            // Add the user selected data directory
            if (!selectedDir.isEmpty()) {
                mDataDirs.push_back(Files::PathContainer::value_type(selectedDir.toStdString()));
                mCfgMgr.processPaths(mDataDirs);
            } else {
                // Cancel from within the dir selection dialog
                return false;
            }

        } else {
            // Cancel
            return false;
        }
    }

    // Add the plugins in the data directories
    Files::Collections dataCollections(mDataDirs, !variables["fs-strict"].as<bool>());
    Files::Collections dataLocalCollections(mDataLocal, !variables["fs-strict"].as<bool>());

    addDataFiles(dataCollections, QString::fromStdString(variables["encoding"].as<std::string>()));
    addDataFiles(dataLocalCollections, QString::fromStdString(variables["encoding"].as<std::string>()));

    mDataFilesModel->sort(0);

    readConfig();
    return true;
}

void DataFilesPage::createActions()
{
    // Refresh the plugins
    QAction *refreshAction = new QAction(tr("Refresh"), this);
    refreshAction->setShortcut(QKeySequence(tr("F5")));
    connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));

    // Profile actions
    mNewProfileAction = new QAction(QIcon::fromTheme("document-new"), tr("&New Profile"), this);
    mNewProfileAction->setToolTip(tr("New Profile"));
    mNewProfileAction->setShortcut(QKeySequence(tr("Ctrl+N")));
    connect(mNewProfileAction, SIGNAL(triggered()), this, SLOT(newProfile()));


    mCopyProfileAction = new QAction(QIcon::fromTheme("edit-copy"), tr("&Copy Profile"), this);
    mCopyProfileAction->setToolTip(tr("Copy Profile"));
    mCopyProfileAction->setShortcut(QKeySequence(tr("Ctrl+C")));
    connect(mCopyProfileAction, SIGNAL(triggered()), this, SLOT(copyProfile()));

    mDeleteProfileAction = new QAction(QIcon::fromTheme("edit-delete"), tr("Delete Profile"), this);
    mDeleteProfileAction->setToolTip(tr("Delete Profile"));
    mDeleteProfileAction->setShortcut(QKeySequence(tr("Delete")));
    connect(mDeleteProfileAction, SIGNAL(triggered()), this, SLOT(deleteProfile()));

    // Add the newly created actions to the toolbar
    mProfileToolBar->addSeparator();
    mProfileToolBar->addAction(mNewProfileAction);
    mProfileToolBar->addAction(mCopyProfileAction);
    mProfileToolBar->addAction(mDeleteProfileAction);

    // Context menu actions
    mMoveUpAction = new QAction(QIcon::fromTheme("go-up"), tr("Move &Up"), this);
    mMoveUpAction->setShortcut(QKeySequence(tr("Ctrl+U")));
    connect(mMoveUpAction, SIGNAL(triggered()), this, SLOT(moveUp()));

    mMoveDownAction = new QAction(QIcon::fromTheme("go-down"), tr("&Move Down"), this);
    mMoveDownAction->setShortcut(QKeySequence(tr("Ctrl+M")));
    connect(mMoveDownAction, SIGNAL(triggered()), this, SLOT(moveDown()));

    mMoveTopAction = new QAction(QIcon::fromTheme("go-top"), tr("Move to Top"), this);
    mMoveTopAction->setShortcut(QKeySequence(tr("Ctrl+Shift+U")));
    connect(mMoveTopAction, SIGNAL(triggered()), this, SLOT(moveTop()));

    mMoveBottomAction = new QAction(QIcon::fromTheme("go-bottom"), tr("Move to Bottom"), this);
    mMoveBottomAction->setShortcut(QKeySequence(tr("Ctrl+Shift+M")));
    connect(mMoveBottomAction, SIGNAL(triggered()), this, SLOT(moveBottom()));

    mCheckAction = new QAction(tr("Check selected"), this);
    connect(mCheckAction, SIGNAL(triggered()), this, SLOT(check()));

    mUncheckAction = new QAction(tr("Uncheck selected"), this);
    connect(mUncheckAction, SIGNAL(triggered()), this, SLOT(uncheck()));

    // Makes shortcuts work even if the context menu is hidden
    this->addAction(refreshAction);
    this->addAction(mMoveUpAction);
    this->addAction(mMoveDownAction);
    this->addAction(mMoveTopAction);
    this->addAction(mMoveBottomAction);

    // Context menu for the plugins table
    mContextMenu = new QMenu(this);

    mContextMenu->addAction(mMoveUpAction);
    mContextMenu->addAction(mMoveDownAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mMoveTopAction);
    mContextMenu->addAction(mMoveBottomAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mCheckAction);
    mContextMenu->addAction(mUncheckAction);

}

void DataFilesPage::newProfile()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("New Profile"),
                                         tr("Profile Name:"), QLineEdit::Normal,
                                         tr("New Profile"), &ok);
    if (ok && !text.isEmpty()) {
        if (mProfilesComboBox->findText(text) != -1) {
            QMessageBox::warning(this, tr("Profile already exists"),
                                 tr("the profile <b>%0</b> already exists.").arg(text),
                                 QMessageBox::Ok);
        } else {
            // Add the new profile to the combobox
            mProfilesComboBox->addItem(text);
            mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(text));

        }

    }

}

void DataFilesPage::copyProfile()
{
    QString profile = mProfilesComboBox->currentText();
    bool ok;

    QString text = QInputDialog::getText(this, tr("Copy Profile"),
                                         tr("Profile Name:"), QLineEdit::Normal,
                                         tr("%0 Copy").arg(profile), &ok);
    if (ok && !text.isEmpty()) {
        if (mProfilesComboBox->findText(text) != -1) {
            QMessageBox::warning(this, tr("Profile already exists"),
                                 tr("the profile <b>%0</b> already exists.").arg(text),
                                 QMessageBox::Ok);
        } else {
            // Add the new profile to the combobox
            mProfilesComboBox->addItem(text);

            // First write the current profile as the new profile
            writeConfig(text);
            mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(text));

        }

    }

}

void DataFilesPage::deleteProfile()
{
    QString profile = mProfilesComboBox->currentText();


    if (profile.isEmpty()) {
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Delete Profile"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setText(tr("Are you sure you want to delete <b>%0</b>?").arg(profile));

    QAbstractButton *deleteButton =
    msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);

    msgBox.exec();

    if (msgBox.clickedButton() == deleteButton) {
       // Make sure we have no groups open
        while (!mLauncherConfig->group().isEmpty()) {
            mLauncherConfig->endGroup();
        }

        mLauncherConfig->beginGroup("Profiles");

        // Open the profile-name subgroup
        mLauncherConfig->beginGroup(profile);
        mLauncherConfig->remove(""); // Clear the subgroup
        mLauncherConfig->endGroup();
        mLauncherConfig->endGroup();

        // Remove the profile from the combobox
        mProfilesComboBox->removeItem(mProfilesComboBox->findText(profile));
    }
}

void DataFilesPage::moveUp()
{
    // Shift the selected plugins up one row

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    qSort(selectedIndexes.begin(), selectedIndexes.end(), rowSmallerThan);

    QModelIndex firstIndex = mPluginsProxyModel->mapToSource(selectedIndexes.first());

    // Check if the first selected plugin is the top one
    if (firstIndex.row() == 0) {
        return;
    }

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        const QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(currentIndex);
        int currentRow = sourceModelIndex.row();

        if (sourceModelIndex.isValid() && currentRow > 0) {
            mPluginsModel->insertRow((currentRow - 1), mPluginsModel->takeRow(currentRow));

            const QModelIndex targetIndex = mPluginsModel->index((currentRow - 1), 0, QModelIndex());

            mPluginsTable->selectionModel()->select(targetIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
            scrollToSelection();
        }
    }
}

void DataFilesPage::moveDown()
{
    // Shift the selected plugins down one row
    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection descending because selectedIndexes returns an unsorted list
    qSort(selectedIndexes.begin(), selectedIndexes.end(), rowGreaterThan);

    const QModelIndex lastIndex = mPluginsProxyModel->mapToSource(selectedIndexes.first());

    // Check if last selected plugin is bottom one
    if ((lastIndex.row() + 1) == mPluginsModel->rowCount()) {
        return;
    }

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        const QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(currentIndex);
        int currentRow = sourceModelIndex.row();

        if (sourceModelIndex.isValid() && (currentRow + 1) < mPluginsModel->rowCount()) {
            mPluginsModel->insertRow((currentRow + 1), mPluginsModel->takeRow(currentRow));

            const QModelIndex targetIndex = mPluginsModel->index((currentRow + 1), 0, QModelIndex());

            mPluginsTable->selectionModel()->select(targetIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
            scrollToSelection();
        }
    }
}

void DataFilesPage::moveTop()
{
    // Move the selection to the top of the table

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    qSort(selectedIndexes.begin(), selectedIndexes.end(), rowSmallerThan);

    QModelIndex firstIndex = mPluginsProxyModel->mapToSource(selectedIndexes.first());

    // Check if the first selected plugin is the top one
    if (firstIndex.row() == 0) {
        return;
    }

    for (int i=0; i < selectedIndexes.count(); ++i) {

        const QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(selectedIndexes.at(i));

        int currentRow = sourceModelIndex.row();

        if (sourceModelIndex.isValid() && currentRow > 0) {

            mPluginsModel->insertRow(i, mPluginsModel->takeRow(currentRow));
            mPluginsTable->selectionModel()->select(mPluginsModel->index(i, 0, QModelIndex()), QItemSelectionModel::Select | QItemSelectionModel::Rows);
            mPluginsTable->scrollToTop();
        }
    }
}

void DataFilesPage::moveBottom()
{
    // Move the selection to the bottom of the table

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection descending because selectedIndexes returns an unsorted list
    qSort(selectedIndexes.begin(), selectedIndexes.end(), rowSmallerThan);

    const QModelIndex lastIndex = mPluginsProxyModel->mapToSource(selectedIndexes.last());

    // Check if last selected plugin is bottom one
    if ((lastIndex.row() + 1) == mPluginsModel->rowCount()) {
        return;
    }

    for (int i=0; i < selectedIndexes.count(); ++i) {

        const QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(selectedIndexes.at(i));

        // Subtract iterations because takeRow shifts the rows below the taken row up
        int currentRow = sourceModelIndex.row() - i;

        if (sourceModelIndex.isValid() && (currentRow + 1) < mPluginsModel->rowCount()) {
            mPluginsModel->appendRow(mPluginsModel->takeRow(currentRow));

            // Rowcount starts with 1, row numbers start with 0
            const QModelIndex lastRow = mPluginsModel->index((mPluginsModel->rowCount() -1), 0, QModelIndex());

            mPluginsTable->selectionModel()->select(lastRow, QItemSelectionModel::Select | QItemSelectionModel::Rows);
            mPluginsTable->scrollToBottom();
        }
    }
}

void DataFilesPage::check()
{
    // Check the current selection

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    qSort(selectedIndexes.begin(), selectedIndexes.end(), rowSmallerThan);

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(currentIndex);

        if (sourceModelIndex.isValid()) {
            mPluginsModel->setData(sourceModelIndex, Qt::Checked, Qt::CheckStateRole);
        }
    }
}

void DataFilesPage::uncheck()
{
    // Uncheck the current selection

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    qSort(selectedIndexes.begin(), selectedIndexes.end(), rowSmallerThan);

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(currentIndex);

        if (sourceModelIndex.isValid()) {
            mPluginsModel->setData(sourceModelIndex, Qt::Unchecked, Qt::CheckStateRole);
        }
    }
}

void DataFilesPage::refresh()
{
    // Refresh the plugins table

    writeConfig();
    readConfig();
}

void DataFilesPage::scrollToSelection()
{
    // Scroll to the selected plugins

    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    // Get the selected indexes visible by determining the middle index
    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();
    qSort(selectedIndexes.begin(), selectedIndexes.end(), rowSmallerThan);

    // The selected rows including non-selected inside selection
    unsigned int selectedRows = selectedIndexes.last().row() - selectedIndexes.first().row();

    // Determine the row which is roughly in the middle of the selection
    unsigned int middleRow = selectedIndexes.first().row() + (int)(selectedRows / 2) + 1;


    const QModelIndex middleIndex = mPluginsProxyModel->mapFromSource(mPluginsModel->index(middleRow, 0, QModelIndex()));

    // Make sure the whole selection is visible
    mPluginsTable->scrollTo(selectedIndexes.first());
    mPluginsTable->scrollTo(selectedIndexes.last());
    mPluginsTable->scrollTo(middleIndex);
}

void DataFilesPage::showContextMenu(const QPoint &point)
{
    // Make sure there are plugins in the view
    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QPoint globalPos = mPluginsTable->mapToGlobal(point);

    QModelIndexList selectedIndexes = mPluginsTable->selectionModel()->selectedIndexes();

    // Show the check/uncheck actions depending on the state of the selected items
    mUncheckAction->setEnabled(false);
    mCheckAction->setEnabled(false);

    foreach (const QModelIndex &currentIndex, selectedIndexes) {
        if (currentIndex.isValid()) {

            const QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(currentIndex);

            if (!sourceIndex.isValid()) {
                return;
            }

            const QStandardItem *currentItem = mPluginsModel->itemFromIndex(sourceIndex);

            if (currentItem->checkState() == Qt::Checked) {
                mUncheckAction->setEnabled(true);
            } else {
                mCheckAction->setEnabled(true);
            }
        }

    }

    // Make sure these are enabled because they might still be disabled
    mMoveUpAction->setEnabled(true);
    mMoveTopAction->setEnabled(true);
    mMoveDownAction->setEnabled(true);
    mMoveBottomAction->setEnabled(true);

    QModelIndex firstIndex = mPluginsProxyModel->mapToSource(selectedIndexes.first());
    QModelIndex lastIndex = mPluginsProxyModel->mapToSource(selectedIndexes.last());

    // Check if selected first item is top row in model
    if (firstIndex.row() == 0) {
        mMoveUpAction->setEnabled(false);
        mMoveTopAction->setEnabled(false);
    }

    // Check if last row is bottom row in model
    if ((lastIndex.row() + 1) == mPluginsModel->rowCount()) {
        mMoveDownAction->setEnabled(false);
        mMoveBottomAction->setEnabled(false);
    }

    // Show menu
    mContextMenu->exec(globalPos);
}

void DataFilesPage::masterSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (mMastersWidget->selectionModel()->hasSelection()) {

        QStringList masters;
        QString masterstr;

        // Create a QStringList containing all the masters
        const QStringList masterList = selectedMasters();

        foreach (const QString &currentMaster, masterList) {
            masters.append(currentMaster);
        }

        masters.sort();
        masterstr = masters.join(","); // Make a comma-separated QString

        // Iterate over all masters in the datafilesmodel to see if they are selected
        for (int r=0; r<mDataFilesModel->rowCount(); ++r) {
            QModelIndex currentIndex = mDataFilesModel->index(r, 0);
            QString master = currentIndex.data().toString();

            if (currentIndex.isValid()) {
                // See if the current master is in the string with selected masters
                if (masterstr.contains(master))
                {
                    // Append the plugins from the current master to pluginsmodel
                    addPlugins(currentIndex);
                }
            }
        }
    }

   // See what plugins to remove
   QModelIndexList deselectedIndexes = deselected.indexes();

   if (!deselectedIndexes.isEmpty()) {
        foreach (const QModelIndex &currentIndex, deselectedIndexes) {

            QString master = currentIndex.data().toString();
            master.prepend("*");
            master.append("*");
            const QList<QStandardItem *> itemList = mDataFilesModel->findItems(master, Qt::MatchWildcard);

            foreach (const QStandardItem *currentItem, itemList) {
                QModelIndex index = currentItem->index();
                removePlugins(index);
            }
        }
   }

}

void DataFilesPage::addPlugins(const QModelIndex &index)
{
    // Find the plugins in the datafilesmodel and append them to the pluginsmodel
    if (!index.isValid())
        return;

    for (int r=0; r<mDataFilesModel->rowCount(index); ++r ) {
        QModelIndex childIndex = index.child(r, 0);

        if (childIndex.isValid()) {
            // Now we see if the pluginsmodel already contains this plugin
            const QString childIndexData = QVariant(mDataFilesModel->data(childIndex)).toString();
            const QString childIndexToolTip = QVariant(mDataFilesModel->data(childIndex, Qt::ToolTipRole)).toString();

            const QList<QStandardItem *> itemList = mPluginsModel->findItems(childIndexData);

            if (itemList.isEmpty())
            {
                // Plugin not yet in the pluginsmodel, add it
                QStandardItem *item = new QStandardItem(childIndexData);
                item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
                item->setCheckable(true);
                item->setToolTip(childIndexToolTip);

                mPluginsModel->appendRow(item);
            }
        }

    }

}

void DataFilesPage::removePlugins(const QModelIndex &index)
{

    if (!index.isValid())
        return;

    for (int r=0; r<mDataFilesModel->rowCount(index); ++r) {
        QModelIndex childIndex = index.child(r, 0);

        const QList<QStandardItem *> itemList = mPluginsModel->findItems(QVariant(childIndex.data()).toString());

        if (!itemList.isEmpty()) {
            foreach (const QStandardItem *currentItem, itemList) {
                mPluginsModel->removeRow(currentItem->row());
            }
        }
    }

}

void DataFilesPage::setCheckState(QModelIndex index)
{
    if (!index.isValid())
        return;

    QModelIndex sourceModelIndex = mPluginsProxyModel->mapToSource(index);

    if (mPluginsModel->data(sourceModelIndex, Qt::CheckStateRole) == Qt::Checked) {
        // Selected row is checked, uncheck it
        mPluginsModel->setData(sourceModelIndex, Qt::Unchecked, Qt::CheckStateRole);
    } else {
        mPluginsModel->setData(sourceModelIndex, Qt::Checked, Qt::CheckStateRole);
    }
}

const QStringList DataFilesPage::selectedMasters()
{
    QStringList masters;
    const QList<QTableWidgetItem *> selectedMasters = mMastersWidget->selectedItems();

    foreach (const QTableWidgetItem *item, selectedMasters) {
        masters.append(item->data(Qt::DisplayRole).toString());
    }

    return masters;
}

const QStringList DataFilesPage::checkedPlugins()
{
    QStringList checkedItems;

    for (int r=0; r<mPluginsModel->rowCount(); ++r ) {
        QModelIndex index = mPluginsModel->index(r, 0);

        if (index.isValid()) {
            // See if the current item is checked
            if (mPluginsModel->data(index, Qt::CheckStateRole) == Qt::Checked) {
                checkedItems.append(index.data().toString());
            }
        }
    }
    return checkedItems;
}

void DataFilesPage::uncheckPlugins()
{
    for (int r=0; r<mPluginsModel->rowCount(); ++r ) {
        QModelIndex index = mPluginsModel->index(r, 0);

        if (index.isValid()) {
            // See if the current item is checked
            if (mPluginsModel->data(index, Qt::CheckStateRole) == Qt::Checked) {
                mPluginsModel->setData(index, Qt::Unchecked, Qt::CheckStateRole);
            }
        }
    }
}

void DataFilesPage::filterChanged(const QString filter)
{
    QRegExp regExp(filter, Qt::CaseInsensitive, QRegExp::FixedString);
    mPluginsProxyModel->setFilterRegExp(regExp);
}

void DataFilesPage::profileChanged(const QString &previous, const QString &current)
{
    // Prevent the deletion of the default profile
    if (current == "Default") {
        mDeleteProfileAction->setEnabled(false);
    } else {
        mDeleteProfileAction->setEnabled(true);
    }

    if (!previous.isEmpty()) {
        writeConfig(previous);
        mLauncherConfig->sync();
    } else {
        return;
    }

    uncheckPlugins();
    // Deselect the masters
    mMastersWidget->selectionModel()->clearSelection();
    readConfig();
}

void DataFilesPage::readConfig()
{
    QString profile = mProfilesComboBox->currentText();

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->beginGroup(profile);

    QStringList childKeys = mLauncherConfig->childKeys();
    QStringList plugins;

    // Sort the child keys numerical instead of alphabetically
    // i.e. Plugin1, Plugin2 instead of Plugin1, Plugin10
    qSort(childKeys.begin(), childKeys.end(), naturalSortLessThanCI);

    foreach (const QString &key, childKeys) {
        const QString keyValue = mLauncherConfig->value(key).toString();

        if (key.startsWith("Plugin")) {
            plugins.append(keyValue);
            continue;
        }

        if (key.startsWith("Master")) {
            const QList<QTableWidgetItem*> masterList = mMastersWidget->findItems(keyValue, Qt::MatchFixedString);

            if (!masterList.isEmpty()) {
                foreach (QTableWidgetItem *currentMaster, masterList) {
                    mMastersWidget->selectionModel()->select(mMastersWidget->model()->index(currentMaster->row(), 0), QItemSelectionModel::Select);
                }
            }
        }
    }

    // Iterate over the plugins to set their checkstate and position
    for (int i = 0; i < plugins.size(); ++i) {
        const QString plugin = plugins.at(i);

        const QList<QStandardItem *> pluginList = mPluginsModel->findItems(plugin);

        if (!pluginList.isEmpty())
        {
            foreach (const QStandardItem *currentPlugin, pluginList) {
                mPluginsModel->setData(currentPlugin->index(), Qt::Checked, Qt::CheckStateRole);

                // Move the plugin to the position specified in the config file
                mPluginsModel->insertRow(i, mPluginsModel->takeRow(currentPlugin->row()));
            }
        }
    }

}

void DataFilesPage::writeConfig(QString profile)
{
    // Don't overwrite the config if no masters are found
    if (mMastersWidget->rowCount() < 1) {
        return;
    }

    QString pathStr = QString::fromStdString(mCfgMgr.getUserPath().string());
    QDir userPath(pathStr);

    if (!userPath.exists()) {
        if (!userPath.mkpath(pathStr)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Error creating OpenMW configuration directory");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<br><b>Could not create %0</b><br><br> \
                              Please make sure you have the right permissions and try again.<br>").arg(pathStr));
            msgBox.exec();

            qApp->quit();
            return;
        }
    }
    // Open the OpenMW config as a QFile
    QFile file(pathStr.append("openmw.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error writing OpenMW configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0</b><br><br> \
                          Please make sure you have the right permissions and try again.<br>").arg(file.fileName()));
        msgBox.exec();

        qApp->quit();
        return;
    }

    QTextStream in(&file);
    QByteArray buffer;

    // Remove all previous entries from config
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.startsWith("master") &&
            !line.startsWith("plugin") &&
            !line.startsWith("data") &&
            !line.startsWith("data-local"))
        {
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

        qApp->quit();
        return;
    }

    if (!buffer.isEmpty()) {
        file.write(buffer);
    }

    QTextStream gameConfig(&file);

    // First write the list of data dirs
    mCfgMgr.processPaths(mDataDirs);
    mCfgMgr.processPaths(mDataLocal);

    QString path;

    // data= directories
    for (Files::PathContainer::iterator it = mDataDirs.begin(); it != mDataDirs.end(); ++it) {
        path = QString::fromStdString(it->string());
        path.remove(QChar('\"'));

        QDir dir(path);

        // Make sure the string is quoted when it contains spaces
        if (path.contains(" ")) {
            gameConfig << "data=\"" << dir.absolutePath() << "\"" << endl;
        } else {
            gameConfig << "data=" << dir.absolutePath() << endl;
        }
    }

    // data-local directory
    if (!mDataLocal.empty()) {
        path = QString::fromStdString(mDataLocal.front().string());
        path.remove(QChar('\"'));

        QDir dir(path);

        if (path.contains(" ")) {
            gameConfig << "data-local=\"" << dir.absolutePath() << "\"" << endl;
        } else {
            gameConfig << "data-local=" << dir.absolutePath() << endl;
        }
    }


    if (profile.isEmpty()) {
        profile = mProfilesComboBox->currentText();
    }

    if (profile.isEmpty()) {
        return;
    }

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->setValue("CurrentProfile", profile);

    // Open the profile-name subgroup
    mLauncherConfig->beginGroup(profile);
    mLauncherConfig->remove(""); // Clear the subgroup

    // Now write the masters to the configs
    const QStringList masters = selectedMasters();

    // We don't use foreach because we need i
    for (int i = 0; i < masters.size(); ++i) {
        const QString currentMaster = masters.at(i);

        mLauncherConfig->setValue(QString("Master%0").arg(i), currentMaster);
        gameConfig << "master=" << currentMaster << endl;

    }

    // And finally write all checked plugins
    const QStringList plugins = checkedPlugins();

    for (int i = 0; i < plugins.size(); ++i) {
        const QString currentPlugin = plugins.at(i);
        mLauncherConfig->setValue(QString("Plugin%1").arg(i), currentPlugin);
        gameConfig << "plugin=" << currentPlugin << endl;
    }

    file.close();
    mLauncherConfig->endGroup();
    mLauncherConfig->endGroup();
    mLauncherConfig->sync();
}
