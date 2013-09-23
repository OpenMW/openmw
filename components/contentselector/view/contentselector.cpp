#include "contentselector.hpp"

#include "../model/contentmodel.hpp"
#include "../model/esmfile.hpp"

#include <QSortFilterProxyModel>

#include <QDebug>
#include <QMenu>
#include <QContextMenuEvent>

ContentSelectorView::ContentSelector::ContentSelector(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    buildContentModel();
    buildGameFileView();
    buildAddonView();
    buildProfilesView();

    updateViews();

}

void ContentSelectorView::ContentSelector::buildContentModel()
{
    mContentModel = new ContentSelectorModel::ContentModel();
    connect(mContentModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));
}

void ContentSelectorView::ContentSelector::buildGameFileView()
{
    mGameFileProxyModel = new QSortFilterProxyModel(this);
    mGameFileProxyModel->setFilterRegExp(QString::number((int)ContentSelectorModel::ContentType_GameFile));
    mGameFileProxyModel->setFilterRole (Qt::UserRole);
    mGameFileProxyModel->setSourceModel (mContentModel);

    gameFileView->setPlaceholderText(QString("Select a game file..."));
    gameFileView->setModel(mGameFileProxyModel);

    connect(gameFileView, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentGameFileIndexChanged(int)));
    connect(gameFileView, SIGNAL(currentIndexChanged(int)), this, SIGNAL(signalGameFileChanged(int)));

    gameFileView->setCurrentIndex(-1);
    gameFileView->setCurrentIndex(0);
}

void ContentSelectorView::ContentSelector::buildAddonView()
{
    mAddonProxyModel = new QSortFilterProxyModel(this);
    mAddonProxyModel->setFilterRegExp (QString::number((int)ContentSelectorModel::ContentType_Addon));
    mAddonProxyModel->setFilterRole (Qt::UserRole);
    mAddonProxyModel->setDynamicSortFilter (true);
    mAddonProxyModel->setSourceModel (mContentModel);

    addonView->setModel(mAddonProxyModel);

    connect(addonView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotAddonTableItemClicked(const QModelIndex &)));
}

void ContentSelectorView::ContentSelector::buildProfilesView()
{
    profilesComboBox->setPlaceholderText(QString("Select a profile..."));
    connect(profilesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentProfileIndexChanged(int)));
}

void ContentSelectorView::ContentSelector::updateViews()
{
    // Ensure the columns are hidden because sort() re-enables them
    addonView->setColumnHidden(1, true);
    addonView->setColumnHidden(2, true);
    addonView->setColumnHidden(3, true);
    addonView->setColumnHidden(4, true);
    addonView->setColumnHidden(5, true);
    addonView->setColumnHidden(6, true);
    addonView->setColumnHidden(7, true);
    addonView->setColumnHidden(8, true);
    addonView->resizeColumnsToContents();
}

void ContentSelectorView::ContentSelector::addFiles(const QString &path)
{
    mContentModel->addFiles(path);
    //mContentModel->sort(3);  // Sort by date accessed
    gameFileView->setCurrentIndex(-1);
    mContentModel->uncheckAll();
}

QStringList ContentSelectorView::ContentSelector::checkedItemsPaths()
{
    QStringList itemPaths;

    foreach( const ContentSelectorModel::EsmFile *file, mContentModel->checkedItems())
        itemPaths << file->path();

    return itemPaths;
}

void ContentSelectorView::ContentSelector::slotCurrentProfileIndexChanged(int index)
{
    emit profileChanged(index);
}

void ContentSelectorView::ContentSelector::slotCurrentGameFileIndexChanged(int index)
{
    static int oldIndex = -1;

    QAbstractItemModel *const model = gameFileView->model();
    QSortFilterProxyModel *proxy = dynamic_cast<QSortFilterProxyModel *>(model);

    if (proxy)
        proxy->setDynamicSortFilter(false);

    if (oldIndex > -1)
        model->setData(model->index(oldIndex, 0), false, Qt::UserRole + 1);

    oldIndex = index;

    model->setData(model->index(index, 0), true, Qt::UserRole + 1);

    if (proxy)
        proxy->setDynamicSortFilter(true);

    emit signalGameFileChanged(true);
}

void ContentSelectorView::ContentSelector::slotAddonTableItemClicked(const QModelIndex &index)
{
    QAbstractItemModel *const model = addonView->model();
    //QSortFilterProxyModel *proxy  = dynamic_cast<QSortFilterProxyModel *>(model);

    if (model->data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        model->setData(index, Qt::Checked, Qt::CheckStateRole);
    else
        model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
}
