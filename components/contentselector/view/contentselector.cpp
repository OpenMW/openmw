#include "contentselector.hpp"

#include "../model/esmfile.hpp"

#include <QSortFilterProxyModel>

#include <QMenu>
#include <QContextMenuEvent>

#include <QGridLayout>
#include <QMessageBox>
#include <QModelIndex>
#include <assert.h>

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

    connect (ui.gameFileView, SIGNAL (currentIndexChanged(int)),
             this, SLOT (slotCurrentGameFileIndexChanged(int)));

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

    mContentModel->uncheckAll();
}

QString ContentSelectorView::ContentSelector::currentFile() const
{
    QModelIndex currentIdx = ui.addonView->currentIndex();

    if (!currentIdx.isValid())
        return ui.gameFileView->currentText();

    QModelIndex idx = mContentModel->index(mAddonProxyModel->mapToSource(currentIdx).row(), 0, QModelIndex());
    return mContentModel->data(idx, Qt::DisplayRole).toString();
}

void ContentSelectorView::ContentSelector::slotCurrentGameFileIndexChanged(int index)
{
    static int oldIndex = -1;

    QAbstractItemModel *const model = ui.gameFileView->model();
    QSortFilterProxyModel *proxy = dynamic_cast<QSortFilterProxyModel *>(model);

    if (proxy)
        proxy->setDynamicSortFilter(false);

    if (index != oldIndex)
    {
        if (oldIndex > -1)
            model->setData(model->index(oldIndex, 0), false, Qt::UserRole + 1);

        oldIndex = index;

        model->setData(model->index(index, 0), true, Qt::UserRole + 1);
    }

    if (proxy)
        proxy->setDynamicSortFilter(true);

    emit signalCurrentGamefileIndexChanged (index);
}

void ContentSelectorView::ContentSelector::slotAddonTableItemClicked(const QModelIndex &index)
{
    QAbstractItemModel *const model = ui.addonView->model();

    Qt::CheckState checkState = Qt::Unchecked;

    if (model->data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        checkState = Qt::Checked;

    model->setData(index, checkState, Qt::CheckStateRole);

    if (checkState == Qt::Checked)
        emit signalAddonFileSelected (index.row());
    else
        emit signalAddonFileUnselected (index.row());

}
