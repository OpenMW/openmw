#include <QtGui>

#include <components/esm/esmreader.hpp>
#include <components/files/configurationmanager.hpp>

#include "model/datafilesmodel.hpp"
#include "model/esm/esmfile.hpp"

#include "utils/lineedit.hpp"

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
    // Model
    mFilesModel = new DataFilesModel(this);

    mFilesProxyModel = new QSortFilterProxyModel();
    mFilesProxyModel->setDynamicSortFilter(true);
    mFilesProxyModel->setSourceModel(mFilesModel);

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

    mFilesTable = new QTableView(this);
    mFilesTable->setModel(mFilesProxyModel);
    mFilesTable->setObjectName("PluginsTable");
    mFilesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mFilesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mFilesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mFilesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mFilesTable->setAlternatingRowColors(true);
    mFilesTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    mFilesTable->horizontalHeader()->setStretchLastSection(true);
    mFilesTable->horizontalHeader()->hide();

    mFilesTable->verticalHeader()->setDefaultSectionSize(height);
    mFilesTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    mFilesTable->setColumnHidden(1, true);
    mFilesTable->setColumnHidden(2, true);
    mFilesTable->setColumnHidden(3, true);
    mFilesTable->setColumnHidden(4, true);
    mFilesTable->setColumnHidden(5, true);
    mFilesTable->setColumnHidden(6, true);
    mFilesTable->setColumnHidden(7, true);
    mFilesTable->setColumnHidden(8, true);

    QVBoxLayout *pageLayout = new QVBoxLayout(this);

    pageLayout->addWidget(filterToolBar);
    pageLayout->addWidget(mFilesTable);

    connect(mFilesTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));
    
    connect(mFilesModel, SIGNAL(checkedItemsChanged(QStringList,QStringList)), mFilesModel, SLOT(slotcheckedItemsChanged(QStringList,QStringList)));

    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

    connect(mFilesTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

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
        mFilesModel->setEncoding(encoding);
    }

    // Add the paths to the respective models
    for (Files::PathContainer::iterator it = dataDirs.begin(); it != dataDirs.end(); ++it) {
        QString path = QString::fromStdString(it->string());
        path.remove(QChar('\"'));
        mFilesModel->addFiles(path);
    }

    mFilesModel->sort(0);
//    mMastersTable->sortByColumn(3, Qt::AscendingOrder);
//    mPluginsTable->sortByColumn(3, Qt::AscendingOrder);

    return true;
}

void DataFilesList::selectedFiles(std::vector<boost::filesystem::path>& paths)
{
    QStringList pluginPaths = mFilesModel->checkedItemsPaths();
    foreach (const QString &path, pluginPaths)
    {
        paths.push_back(path.toStdString());
    }
}

void DataFilesList::check()
{
    // Check the current selection
    if (!mFilesTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = mFilesTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    //qSort(indexes.begin(), indexes.end(), rowSmallerThan);

    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid())
            return;

        mFilesModel->setCheckState(index, Qt::Checked);
    }
}

void DataFilesList::uncheck()
{
    // uncheck the current selection
    if (!mFilesTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = mFilesTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    //qSort(indexes.begin(), indexes.end(), rowSmallerThan);

    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid())
            return;

        mFilesModel->setCheckState(index, Qt::Unchecked);
    }
}

void DataFilesList::refresh()
{
    mFilesModel->sort(0);


    // Refresh the plugins table
    mFilesTable->scrollToTop();
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
        QModelIndex sourceIndex = mFilesProxyModel->mapToSource(index);

        (mFilesModel->checkState(sourceIndex) == Qt::Checked)
                ? mFilesModel->setCheckState(sourceIndex, Qt::Unchecked)
                : mFilesModel->setCheckState(sourceIndex, Qt::Checked);
    }

    return;

}

void DataFilesList::uncheckAll()
{
    mFilesModel->uncheckAll();
}

void DataFilesList::filterChanged(const QString filter)
{
    QRegExp regExp(filter, Qt::CaseInsensitive, QRegExp::FixedString);
    mFilesProxyModel->setFilterRegExp(regExp);
}

void DataFilesList::showContextMenu(const QPoint &point)
{
    // Make sure there are plugins in the view
    if (!mFilesTable->selectionModel()->hasSelection()) {
        return;
    }

    QPoint globalPos = mFilesTable->mapToGlobal(point);

    QModelIndexList indexes = mFilesTable->selectionModel()->selectedIndexes();

    // Show the check/uncheck actions depending on the state of the selected items
    mUncheckAction->setEnabled(false);
    mCheckAction->setEnabled(false);

    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid())
            return;

         (mFilesModel->checkState(index) == Qt::Checked)
             ? mUncheckAction->setEnabled(true)
             : mCheckAction->setEnabled(true);
    }

    // Show menu
    mContextMenu->exec(globalPos);
}

void DataFilesList::setCheckState(const QString& element, Qt::CheckState state)
{
    EsmFile *file = mFilesModel->findItem(element);
    if (file)
    {
        mFilesModel->setCheckState(mFilesModel->indexFromItem(file), Qt::Checked);
    }
}

QStringList DataFilesList::checkedFiles()
{
    return mFilesModel->checkedItems();
}
