#include "contentselector.hpp"

#include "../model/datafilesmodel.hpp"
#include "../model/contentmodel.hpp"
#include "../model/esmfile.hpp"

#include <QSortFilterProxyModel>

#include <QDebug>
#include <QMenu>
#include <QContextMenuEvent>

EsxView::ContentSelector::ContentSelector(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    buildSourceModel();
    buildMasterView();
    buildPluginsView();
    buildProfilesView();

    updateViews();

}

void EsxView::ContentSelector::buildSourceModel()
{
    mContentModel = new EsxModel::ContentModel();
    connect(mContentModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));
}

void EsxView::ContentSelector::buildMasterView()
{
    mMasterProxyModel = new QSortFilterProxyModel(this);
    mMasterProxyModel->setFilterRegExp(QString("game"));
    mMasterProxyModel->setFilterRole (Qt::UserRole);
    mMasterProxyModel->setSourceModel (mContentModel);

    masterView->setPlaceholderText(QString("Select a game file..."));
    masterView->setModel(mMasterProxyModel);

    connect(masterView, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentMasterIndexChanged(int)));

    masterView->setCurrentIndex(-1);
    masterView->setCurrentIndex(0);
}

void EsxView::ContentSelector::buildPluginsView()
{
    mPluginsProxyModel = new QSortFilterProxyModel(this);
    mPluginsProxyModel->setFilterRegExp (QString("addon"));
    mPluginsProxyModel->setFilterRole (Qt::UserRole);
    mPluginsProxyModel->setDynamicSortFilter (true);
    mPluginsProxyModel->setSourceModel (mContentModel);

    pluginView->setModel(mPluginsProxyModel);

    connect(pluginView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotPluginTableItemClicked(const QModelIndex &)));
}

void EsxView::ContentSelector::buildProfilesView()
{
    profilesComboBox->setPlaceholderText(QString("Select a profile..."));
    connect(profilesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentProfileIndexChanged(int)));
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

void EsxView::ContentSelector::addFiles(const QString &path)
{
    mContentModel->addFiles(path);
    mContentModel->sort(3);  // Sort by date accessed
    masterView->setCurrentIndex(-1);
    mContentModel->uncheckAll();
}

void EsxView::ContentSelector::setEncoding(const QString &encoding)
{
    mContentModel->setEncoding(encoding);
}

QStringList EsxView::ContentSelector::checkedItemsPaths()
{
    QStringList itemPaths;

    foreach( const EsxModel::EsmFile *file, mContentModel->checkedItems())
        itemPaths << file->path();

    return itemPaths;
}

void EsxView::ContentSelector::slotCurrentProfileIndexChanged(int index)
{
    emit profileChanged(index);
}

void EsxView::ContentSelector::slotCurrentMasterIndexChanged(int index)
{
    static int oldIndex = -1;

    QAbstractItemModel *const model = masterView->model();
    QSortFilterProxyModel *proxy = dynamic_cast<QSortFilterProxyModel *>(model);

    if (proxy)
        proxy->setDynamicSortFilter(false);

    if (oldIndex > -1)
        qDebug() << "clearing old master check state";
        model->setData(model->index(oldIndex, 0), false, Qt::UserRole + 1);

    oldIndex = index;

    qDebug() << "setting new master check state";
    model->setData(model->index(index, 0), true, Qt::UserRole + 1);

    if (proxy)
        proxy->setDynamicSortFilter(true);
}

void EsxView::ContentSelector::slotPluginTableItemClicked(const QModelIndex &index)
{
    QAbstractItemModel *const model = pluginView->model();
    QSortFilterProxyModel *proxy  = dynamic_cast<QSortFilterProxyModel *>(model);

    if (proxy)
        proxy->setDynamicSortFilter(false);

    if (model->data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        model->setData(index, Qt::Checked, Qt::CheckStateRole);
    else
        model->setData(index, Qt::Unchecked, Qt::CheckStateRole);

    if (proxy)
        proxy->setDynamicSortFilter(true);
}
