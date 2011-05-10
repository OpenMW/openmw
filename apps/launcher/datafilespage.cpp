#include <QtGui>

#include <QDebug> // TODO: Remove

#include <components/esm/esm_reader.hpp>
#include <components/files/path.hpp>

#include "datafilespage.hpp"
#include "lineedit.hpp"

using namespace ESM;

DataFilesPage::DataFilesPage(QWidget *parent) : QWidget(parent)
{
    mDataFilesModel = new QStandardItemModel(); // Contains all plugins with masters
    mPluginsModel = new QStandardItemModel(); // Contains selectable plugins

    mPluginsSelectModel = new QItemSelectionModel(mPluginsModel);

    QLabel *filterLabel = new QLabel(tr("Filter:"), this);
    LineEdit *filterLineEdit = new LineEdit(this);

    QHBoxLayout *topLayout = new QHBoxLayout();
    QSpacerItem *hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    topLayout->addItem(hSpacer1);
    topLayout->addWidget(filterLabel);
    topLayout->addWidget(filterLineEdit);

    mMastersWidget = new QTableWidget(this); // Contains the available masters
    mPluginsTable = new QTableView(this);

    mMastersWidget->setObjectName("MastersWidget");
    mMastersWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    mMastersWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    mMastersWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mMastersWidget->setAlternatingRowColors(true);
    mMastersWidget->horizontalHeader()->setStretchLastSection(true);
    mMastersWidget->horizontalHeader()->hide();
    mMastersWidget->verticalHeader()->hide();
    mMastersWidget->insertColumn(0);

    mPluginsTable->setModel(mPluginsModel);
    mPluginsTable->setSelectionModel(mPluginsSelectModel);
    mPluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPluginsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mPluginsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mPluginsTable->setAlternatingRowColors(true);
    mPluginsTable->horizontalHeader()->setStretchLastSection(true);
    mPluginsTable->horizontalHeader()->hide();

    mPluginsTable->setDragEnabled(true);
    mPluginsTable->setDragDropMode(QAbstractItemView::InternalMove);
    mPluginsTable->setDropIndicatorShown(true);
    mPluginsTable->setDragDropOverwriteMode(false);
    mPluginsTable->viewport()->setAcceptDrops(true);

    mPluginsTable->setContextMenuPolicy(Qt::CustomContextMenu);

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
    QLabel *profileLabel = new QLabel(tr("Current Profile:"), this);

    mProfilesComboBox = new ComboBox(this);
    mProfilesComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
    mProfilesComboBox->setInsertPolicy(QComboBox::InsertAtBottom);

    mNewProfileButton = new QPushButton(this);
    mNewProfileButton->setIcon(QIcon::fromTheme("document-new"));
    mNewProfileButton->setToolTip(tr("New Profile"));
    mNewProfileButton->setShortcut(QKeySequence(tr("Ctrl+N")));

    mCopyProfileButton = new QPushButton(this);
    mCopyProfileButton->setIcon(QIcon::fromTheme("edit-copy"));
    mCopyProfileButton->setToolTip(tr("Copy Profile"));

    mDeleteProfileButton = new QPushButton(this);
    mDeleteProfileButton->setIcon(QIcon::fromTheme("edit-delete"));
    mDeleteProfileButton->setToolTip(tr("Delete Profile"));
    mDeleteProfileButton->setShortcut(QKeySequence(tr("Delete")));

    QHBoxLayout *bottomLayout = new QHBoxLayout();

    bottomLayout->addWidget(profileLabel);
    bottomLayout->addWidget(mProfilesComboBox);
    bottomLayout->addWidget(mNewProfileButton);
    bottomLayout->addWidget(mCopyProfileButton);
    bottomLayout->addWidget(mDeleteProfileButton);

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    // Add some space above and below the page items
    QSpacerItem *vSpacer2 = new QSpacerItem(5, 5, QSizePolicy::Minimum, QSizePolicy::Minimum);
    QSpacerItem *vSpacer3 = new QSpacerItem(5, 5, QSizePolicy::Minimum, QSizePolicy::Minimum);

    pageLayout->addLayout(topLayout);
    pageLayout->addItem(vSpacer2);
    pageLayout->addWidget(splitter);
    pageLayout->addLayout(bottomLayout);
    pageLayout->addItem(vSpacer3);

    connect(mMastersWidget->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(masterSelectionChanged(const QItemSelection&, const QItemSelection&)));

    connect(mPluginsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckstate(QModelIndex)));
    connect(mPluginsModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(resizeRows()));

    connect(mNewProfileButton, SIGNAL(pressed()), this, SLOT(newProfile()));
    connect(mCopyProfileButton, SIGNAL(pressed()), this, SLOT(copyProfile()));
    connect(mDeleteProfileButton, SIGNAL(pressed()), this, SLOT(deleteProfile()));

    setupConfig();
}

void DataFilesPage::newProfile()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("New Profile"),
                                                  tr("Profile Name:"), QLineEdit::Normal,
                                         tr("New Profile"), &ok);
    if (ok && !text.isEmpty()) {
        if (mProfilesComboBox->findText(text) != -1)
        {
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
        if (mProfilesComboBox->findText(text) != -1)
        {
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

    QMessageBox deleteMessageBox(this);
    deleteMessageBox.setWindowTitle(tr("Delete Profile"));
    deleteMessageBox.setText(tr("Are you sure you want to delete <b>%0</b>?").arg(profile));
    deleteMessageBox.setIcon(QMessageBox::Warning);
    QAbstractButton *deleteButton =
        deleteMessageBox.addButton(tr("Delete"), QMessageBox::ActionRole);

    deleteMessageBox.addButton(QMessageBox::Cancel);

    deleteMessageBox.exec();

    if (deleteMessageBox.clickedButton() == deleteButton) {

        qDebug() << "Delete profile " << profile;

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

void DataFilesPage::setupDataFiles(const QString &path)
{
    qDebug() << "setupDataFiles called!";
    // TODO: Add a warning when a master is missing

    QDir dataFilesDir(path);

    if (!dataFilesDir.exists())
        qWarning("Cannot find the plugin directory");

    // First we add all the master files from the plugin dir
    dataFilesDir.setNameFilters((QStringList() << "*.esm")); // Only load masters

    QStringList masterFiles = dataFilesDir.entryList();

    for (int i=0; i<masterFiles.count(); ++i)
    {
        QString currentMaster = masterFiles.at(i);
        const QList<QTableWidgetItem*> itemList = mMastersWidget->findItems(currentMaster, Qt::MatchExactly);

        if (itemList.isEmpty()) // Master is not yet in the widget
            {
                mMastersWidget->insertRow(i);
                QTableWidgetItem *item = new QTableWidgetItem(currentMaster);
                mMastersWidget->setItem(i, 0, item);
            }
    }

    // Now on to the plugins
    dataFilesDir.setNameFilters((QStringList() << "*.esp")); // Only load plugins

    QStringList pluginFiles = dataFilesDir.entryList();

    for (int i=0; i<pluginFiles.count(); ++i)
    {
        ESMReader fileReader;
        QString currentFile = pluginFiles.at(i);
        QStringList availableMasters; // Will contain all found masters

        QString filePath = dataFilesDir.absolutePath();
        filePath.append("/");
        filePath.append(currentFile);
        fileReader.open(filePath.toStdString());

        // First we fill the availableMasters and the mMastersWidget
        ESMReader::MasterList mlist = fileReader.getMasters();

        for (unsigned int i = 0; i < mlist.size(); ++i) {
            const QString currentMaster = QString::fromStdString(mlist[i].name);
            availableMasters.append(currentMaster);

            const QList<QTableWidgetItem*> itemList = mMastersWidget->findItems(currentMaster, Qt::MatchExactly);

            if (itemList.isEmpty()) // Master is not yet in the widget
            {
                mMastersWidget->insertRow(i);
                QTableWidgetItem *item = new QTableWidgetItem(currentMaster);
                mMastersWidget->setItem(i, 0, item);
            }
        }

        availableMasters.sort(); // Sort the masters alphabetically

        // Now we put the currentFile in the mDataFilesModel under its masters
        QStandardItem *parent = new QStandardItem(availableMasters.join(","));
        QStandardItem *child = new QStandardItem(currentFile);

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
    }

    readConfig();
}

void DataFilesPage::setupConfig()
{
    qDebug() << "setupConfig called";
    QString config = "./launcher.cfg";
    QFile file(config);

    if (!file.exists()) {
        config = QString::fromStdString(Files::getPath(Files::Path_ConfigUser,
                                                       "openmw", "launcher.cfg"));
    }

    file.setFileName(config); // Just for displaying information
    qDebug() << "Using config file from " << file.fileName();
    file.open(QIODevice::ReadOnly);
    qDebug() << "File contents:" << file.readAll();
    file.close();

    // Open our config file
    mLauncherConfig = new QSettings(config, QSettings::IniFormat);
    mLauncherConfig->sync();


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

    qDebug() << mLauncherConfig->value("CurrentProfile").toString();
    qDebug() << mLauncherConfig->childGroups();

    if (currentProfile.isEmpty()) {
        qDebug() << "No current profile selected";
        currentProfile = "Default";
    }
    mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(currentProfile));

    mLauncherConfig->endGroup();

    // Now we connect the combobox to do something if the profile changes
    // This prevents strange behaviour while reading and appending the profiles
    connect(mProfilesComboBox, SIGNAL(textChanged(const QString&, const QString&)), this, SLOT(profileChanged(const QString&, const QString&)));
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

        qDebug() << "Masters" << masterstr;

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
                    mPluginsTable->resizeRowsToContents();
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

            if (itemList.isEmpty())
                qDebug() << "Empty as shit";

            foreach (const QStandardItem *currentItem, itemList) {

                QModelIndex index = currentItem->index();
                qDebug() << "Master to remove plugins of:" << index.data().toString();

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

            const QList<QStandardItem *> itemList = mPluginsModel->findItems(childIndexData);

            if (itemList.isEmpty())
            {
                // Plugin not yet in the pluginsmodel, add it
                QStandardItem *item = new QStandardItem(childIndexData);
                item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
                item->setCheckable(true);

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
                qDebug() << "Remove plugin:" << currentItem->data(Qt::DisplayRole).toString();

                mPluginsModel->removeRow(currentItem->row());
            }
        }
    }

}

void DataFilesPage::setCheckstate(QModelIndex index)
{
    if (!index.isValid())
        return;

    if (mPluginsModel->data(index, Qt::CheckStateRole) == Qt::Checked) {
        // Selected row is checked, uncheck it
        mPluginsModel->setData(index, Qt::Unchecked, Qt::CheckStateRole);
    } else {
        mPluginsModel->setData(index, Qt::Checked, Qt::CheckStateRole);
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
                qDebug() << "Uncheck!";
                mPluginsModel->setData(index, Qt::Unchecked, Qt::CheckStateRole);
            }
        }
    }
}

void DataFilesPage::resizeRows()
{
    // Contents changed
    mPluginsTable->resizeRowsToContents();
}

void DataFilesPage::profileChanged(const QString &previous, const QString &current)
{
    qDebug() << "Profile changed " << current << previous;

    if (!previous.isEmpty()) {
        writeConfig(previous);
        mLauncherConfig->sync();
    }

    uncheckPlugins();
    // Deselect the masters
    mMastersWidget->selectionModel()->clearSelection();
    readConfig();

}

void DataFilesPage::readConfig()
{
    QString profile = mProfilesComboBox->currentText();
    qDebug() << "read from: " << profile;

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->beginGroup(profile);

    QStringList childKeys = mLauncherConfig->childKeys();

    foreach (const QString &key, childKeys) {
        const QString keyValue = mLauncherConfig->value(key).toString();

        if (key.startsWith("Plugin")) {
            const QList<QStandardItem *> pluginList = mPluginsModel->findItems(keyValue);

            if (!pluginList.isEmpty())
            {
                foreach (const QStandardItem *currentPlugin, pluginList) {
                    mPluginsModel->setData(currentPlugin->index(), Qt::Checked, Qt::CheckStateRole);
                }
            }
        }

        if (key.startsWith("Master")) {
            qDebug() << "Read master: " << keyValue;
            const QList<QTableWidgetItem*> masterList = mMastersWidget->findItems(keyValue, Qt::MatchFixedString);

            if (!masterList.isEmpty()) {
                foreach (QTableWidgetItem *currentMaster, masterList) {
                    mMastersWidget->selectionModel()->select(mMastersWidget->model()->index(currentMaster->row(), 0), QItemSelectionModel::Select);
                }
            }
        }
    }
}

void DataFilesPage::writeConfig(QString profile)
{
    // TODO: Testing the config here
    if (profile.isEmpty()) {
        profile = mProfilesComboBox->currentText();
    }

    if (profile.isEmpty()) {
        return;
    }

    qDebug() << "Writing: " << profile;

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->setValue("CurrentProfile", profile);

    // Open the profile-name subgroup
    qDebug() << "beginning group: " << profile;
    mLauncherConfig->beginGroup(profile);
    mLauncherConfig->remove(""); // Clear the subgroup

    // First write the masters to the config
    const QStringList masterList = selectedMasters();

    // We don't use foreach because we need i
    for (int i = 0; i < masterList.size(); ++i) {
        const QString master = masterList.at(i);
        mLauncherConfig->setValue(QString("Master%0").arg(i), master);
    }

    // Now write all checked plugins
    const QStringList plugins = checkedPlugins();

    for (int i = 0; i < plugins.size(); ++i)
    {
        mLauncherConfig->setValue(QString("Plugin%1").arg(i), plugins.at(i));
    }

    mLauncherConfig->endGroup();
    mLauncherConfig->endGroup();

}
