#include <QtGui>
#include <components/esm/esm_reader.hpp>

#include "datafilesdialog.h"

using namespace ESM;

DataFilesDialog::DataFilesDialog()
{
    QSplitter *splitter = new QSplitter(this);
    tree = new QTreeView(this);
    mastertable = new QTableView(this);
    plugintable = new QTableView(this);

    /* listtest
    QTableWidget *pluginlist = new QTableWidget(this);

    pluginlist->horizontalHeader()->setStretchLastSection(true);
    pluginlist->insertColumn(0);

    for (unsigned int i=0; i<6; ++i) {
        pluginlist->insertRow(i);
        QTableWidgetItem *item = new QTableWidgetItem(QString("Plugin %0").arg(i));
        item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
        pluginlist->setItem(i, 0, item);
    }

    pluginlist->insertRow(6);
    pluginlist->setSelectionMode(QAbstractItemView::SingleSelection); // single item can be draged or droped
    pluginlist->setDragEnabled(true);
    pluginlist->setDragDropMode(QAbstractItemView::InternalMove);
    pluginlist->viewport()->setAcceptDrops(true);

    pluginlist->setDropIndicatorShown(true);
    */

    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(tree);
    splitter->addWidget(mastertable);
    splitter->addWidget(plugintable);

    // Adjust the widget heights inside the splitter
    QList<int> sizelist;
    sizelist << 100 << 200 << 400;
    splitter->setSizes(sizelist);

    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(splitter);
    //dialogLayout->addWidget(plugintable);

    datafilesmodel = new QStandardItemModel();
    mastersmodel = new QStandardItemModel();

    QDir datafilesdir("data/");
    if (!datafilesdir.exists())
        qWarning("Cannot find the plugin directory");

    QStringList acceptedfiles = (QStringList() << "*.esp");
    QStringList files;

    datafilesdir.setNameFilters(acceptedfiles);
    files = datafilesdir.entryList();


    //foreach (const QString &currentfile, datafiles) {
    for (int i=0; i<files.count(); ++i)
    {
        ESMReader datafile;
        QString currentfile = files.at(i);

        QStringList masters;
        QString path = QString("data/").append(currentfile);
        datafile.open(path.toStdString());

        ESMReader::MasterList mlist = datafile.getMasters();

        for (unsigned int i = 0; i < mlist.size(); ++i) {// Add each master file
            masters.append(QString::fromStdString(mlist[i].name));
        }

        masters.sort(); // Sort the masters alphabetically

        // Add the masters to mastersmodel
        foreach (const QString &currentmaster, masters) {
            QStandardItem *item = new QStandardItem(currentmaster);
            item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));

            QList<QStandardItem*> foundmasters = mastersmodel->findItems(currentmaster);

            if (foundmasters.isEmpty()) {
                 // Current master is not found in the master, add it
                mastersmodel->appendRow(item);
            }
        }

        // Add the masters to datafilesmodel
        QStandardItem *item = new QStandardItem(masters.join(","));
        item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
        QStandardItem *child = new QStandardItem(currentfile);
        child->setFlags(child->flags() & ~(Qt::ItemIsDropEnabled));

        QList<QStandardItem*> masteritems = datafilesmodel->findItems(masters.join(","));


        if (masteritems.isEmpty()) {
            item->appendRow(child);
            datafilesmodel->appendRow(item);
        } else {
            foreach (QStandardItem *currentitem, masteritems) {
                currentitem->setFlags(currentitem->flags() & ~(Qt::ItemIsDropEnabled));
                currentitem->appendRow(child);
            }
        }
        //if (foundmasters.isEmpty()) {
            //datafilesmodel->appendRow(item);
        //}
    }

    /*for( int r=0; r<5; ++r ) {
        QStandardItem *item = new QStandardItem( QString("Morrowind.esm").arg(r));

        / *for( int i=0; i<3; i++ ) {
            QStandardItem *child = new QStandardItem( QString("Master %0 Item %1").arg(r).arg(i));
            //child->setEditable( false );
            item->appendRow( child );
        }* /

        mastersmodel->setItem(r, 0, item);
    }*/


    pluginsmodel = new QStandardItemModel(0, 1);
    pluginsmodel->setSupportedDragActions(Qt::MoveAction);
    pluginsmodel->invisibleRootItem()->setFlags(Qt::ItemIsDropEnabled);


    masterselectmodel = new QItemSelectionModel(mastersmodel);
    pluginselectmodel = new QItemSelectionModel(pluginsmodel);

    tree->setModel(datafilesmodel);
    tree->header()->hide();

    mastertable->setModel(mastersmodel);
    mastertable->setSelectionModel(masterselectmodel);

    mastertable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mastertable->setSelectionMode(QAbstractItemView::MultiSelection);
    mastertable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mastertable->horizontalHeader()->setStretchLastSection(true);
    mastertable->horizontalHeader()->hide();

    plugintable->setModel(pluginsmodel);
    plugintable->setSelectionModel(pluginselectmodel);
    plugintable->setSelectionBehavior(QAbstractItemView::SelectRows);
    plugintable->setSelectionMode(QAbstractItemView::SingleSelection);
    plugintable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    plugintable->horizontalHeader()->setStretchLastSection(true);
    plugintable->horizontalHeader()->hide();

    plugintable->setDragEnabled(true);
    plugintable->setDragDropMode(QAbstractItemView::InternalMove);
    plugintable->setDropIndicatorShown(true);
    plugintable->setDragDropOverwriteMode(false);
    plugintable->viewport()->setAcceptDrops(true);

    plugintable->setContextMenuPolicy(Qt::CustomContextMenu);


    connect(masterselectmodel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(masterSelectionChanged(const QItemSelection&, const QItemSelection&)));
    connect(plugintable, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(plugintable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckstate(QModelIndex)));
    connect(pluginsmodel, SIGNAL(rowsAboutToBeMoved(const QModelIndex&, int, int, const QModelIndex&, int)), this, SLOT(test()));

    // Adjust the dialog width
    setMinimumWidth(500);
}

void DataFilesDialog::test()
{
    qDebug() << "Breaky Breaky!";
    /*QModelIndexList deselectedindexes = deselected.indexes();

    if (!deselectedindexes.isEmpty()) {
        foreach (const QModelIndex &currentindex, deselectedindexes) {
            qDebug() << "Data is: " << currentindex.data();
            qDebug() << "Row is: "<< currentindex.row();
            QList<QStandardItem *> itemlist = pluginsmodel->findItems(QVariant(currentindex.data()).toString());

            if (!itemlist.isEmpty())
            {
                foreach (const QStandardItem *currentitem, itemlist) {
                    pluginselectmodel->select(currentitem->index(), QItemSelectionModel::Toggle);
                }
            }
        }
    }*/
}

void DataFilesDialog::appendPlugins(const QModelIndex &masterindex)
{
    // Find the plugins in the datafilesmodel and append them to the pluginsmodel
    if (!masterindex.isValid())
        return;

    for (int r=0; r<datafilesmodel->rowCount(masterindex); ++r ) {
        QModelIndex childindex = masterindex.child(r, 0);

        if (childindex.isValid()) {
            // Now we see if the pluginsmodel already contains this plugin
            QList<QStandardItem *> itemlist = pluginsmodel->findItems(QVariant(datafilesmodel->data(childindex)).toString());

            if (itemlist.isEmpty())
            {
                // Plugin not yet in the pluginsmodel, add it
                QStandardItem *item = new QStandardItem(QVariant(datafilesmodel->data(childindex)).toString());
                item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
                item->setCheckable(true);
                pluginsmodel->appendRow(item);
            }
        }

    }

}

void DataFilesDialog::removePlugins(const QModelIndex &masterindex)
{

    if (!masterindex.isValid())
        return;

    for (int r=0; r<datafilesmodel->rowCount(masterindex); ++r) {
        QModelIndex childindex = masterindex.child(r, 0);

        QList<QStandardItem *> itemlist = pluginsmodel->findItems(QVariant(childindex.data()).toString());

        if (!itemlist.isEmpty()) {
            foreach (const QStandardItem *currentitem, itemlist) {
                qDebug() << "Remove plugin:" << currentitem->data(Qt::DisplayRole).toString();
                pluginsmodel->removeRow(currentitem->row());
            }
        }
    }

}

void DataFilesDialog::masterSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (masterselectmodel->hasSelection()) { // Not exactly necessary to check
        const QModelIndexList selectedindexes = masterselectmodel->selectedIndexes();

        QStringList masters;
        QString masterstr;

        // Create a QStringList containing all the masters
        foreach (const QModelIndex &currentindex, selectedindexes) {
            masters.append(currentindex.data().toString());
        }

        masters.sort();
        masterstr = masters.join(","); // Make a comma-separated QString

        qDebug() << "Masters" << masterstr;

        // Iterate over all masters in the datafilesmodel to see if they are selected
        for (int r=0; r<datafilesmodel->rowCount(); ++r) {
            QModelIndex currentindex = datafilesmodel->index(r, 0);
            QString master = currentindex.data().toString();

            if (currentindex.isValid()) {
                // See if the current master is in the string with selected masters
                if (masterstr.contains(master))
                {
                    // Append the plugins from the current master to pluginsmodel
                    appendPlugins(currentindex);
                }
            }
        }
    }

   // See what plugins to remove
   QModelIndexList deselectedindexes = deselected.indexes();

   if (!deselectedindexes.isEmpty()) {
        foreach (const QModelIndex &currentindex, deselectedindexes) {

            QString master = currentindex.data().toString();
            master.prepend("*");
            master.append("*");
            QList<QStandardItem *> itemlist = datafilesmodel->findItems(master, Qt::MatchWildcard);

            if (itemlist.isEmpty())
                qDebug() << "Empty as shit";

            foreach (const QStandardItem *currentitem, itemlist) {

                QModelIndex index = currentitem->index();
                qDebug() << "Master to remove plugins of:" << index.data().toString();

                removePlugins(index);
            }
        }
   }
}

void DataFilesDialog::showContextMenu(const QPoint &point)
{
    qDebug() << "Show me the money!";



    QAction *action1 = new QAction(QIcon::fromTheme("arrow-up-double"), tr("Move to Top"), this);
    QAction *action2 = new QAction(QIcon::fromTheme("arrow-down-double"), tr("Move to Bottom"), this);
    QAction *action3 = new QAction(QIcon::fromTheme("arrow-up"), tr("Move Up"), this);
    QAction *action4 = new QAction(QIcon::fromTheme("arrow-down"), tr("Move Down"), this);
    QAction *action5 = new QAction(this);

    QModelIndex index = plugintable->indexAt(point);

    if (index.isValid()) { // Should be valid!
        const QStandardItem *item = pluginsmodel->itemFromIndex(index);

        if (item->checkState() == Qt::Checked) {
            action5->setText("Uncheck Item");
        } else if (item->checkState() == Qt::Unchecked) {
            action5->setText("Check Item");
        }
    }

    connect(action5, SIGNAL(triggered()), this, SLOT(actionCheckstate()));

    QMenu menu(this);
    menu.addAction(action1);
    menu.addAction(action2);
    menu.addSeparator();
    menu.addAction(action3);
    menu.addAction(action4);
    menu.addSeparator();
    menu.addAction(action5);

    menu.exec(plugintable->viewport()->mapToGlobal(point));

}

void DataFilesDialog::actionCheckstate()
{
    qDebug() << "actionCheckstate";

    const QModelIndexList selectedindexes = pluginselectmodel->selectedIndexes();

    // Should only be one index selected
    foreach (const QModelIndex &currentindex, selectedindexes) {
        setCheckstate(currentindex);
    }

}

void DataFilesDialog::setCheckstate(QModelIndex index)
{
    // No check if index is valid: doubleclicked() always returns
    // a valid index when emitted

    //index = QModelIndex(sortModel->mapToSource(index)); // Get a valid index
    index = index.sibling(index.row(), 0); // reset index to first column
    // because that's where te checkbox is; makes it possible to doubleclick whole row

    if (!index.isValid())
        return;

    if (pluginsmodel->data(index, Qt::CheckStateRole) == Qt::Checked) {
        // Selected row is checked, uncheck it
        pluginsmodel->setData(index, Qt::Unchecked, Qt::CheckStateRole);
    } else {
        pluginsmodel->setData(index, Qt::Checked, Qt::CheckStateRole);
    }
}

void DataFilesDialog::readConfig()
{
/*    QString filename;
    QString path = "data/"; // TODO: Should be global

    QFile file("openmw.cfg"); // Specify filepath later
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "error open";
        close(); // File cannot be opened or created TODO: throw error
    }

    QTextStream in(&file);

    QStringList datafiles;

    // Add each data file read from the config file to a QStringList
    while (!in.atEnd()) {
        QString line = in.readLine();

        if (line.contains("master")) {
            filename = line.remove("master=");
            filename.prepend(path);

            datafiles << filename << "\n";

        } else if (line.contains("plugin")) {
            filename = line.remove("plugin=");
            filename.prepend(path);

            datafiles << filename << "\n";
        }
    }

    file.close();

    // Check if the files are in the model, set to checked if found
    foreach(const QString &currentfile, datafiles) {
        QModelIndex index = dataFilesModel->index(currentfile, 0);

        if (index.isValid()) {
            // File is found in model, set it to checked
            dataFilesModel->setData(index, Qt::Checked, Qt::CheckStateRole);
        }
    }*/
}

void DataFilesDialog::writeConfig()
{
    /* Custom write method: We cannot use QSettings because it does not accept spaces

//    QList<QPersistentModelIndex> checkeditems = dataFilesModel->getCheckedItems().toList();
    QStringList checkeditems = dataFilesModel->getCheckedItems();
    //QString sectionname = "[Game Files]";
    QString filename;
    QFileInfo datafile;

    // Sort the items so that master files end up on top
    foreach (const QString &currentitem, checkeditems) {
        if(currentitem.endsWith(QString(".esm"), Qt::CaseInsensitive)) {
            checkeditems.move(checkeditems.indexOf(currentitem), 0);
        }
    }

    QFile file("openmw.cfg"); // Specify filepath later
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        close(); // File cannot be opened or created TODO: throw error

    //QTextStream in(&file);
    QTextStream in(&file);

    //QString buffer;
    QByteArray buffer;

    // Remove all previous master/plugin entries from config
    while (!in.atEnd()) {
        QString line = in.readLine();
        //if (!line.contains("GameFile") && line != "[Game Files]") {
        if (!line.contains("master") && !line.contains("plugin")) {
            buffer += line += "\n";
        }
    }

    file.close();

    // Now we write back the other config entries
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        close(); // File cannot be opened or created TODO: throw error

    file.write(buffer);
    QTextStream out(&file);

    // Write the section name to the config file before we write out the data files
    //out << sectionname << endl;

    // Write the list of game files to the config
    for (int i = 0; i < checkeditems.size(); ++i) {
        //filename = dataFilesModel->fileName(checkeditems.at(i));
        filename = checkeditems.at(i);
        datafile = QFileInfo(filename);

        if (datafile.exists()) {
            if (filename.endsWith(QString(".esm"), Qt::CaseInsensitive)) {
                out << "master=" << datafile.fileName() << endl;
            } else if (filename.endsWith(QString(".esp"), Qt::CaseInsensitive)) {
                out << "plugin=" << datafile.fileName() << endl;
            }
        }
    }


    file.close();
    close(); // Exit dialog*/
}

void DataFilesDialog::restoreDefaults()
{
/*    // Uncheck all checked items
    dataFilesModel->checkedItems.clear();

    QModelIndexList indexlist; // Make a list of default master files
    indexlist.append(dataFilesModel->index("data/Morrowind.esm", 0));
    indexlist.append(dataFilesModel->index("data/Tribunal.esm", 0));
    indexlist.append(dataFilesModel->index("data/Bloodmoon.esm", 0));

    foreach (const QModelIndex &index, indexlist) {
        if (index.isValid()) {
            // Master file found, check it
            dataFilesModel->setData(index, Qt::Checked, Qt::CheckStateRole);
        }
    }
    dataFilesModel->submit(); // Force refresh of view*/
}



/*void DataFilesDialog::setFilter()
{
    / *
    QStringList filefilter = (QStringList() << "*.esm" << "*.esp");
    QStringList currentfilefilter;

    QString esmfilter = lineFilter->text();
    QString espfilter = lineFilter->text();

    if (lineFilter->text().isEmpty()) {
        dataFilesModel->setNameFilters(filefilter);
    //     sortModel->setFilterRegExp(QRegExp("*.esp", Qt::CaseInsensitive,
  //                                           QRegExp::FixedString));
      //   sortModel->setFilterKeyColumn(0);
        return;
    }

    esmfilter.prepend("*");
    esmfilter.append("*.esm");
    espfilter.prepend("*");
    espfilter.append("*.esp");

    currentfilefilter << esmfilter << espfilter;
//    sortModel->setFilterRegExp(QRegExp(espfilter, Qt::CaseInsensitive,
    //                                         QRegExp::FixedString));
  //  sortModel->setFilterKeyColumn(0);
    dataFilesModel->setNameFilters(currentfilefilter);

//    readConfig();
//    dataFilesModel->submit();
//    dataFilesModel->setData();
* /
}
*/

