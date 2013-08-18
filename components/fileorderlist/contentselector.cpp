#include "contentselector.hpp"

#include "model/datafilesmodel.hpp"
#include "masterproxymodel.hpp"
#include "model/pluginsproxymodel.hpp"

#include <QSortFilterProxyModel>

#include <QDebug>
#include <QMenu>
#include <QContextMenuEvent>

FileOrderList::ContentSelector::ContentSelector(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);
    buildModelsAndViews();
}

void FileOrderList::ContentSelector::buildModelsAndViews()
{
    // Models
    mDataFilesModel = new DataFilesModel (this);

    mMasterProxyModel = new FileOrderList::MasterProxyModel (this, mDataFilesModel);
    mPluginsProxyModel = new PluginsProxyModel (this, mDataFilesModel);

    masterView->setModel(mMasterProxyModel);
    pluginView->setModel(mPluginsProxyModel);
    pluginView->

    connect(mDataFilesModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));
    connect(pluginView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotPluginTableItemClicked(const QModelIndex &)));
    connect(masterView, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentMasterIndexChanged(int)));
    connect(profilesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentProfileIndexChanged(int)));
}

void FileOrderList::ContentSelector::addFiles(const QString &path)
{
    mDataFilesModel->addFiles(path);
    mDataFilesModel->sort(3);  // Sort by date accessed
    masterView->setCurrentIndex(-1);
    mDataFilesModel->uncheckAll();
}

void FileOrderList::ContentSelector::setEncoding(const QString &encoding)
{
    mDataFilesModel->setEncoding(encoding);
}

void FileOrderList::ContentSelector::setCheckState(QModelIndex index, QSortFilterProxyModel *model)
{
    if (!index.isValid())
        return;

    if (!model)
        return;

    QModelIndex sourceIndex = model->mapToSource(index);

    if (sourceIndex.isValid())
    {
        (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                ? mDataFilesModel->setCheckState(sourceIndex, Qt::Unchecked)
                : mDataFilesModel->setCheckState(sourceIndex, Qt::Checked);
    }
}

QStringList FileOrderList::ContentSelector::checkedItemsPaths()
{
    return mDataFilesModel->checkedItemsPaths();
}

void FileOrderList::ContentSelector::updateViews()
{
    // Ensure the columns are hidden because sort() re-enables them
    pluginView->setColumnHidden(1, true);
    pluginView->setColumnHidden(3, true);
    pluginView->setColumnHidden(4, true);
    pluginView->setColumnHidden(5, true);
    pluginView->setColumnHidden(6, true);
    pluginView->setColumnHidden(7, true);
    pluginView->setColumnHidden(8, true);
    pluginView->resizeColumnsToContents();

}

void FileOrderList::ContentSelector::slotCurrentProfileIndexChanged(int index)
{
    emit profileChanged(index);
}

void FileOrderList::ContentSelector::slotCurrentMasterIndexChanged(int index)
{
    QObject *object = QObject::sender();

    // Not a signal-slot call
    if (!object)
        return;

    setCheckState(mMasterProxyModel->index(index, 0), mMasterProxyModel);
}

void FileOrderList::ContentSelector::slotPluginTableItemClicked(const QModelIndex &index)
{
    setCheckState(index, mPluginsProxyModel);
}
