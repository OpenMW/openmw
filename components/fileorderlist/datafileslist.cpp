#include <QtGui>

#include <components/esm/esmreader.hpp>
#include <components/files/configurationmanager.hpp>

#include "model/datafilesmodel.hpp"
#include "model/esm/esmfile.hpp"

#include "utils/filedialog.hpp"
#include "utils/lineedit.hpp"
#include "utils/naturalsort.hpp"

#include "datafileslist.hpp"

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

DataFilesList::DataFilesList(Files::ConfigurationManager &cfg, QWidget *parent)
    : QWidget(parent)
    , mCfgMgr(cfg)
{
    // Models
    mMastersModel = new DataFilesModel(this);
    mPluginsModel = new DataFilesModel(this);

    mPluginsProxyModel = new QSortFilterProxyModel();
    mPluginsProxyModel->setDynamicSortFilter(true);
    mPluginsProxyModel->setSourceModel(mPluginsModel);

    // Filter toolbar
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

    QCheckBox checkBox;
    unsigned int height = checkBox.sizeHint().height() + 4;

    mMastersTable = new QTableView(this);
    mMastersTable->setModel(mMastersModel);
    mMastersTable->setObjectName("MastersTable");
    mMastersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mMastersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mMastersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mMastersTable->setAlternatingRowColors(true);
    mMastersTable->horizontalHeader()->setStretchLastSection(true);
    mMastersTable->horizontalHeader()->hide();

    // Set the row height to the size of the checkboxes
    mMastersTable->verticalHeader()->setDefaultSectionSize(height);
    mMastersTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    mMastersTable->verticalHeader()->hide();
    mMastersTable->setColumnHidden(1, true);
    mMastersTable->setColumnHidden(2, true);
    mMastersTable->setColumnHidden(3, true);
    mMastersTable->setColumnHidden(4, true);
    mMastersTable->setColumnHidden(5, true);
    mMastersTable->setColumnHidden(6, true);
    mMastersTable->setColumnHidden(7, true);
    mMastersTable->setColumnHidden(8, true);

    mPluginsTable = new QTableView(this);
    mPluginsTable->setModel(mPluginsProxyModel);
    mPluginsTable->setObjectName("PluginsTable");
    mPluginsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mPluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPluginsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mPluginsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mPluginsTable->setAlternatingRowColors(true);
    mPluginsTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    mPluginsTable->horizontalHeader()->setStretchLastSection(true);
    mPluginsTable->horizontalHeader()->hide();

    mPluginsTable->verticalHeader()->setDefaultSectionSize(height);
    mPluginsTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    mPluginsTable->setColumnHidden(1, true);
    mPluginsTable->setColumnHidden(2, true);
    mPluginsTable->setColumnHidden(3, true);
    mPluginsTable->setColumnHidden(4, true);
    mPluginsTable->setColumnHidden(5, true);
    mPluginsTable->setColumnHidden(6, true);
    mPluginsTable->setColumnHidden(7, true);
    mPluginsTable->setColumnHidden(8, true);

    // Add both tables to a splitter
    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);
    splitter->addWidget(mMastersTable);
    splitter->addWidget(mPluginsTable);

    // Adjust the default widget widths inside the splitter
    QList<int> sizeList;
    sizeList << 175 << 200;
    splitter->setSizes(sizeList);

    QVBoxLayout *pageLayout = new QVBoxLayout(this);

    pageLayout->addWidget(filterToolBar);
    pageLayout->addWidget(splitter);

    connect(mPluginsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));
    connect(mMastersTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));

    connect(mMastersModel, SIGNAL(checkedItemsChanged(QStringList,QStringList)), mPluginsModel, SLOT(slotcheckedItemsChanged(QStringList,QStringList)));

    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

    connect(mPluginsTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    createActions();
}

void DataFilesList::createActions()
{
    // Refresh the plugins
    QAction *refreshAction = new QAction(tr("Refresh"), this);
    refreshAction->setShortcut(QKeySequence(tr("F5")));
    connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));

    // Context menu actions
    mCheckAction = new QAction(tr("Check selected"), this);
    connect(mCheckAction, SIGNAL(triggered()), this, SLOT(check()));

    mUncheckAction = new QAction(tr("Uncheck selected"), this);
    connect(mUncheckAction, SIGNAL(triggered()), this, SLOT(uncheck()));

    // Context menu for the plugins table
    mContextMenu = new QMenu(this);

    mContextMenu->addAction(mCheckAction);
    mContextMenu->addAction(mUncheckAction);

}

bool DataFilesList::setupDataFiles(Files::PathContainer dataDirs, const QString encoding)
{
    // Set the charset for reading the esm/esp files
    if (!encoding.isEmpty() && encoding != QLatin1String("win1252")) {
        mMastersModel->setEncoding(encoding);
        mPluginsModel->setEncoding(encoding);
    }

    // Add the paths to the respective models
    for (Files::PathContainer::iterator it = dataDirs.begin(); it != dataDirs.end(); ++it) {
        QString path = QString::fromStdString(it->string());
        path.remove(QChar('\"'));
        mMastersModel->addMasters(path);
        mPluginsModel->addPlugins(path);
    }

    mMastersModel->sort(0);
    mPluginsModel->sort(0);
//    mMastersTable->sortByColumn(3, Qt::AscendingOrder);
//    mPluginsTable->sortByColumn(3, Qt::AscendingOrder);

    return true;
}

void DataFilesList::selectedFiles(std::vector<boost::filesystem::path>& paths)
{
    QStringList masterPaths = mMastersModel->checkedItemsPaths();
    foreach (const QString &path, masterPaths)
    {
        paths.push_back(path.toStdString());
    }
    QStringList pluginPaths = mPluginsModel->checkedItemsPaths();
    foreach (const QString &path, pluginPaths)
    {
        paths.push_back(path.toStdString());
    }
}

void DataFilesList::check()
{
    // Check the current selection
    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    //qSort(indexes.begin(), indexes.end(), rowSmallerThan);

    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid())
            return;

        mPluginsModel->setCheckState(index, Qt::Checked);
    }
}

void DataFilesList::uncheck()
{
    // uncheck the current selection
    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    //qSort(indexes.begin(), indexes.end(), rowSmallerThan);

    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid())
            return;

        mPluginsModel->setCheckState(index, Qt::Unchecked);
    }
}

void DataFilesList::refresh()
{
    mPluginsModel->sort(0);


    // Refresh the plugins table
    mPluginsTable->scrollToTop();
}


void DataFilesList::setCheckState(QModelIndex index)
{
    if (!index.isValid())
        return;

    QObject *object = QObject::sender();

    // Not a signal-slot call
    if (!object)
        return;

    if (object->objectName() == QLatin1String("PluginsTable")) {
        QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(index);

        (mPluginsModel->checkState(sourceIndex) == Qt::Checked)
                ? mPluginsModel->setCheckState(sourceIndex, Qt::Unchecked)
                : mPluginsModel->setCheckState(sourceIndex, Qt::Checked);
    }

    if (object->objectName() == QLatin1String("MastersTable")) {
        (mMastersModel->checkState(index) == Qt::Checked)
                ? mMastersModel->setCheckState(index, Qt::Unchecked)
                : mMastersModel->setCheckState(index, Qt::Checked);
    }

    return;

}

void DataFilesList::uncheckAll()
{
    mMastersModel->uncheckAll();
    mPluginsModel->uncheckAll();
}

void DataFilesList::filterChanged(const QString filter)
{
    QRegExp regExp(filter, Qt::CaseInsensitive, QRegExp::FixedString);
    mPluginsProxyModel->setFilterRegExp(regExp);
}

void DataFilesList::showContextMenu(const QPoint &point)
{
    // Make sure there are plugins in the view
    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QPoint globalPos = mPluginsTable->mapToGlobal(point);

    QModelIndexList indexes = mPluginsTable->selectionModel()->selectedIndexes();

    // Show the check/uncheck actions depending on the state of the selected items
    mUncheckAction->setEnabled(false);
    mCheckAction->setEnabled(false);

    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid())
            return;

         (mPluginsModel->checkState(index) == Qt::Checked)
             ? mUncheckAction->setEnabled(true)
             : mCheckAction->setEnabled(true);
    }

    // Show menu
    mContextMenu->exec(globalPos);
}

void DataFilesList::setCheckState(const QString& element, Qt::CheckState state)
{
    EsmFile *file = mPluginsModel->findItem(element);
    if (file)
    {
        mPluginsModel->setCheckState(mPluginsModel->indexFromItem(file), Qt::Checked);
    }
    else
    {
        file = mMastersModel->findItem(element);
        mMastersModel->setCheckState(mMastersModel->indexFromItem(file), Qt::Checked);
    }
}

QStringList DataFilesList::checkedFiles()
{
    return mMastersModel->checkedItems() + mPluginsModel->checkedItems();
}