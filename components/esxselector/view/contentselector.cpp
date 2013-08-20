#include "contentselector.hpp"

#include "../model/datafilesmodel.hpp"
#include "../model/masterproxymodel.hpp"
#include "../model/pluginsproxymodel.hpp"

#include <QSortFilterProxyModel>

#include <QDebug>
#include <QMenu>
#include <QContextMenuEvent>

EsxView::ContentSelector::ContentSelector(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);
    buildModelsAndViews();
}

void EsxView::ContentSelector::buildModelsAndViews()
{
    // Models
    mDataFilesModel = new EsxModel::DataFilesModel (this);

    mMasterProxyModel = new EsxModel::MasterProxyModel (this, mDataFilesModel);
    mPluginsProxyModel = new EsxModel::PluginsProxyModel (this, mDataFilesModel);

    masterView->setPlaceholderText(QString("Select a game file..."));
    masterView->setModel(mMasterProxyModel);
    pluginView->setModel(mPluginsProxyModel);
    profilesComboBox->setPlaceholderText(QString("Select a profile..."));


    connect(mDataFilesModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));
    connect(pluginView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotPluginTableItemClicked(const QModelIndex &)));
    connect(masterView, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentMasterIndexChanged(int)));
    connect(profilesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentProfileIndexChanged(int)));
}

void EsxView::ContentSelector::addFiles(const QString &path)
{
    mDataFilesModel->addFiles(path);
    mDataFilesModel->sort(3);  // Sort by date accessed
    masterView->setCurrentIndex(-1);
    mDataFilesModel->uncheckAll();
}

void EsxView::ContentSelector::setEncoding(const QString &encoding)
{
    mDataFilesModel->setEncoding(encoding);
}

void EsxView::ContentSelector::setCheckState(QModelIndex index, QSortFilterProxyModel *model)
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

QStringList EsxView::ContentSelector::checkedItemsPaths()
{
    return mDataFilesModel->checkedItemsPaths();
}

void EsxView::ContentSelector::updateViews()
{
    // Ensure the columns are hidden because sort() re-enables them
    pluginView->setColumnHidden(1, true);
    pluginView->setColumnHidden(2, true);
    pluginView->setColumnHidden(3, true);
    pluginView->setColumnHidden(4, true);
    pluginView->setColumnHidden(5, true);
    pluginView->setColumnHidden(6, true);
    pluginView->setColumnHidden(7, true);
    pluginView->setColumnHidden(8, true);
    pluginView->resizeColumnsToContents();

}

void EsxView::ContentSelector::slotCurrentProfileIndexChanged(int index)
{
    emit profileChanged(index);
}

void EsxView::ContentSelector::slotCurrentMasterIndexChanged(int index)
{
    QObject *object = QObject::sender();

    // Not a signal-slot call
    if (!object)
        return;

    setCheckState(mMasterProxyModel->index(index, 0), mMasterProxyModel);
}

void EsxView::ContentSelector::slotPluginTableItemClicked(const QModelIndex &index)
{
    setCheckState(index, mPluginsProxyModel);
}
