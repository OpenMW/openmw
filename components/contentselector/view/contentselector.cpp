#include "contentselector.hpp"

#include "../model/esmfile.hpp"

#include <QSortFilterProxyModel>

#include <QMenu>
#include <QContextMenuEvent>

#include <QGridLayout>
#include <QMessageBox>
#include <assert.h>

#include <QDebug>

ContentSelectorView::ContentSelector::ContentSelector(QWidget *parent) :
    QObject(parent)
{
    ui.setupUi (parent);

    buildContentModel();
    buildGameFileView();
    buildAddonView();
}

void ContentSelectorView::ContentSelector::buildContentModel()
{
    mContentModel = new ContentSelectorModel::ContentModel();
    //connect(mContentModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));
}

void ContentSelectorView::ContentSelector::buildGameFileView()
{
    ui.gameFileView->setVisible (true);

    mGameFileProxyModel = new QSortFilterProxyModel(this);
    mGameFileProxyModel->setFilterRegExp(QString::number((int)ContentSelectorModel::ContentType_GameFile));
    mGameFileProxyModel->setFilterRole (Qt::UserRole);
    mGameFileProxyModel->setSourceModel (mContentModel);

    ui.gameFileView->setPlaceholderText(QString("Select a game file..."));
    ui.gameFileView->setModel(mGameFileProxyModel);

    connect (ui.gameFileView, SIGNAL(currentIndexChanged(int)),
             this, SLOT (slotCurrentGameFileIndexChanged(int)));

    connect (ui.gameFileView, SIGNAL (currentIndexChanged (int)),
             this, SIGNAL (signalCurrentGamefileIndexChanged (int)));

    ui.gameFileView->setCurrentIndex(-1);
    ui.gameFileView->setCurrentIndex(0);
}

void ContentSelectorView::ContentSelector::buildAddonView()
{
    ui.addonView->setVisible (true);

    mAddonProxyModel = new QSortFilterProxyModel(this);
    mAddonProxyModel->setFilterRegExp (QString::number((int)ContentSelectorModel::ContentType_Addon));
    mAddonProxyModel->setFilterRole (Qt::UserRole);
    mAddonProxyModel->setDynamicSortFilter (true);
    mAddonProxyModel->setSourceModel (mContentModel);

    ui.addonView->setModel(mAddonProxyModel);

    connect(ui.addonView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotAddonTableItemClicked(const QModelIndex &)));

    for (int i = 0; i < mAddonProxyModel->rowCount(); ++i)
        qDebug() << mAddonProxyModel->data(mAddonProxyModel->index(i,0,QModelIndex()));
}

void ContentSelectorView::ContentSelector::setGameFile(const QString &filename)
{
    int index = -1;

    if (!filename.isEmpty())
    {
        index = ui.gameFileView->findText(filename);

        //verify that the current index is also checked in the model
        mContentModel->setCheckState(filename, true);
    }

    ui.gameFileView->setCurrentIndex(index);
}

void ContentSelectorView::ContentSelector::clearCheckStates()
{
    mContentModel->uncheckAll();
}

void ContentSelectorView::ContentSelector::setCheckStates(const QStringList &list)
{
    if (list.isEmpty())
        return;

    foreach (const QString &file, list)
        mContentModel->setCheckState(file, Qt::Checked);
}

ContentSelectorModel::ContentFileList
        ContentSelectorView::ContentSelector::selectedFiles() const
{
    if (!mContentModel)
        return ContentSelectorModel::ContentFileList();

    return mContentModel->checkedItems();
}

void ContentSelectorView::ContentSelector::addFiles(const QString &path)
{
    mContentModel->addFiles(path);

    if (ui.gameFileView->currentIndex() != -1)
        ui.gameFileView->setCurrentIndex(-1);
}

void ContentSelectorView::ContentSelector::slotCurrentGameFileIndexChanged(int index)
{
    static int oldIndex = -1;

    QAbstractItemModel *const model = ui.gameFileView->model();
    QSortFilterProxyModel *proxy = dynamic_cast<QSortFilterProxyModel *>(model);

    if (proxy)
        proxy->setDynamicSortFilter(false);

    if (oldIndex > -1)
        model->setData(model->index(oldIndex, 0), false, Qt::UserRole + 1);

    oldIndex = index;

    model->setData(model->index(index, 0), true, Qt::UserRole + 1);

    if (proxy)
        proxy->setDynamicSortFilter(true);
}

void ContentSelectorView::ContentSelector::slotAddonTableItemClicked(const QModelIndex &index)
{
    QAbstractItemModel *const model = ui.addonView->model();
    //QSortFilterProxyModel *proxy  = dynamic_cast<QSortFilterProxyModel *>(model);

    if (model->data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        model->setData(index, Qt::Checked, Qt::CheckStateRole);
    else
        model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
}
