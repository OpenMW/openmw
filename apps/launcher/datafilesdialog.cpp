#include <QtGui>
#include <components/esm/esm_reader.hpp>

#include "datafilesdialog.h"
#include "datafilesmodel.h"

using namespace ESM;

//DataFilesDialog::DataFilesDialog(QWidget *parent)
//    : QDialog(parent)

DataFilesDialog::DataFilesDialog()
{
    //QWidget *centralWidget = new QWidget;

    dataFilesModel = new DataFilesModel();
    dataFilesModel->setReadOnly(true); // Prevent changes to files
    dataFilesModel->setRootPath("/opt/openmw/data");

//    sortModel = new QSortFilterProxyModel();
//    sortModel->setSourceModel(dataFilesModel);

    selectionModel = new QItemSelectionModel(dataFilesModel);
//    selectionModel = new QItemSelectionModel(sortModel);

    // First, show only plugin files and sort them
    QStringList acceptedfiles = (QStringList() << "*.esp");
    dataFilesModel->setNameFilters(acceptedfiles);
    dataFilesModel->setNameFilterDisables(false); // Hide all other files

    dataFilesModel->sort(3, Qt::AscendingOrder); // Sort the plugins by date
    dataFilesModel->submit(); // Force refresh of the data

    // Now show master files too, to make them appear at the top of the list
    acceptedfiles = (QStringList() << "*.esm" << "*.esp");
    dataFilesModel->setNameFilters(acceptedfiles);
    dataFilesModel->setFilter(QDir::Files);

    // Column 1
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    QHBoxLayout *groupsLayout = new QHBoxLayout();

    QGroupBox *groupFiles = new QGroupBox(tr("Morrowind Files"), this);
    groupFiles->setMinimumWidth(450);
    QVBoxLayout *groupFilesLayout = new QVBoxLayout(groupFiles);

    QSpacerItem *vSpacer1 = new QSpacerItem(20, 2, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QHBoxLayout *filterLayout = new QHBoxLayout();
    QLabel *labelFilter = new QLabel(tr("Filter:"), groupFiles);
    lineFilter = new LineEdit(groupFiles);

    filterLayout->addWidget(labelFilter);
    filterLayout->addWidget(lineFilter);

    // View for the game files
    dataFilesView = new QTableView(groupFiles);

    // Put everything in the correct layouts
    groupFilesLayout->addLayout(filterLayout);
    groupFilesLayout->addItem(vSpacer1);
    groupFilesLayout->addWidget(dataFilesView);
    groupsLayout->addWidget(groupFiles);

    // Column 2
    QGroupBox *groupInfo = new QGroupBox(tr("File Information"), this);
    groupInfo->setFixedWidth(250);
    QVBoxLayout *groupInfoLayout = new QVBoxLayout(groupInfo);

    QSpacerItem *vSpacer2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QLabel *labelAuthor = new QLabel(tr("Author:"), groupInfo);
    lineAuthor = new QLineEdit(groupInfo);
    lineAuthor->setReadOnly(true);

    QSpacerItem *vSpacer3 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QLabel *labelDesc = new QLabel(tr("Description:"), groupInfo);
    textDesc = new QPlainTextEdit(groupInfo);
    textDesc->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textDesc->setMinimumHeight(180);
    textDesc->setReadOnly(true);

    QSpacerItem *vSpacer4 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QLabel *labelDepends = new QLabel(tr("Dependencies:"), groupInfo);
    textDepends = new QPlainTextEdit(groupInfo);
    textDepends->setFixedHeight(80);
    textDepends->setReadOnly(true);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    QDialogButtonBox *buttonBox = new QDialogButtonBox();
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::RestoreDefaults);
    buttonBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    buttonsLayout->addItem(horizontalSpacer);
    buttonsLayout->addWidget(buttonBox);

    // Put everything in the correct layouts
    groupInfoLayout->addItem(vSpacer2);
    groupInfoLayout->addWidget(labelAuthor);
    groupInfoLayout->addWidget(lineAuthor);
    groupInfoLayout->addItem(vSpacer3);
    groupInfoLayout->addWidget(labelDesc);
    groupInfoLayout->addWidget(textDesc);
    groupInfoLayout->addItem(vSpacer4);
    groupInfoLayout->addWidget(labelDepends);
    groupInfoLayout->addWidget(textDepends);

    groupsLayout->addWidget(groupInfo);

    dialogLayout->addLayout(groupsLayout);
    dialogLayout->addLayout(buttonsLayout);

    setWindowTitle(tr("Data Files"));


    // Signals and slots
    connect(dataFilesModel, SIGNAL(directoryLoaded(const QString &)), this, SLOT(setupView()));
//    connect(dataFilesModel, SIGNAL(dataChanged(const QModelIndex, const QModelIndex)), this, SLOT(readConfig()));

    connect(dataFilesView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckstate(QModelIndex)));
    connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(changeData(QModelIndex, QModelIndex)));

    connect(lineFilter, SIGNAL(textChanged(const QString &)), this, SLOT(setFilter()));

    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()), this, SLOT(restoreDefaults()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(writeConfig()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    readConfig();
}

void DataFilesDialog::changeData(QModelIndex index, QModelIndex bottom)
{
    if (!index.isValid()) {
        return;
    }

    // Added for testing

    textDepends->clear(); // Clear the dependencies of previous file

    ESMReader datafile;
    QString path(dataFilesModel->filePath(index));
//    QString path(dataFilesModel->filePath(sortModel->mapToSource(index)));

    datafile.open(path.toStdString()); // Open the selected file

    // Get the author of the selected file and display it
    QString author(QString::fromStdString(datafile.getAuthor()));
    lineAuthor->setText(author);

    // Get the file desciption
    QString desc(QString::fromStdString(datafile.getDesc()));
    textDesc->setPlainText(desc);

    // Get a list of master files on which the file depends
    ESMReader::MasterList mlist = datafile.getMasters();

    for (unsigned int i = 0; i < mlist.size(); ++i) // Add each master file
        textDepends->appendPlainText(QString::fromStdString(mlist[i].name));

    /* Get the date of creation
    QDateTime dateCreated = dataFilesModel->fileInfo(index).created();
    QString created = dateCreated.toString(QString("dd.MM.yyyy"));
    labelDateCreated->setText(created);

    // Get the date last modified
    QDateTime dateModified = dataFilesModel->fileInfo(index).lastModified();
    QString modified = dateModified.toString(QString("dd.MM.yyyy"));
    labelDateModified->setText(modified);*/
}

void DataFilesDialog::setupView()
{
    // The signal directoryLoaded is emitted after all files are in the model
    dataFilesView->setModel(dataFilesModel);
//    dataFilesView->setModel(sortModel);

    // Set the view to the data directory
    dataFilesView->setRootIndex(QModelIndex(dataFilesModel->index("/opt/openmw/data")));
//    dataFilesView->setRootIndex(sortModel->mapFromSource(QModelIndex(dataFilesModel->index("/opt/openmw/data"))));

    dataFilesView->verticalHeader()->hide();

    //dataFilesView->hideColumn(1);
    dataFilesView->hideColumn(3); // Hide Date Modified column
    dataFilesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    dataFilesView->verticalHeader()->setDefaultSectionSize(25); //setHeight

    dataFilesView->setSortingEnabled(true);

    dataFilesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    dataFilesView->setSelectionModel(selectionModel);


    dataFilesView->setAlternatingRowColors(true); // Fancy colors!

    dataFilesView->resizeColumnsToContents();
    dataFilesView->horizontalHeader()->setStretchLastSection(true);


    //sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);

}

void DataFilesDialog::readConfig()
{
    qDebug() << "datachanged";

    // Morrowind.ini settings
    QSettings settings("Morrowind.ini",
                        QSettings::IniFormat);
    settings.beginGroup("Game Files");

    const QStringList childKeys = settings.childKeys();

    // See if the files from the config file actually exist
    foreach (const QString &childKey, childKeys) {
        // Create full path to current file found in config
        QString path = "/opt/openmw/data/"; // Note: get path from config
        path.append(settings.value(childKey).toString());

        QModelIndex index = dataFilesModel->index(path, 0);
//        QModelIndex index = sortModel->mapFromSource(dataFilesModel->index(path, 0));
//        QModelIndex index = sortModel->mapFromSource(dataFilesModel->index(path));

        if (index.isValid()) {
            // File is found in model, set it to checked
            qDebug() << "File is found in model, set it to checked";
      //      dataFilesModel->setData(sortModel->mapToSource(index), Qt::Checked, Qt::CheckStateRole);
              dataFilesModel->setData(index, Qt::Checked, Qt::CheckStateRole);
//            dataFilesModel->checkedItems.insert(QPersistentModelIndex(sortModel->mapToSource(index)));
        //    dataFilesModel->checkedItems.insert(index);

            //qDebug() << index;
        } else {
           // File is not found in the model
           qDebug() << "file not found!";
       }
    }
    settings.endGroup();
}

void DataFilesDialog::writeConfig()
{
/*    // Custom write method: We cannot use QSettings because it does not accept spaces

//    QList<QPersistentModelIndex> checkedItems = dataFilesModel->getCheckedItems().toList();
    QList<QPersistentModelIndex> checkedItems = dataFilesModel->getCheckedItems();

    qSort(checkedItems); // Sort the indexes so that master files end up on top

    QString keyName;
    QString fileName;

    QFile file("Morrowind.ini"); // Specify filepath later
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        close(); // File cannot be opened or created

    //QTextStream in(&file);
    QTextStream in(&file);

    //QString buffer;
    QByteArray buffer;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.contains("GameFile") && line != "[Game Files]") {
            buffer += line + "\n";
        }
    }

    file.close();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        close();

    QTextStream out(&file);
    file.write(buffer);

    out << "[Game Files]\n";

    // Write the list of game files to the config
    for (int i = 0; i < checkedItems.size(); ++i) {
        fileName = dataFilesModel->fileName(checkedItems.at(i));

        keyName.setNum(i);
        keyName.prepend("GameFile");

        out << keyName << "=" << fileName << "\n";
    }

    file.close();*/
    close(); // Exit dialog
}

void DataFilesDialog::restoreDefaults()
{
    // Uncheck all checked items
    dataFilesModel->checkedItems.clear();

    QModelIndexList indexlist; // Make a list of default master files
    indexlist.append(dataFilesModel->index("/opt/openmw/data/Morrowind.esm", 0));
    indexlist.append(dataFilesModel->index("/opt/openmw/data/Tribunal.esm", 0));
    indexlist.append(dataFilesModel->index("/opt/openmw/data/Bloodmoon.esm", 0));

    foreach (const QModelIndex &index, indexlist) {
        if (index.isValid()) {
            // Master file found, check it
            dataFilesModel->setData(index, Qt::Checked, Qt::CheckStateRole);
        }
    }
    dataFilesModel->submit(); // Force refresh of view
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

    if (dataFilesModel->data(index, Qt::CheckStateRole) == Qt::Checked) {
        // Selected row is checked, uncheck it
        dataFilesModel->setData(index, Qt::Unchecked, Qt::CheckStateRole);
    } else {
       dataFilesModel->setData(index, Qt::Checked, Qt::CheckStateRole);
    }
}

void DataFilesDialog::setFilter()
{
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

}
