#include "contentselector.hpp"

#include "model/datafilesmodel.hpp"
#include "masterproxymodel.hpp"
#include "model/pluginsproxymodel.hpp"

#include <QSortFilterProxyModel>

FileOrderList::ContentSelector::ContentSelector(QWidget *parent) :
    QWidget(parent)
{
}

void FileOrderList::ContentSelector::buildModelsAndViews()
{
    // Models
    mDataFilesModel = new DataFilesModel (this);

    mMasterProxyModel = new FileOrderList::MasterProxyModel (this, mDataFilesModel);
    mPluginsProxyModel = new PluginsProxyModel (this, mDataFilesModel);


    mFilterProxyModel = new QSortFilterProxyModel();
    mFilterProxyModel->setDynamicSortFilter(true);
    mFilterProxyModel->setSourceModel(mPluginsProxyModel);

    masterView->setModel(mMasterProxyModel);
/*
    mastersTable->setModel(mMastersProxyModel);
    mastersTable->setObjectName("MastersTable");
    mastersTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mastersTable->setSortingEnabled(false);
    mastersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mastersTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mastersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mastersTable->setAlternatingRowColors(true);
    mastersTable->horizontalHeader()->setStretchLastSection(true);

    // Set the row height to the size of the checkboxes
    mastersTable->verticalHeader()->setDefaultSectionSize(height);
    mastersTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    mastersTable->verticalHeader()->hide();
*/
    pluginsTable->setModel(mFilterProxyModel);
    pluginsTable->setObjectName("PluginsTable");
    pluginsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    pluginsTable->setSortingEnabled(false);
    pluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pluginsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pluginsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pluginsTable->setAlternatingRowColors(true);
    pluginsTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    pluginsTable->horizontalHeader()->setStretchLastSection(true);

    connect(mDataFilesModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));
}

void FileOrderList::ContentSelector::addFiles(const QString &path)
{
    mDataFilesModel->addFiles(path);
    mDataFilesModel->sort(3);  // Sort by date accessed
}

void FileOrderList::ContentSelector::setEncoding(const QString &encoding)
{
    mDataFilesModel->setEncoding(encoding);
}

void FileOrderList::ContentSelector::setCheckState(QModelIndex index)
{
    if (!index.isValid())
        return;

    QObject *object = QObject::sender();

    // Not a signal-slot call
    if (!object)
        return;


    if (object->objectName() == QLatin1String("PluginsTable")) {
        QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(
                    mFilterProxyModel->mapToSource(index));

        if (sourceIndex.isValid()) {
            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? mDataFilesModel->setCheckState(sourceIndex, Qt::Unchecked)
                    : mDataFilesModel->setCheckState(sourceIndex, Qt::Checked);
        }
    }
/*
    if (object->objectName() == QLatin1String("MastersTable")) {
        QModelIndex sourceIndex = mMasterProxyModel->mapToSource(index);

        if (sourceIndex.isValid()) {
            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? mDataFilesModel->setCheckState(sourceIndex, Qt::Unchecked)
                    : mDataFilesModel->setCheckState(sourceIndex, Qt::Checked);
        }
    }
*/
    return;
}

QStringList FileOrderList::ContentSelector::checkedItemsPaths()
{
    return mDataFilesModel->checkedItemsPaths();
}

void FileOrderList::ContentSelector::updateViews()
{
    // Ensure the columns are hidden because sort() re-enables them
    /*
    mastersTable->setColumnHidden(1, true);
    mastersTable->setColumnHidden(3, true);
    mastersTable->setColumnHidden(4, true);
    mastersTable->setColumnHidden(5, true);
    mastersTable->setColumnHidden(6, true);
    mastersTable->setColumnHidden(7, true);
    mastersTable->setColumnHidden(8, true);
    mastersTable->resizeColumnsToContents();
*/
    pluginsTable->setColumnHidden(1, true);
    pluginsTable->setColumnHidden(3, true);
    pluginsTable->setColumnHidden(4, true);
    pluginsTable->setColumnHidden(5, true);
    pluginsTable->setColumnHidden(6, true);
    pluginsTable->setColumnHidden(7, true);
    pluginsTable->setColumnHidden(8, true);
    pluginsTable->resizeColumnsToContents();

}
