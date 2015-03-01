#include "contentselector.hpp"

#include "../model/esmfile.hpp"

#include <QSortFilterProxyModel>

#include <QMenu>
#include <QContextMenuEvent>

#include <QGridLayout>
#include <QMessageBox>
#include <QModelIndex>
#include <QDir>
#include <assert.h>

ContentSelectorView::ContentSelector::ContentSelector(QWidget *parent, bool showGameFiles) :
    QObject(parent)
{
    ui.setupUi(parent);
    ui.addonView->setDragDropMode(QAbstractItemView::InternalMove);

    buildContentModel(showGameFiles);
    buildGameFileView();
    buildAddonView();
}

void ContentSelectorView::ContentSelector::buildContentModel(bool showGameFiles)
{
    QIcon warningIcon(ui.addonView->style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(QSize(16, 15)));
    mContentModel = new ContentSelectorModel::ContentModel(this, warningIcon, showGameFiles);
}

void ContentSelectorView::ContentSelector::buildGameFileView()
{
    ui.gameFileView->setVisible (true);

    ui.gameFileView->setPlaceholderText(QString("Select a game file..."));

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

    connect(ui.addonView, SIGNAL(activated(const QModelIndex&)), this, SLOT(slotAddonTableItemActivated(const QModelIndex&)));
    connect(mContentModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SIGNAL(signalAddonDataChanged(QModelIndex,QModelIndex)));
}

void ContentSelectorView::ContentSelector::setProfileContent(const QStringList &fileList)
{
    clearCheckStates();

    foreach (const QString &filepath, fileList)
    {
        const ContentSelectorModel::EsmFile *file = mContentModel->item(filepath);
        if (file && file->isGameFile())
        {
            setGameFile (filepath);
            break;
        }
    }

    setContentList(fileList);
}

void ContentSelectorView::ContentSelector::setGameFile(const QString &filename)
{
    int index = -1;

    if (!filename.isEmpty())
    {
        const ContentSelectorModel::EsmFile *file = mContentModel->item (filename);
        index = ui.gameFileView->findText (file->fileName());

        //verify that the current index is also checked in the model
        if (!mContentModel->setCheckState(filename, true))
        {
            //throw error in case file not found?
            return;
        }
    }

    ui.gameFileView->setCurrentIndex(index);
}

void ContentSelectorView::ContentSelector::clearCheckStates()
{
    mContentModel->uncheckAll();
}

void ContentSelectorView::ContentSelector::setContentList(const QStringList &list)
{
    if (list.isEmpty())
    {
        slotCurrentGameFileIndexChanged (ui.gameFileView->currentIndex());
    }
    else
        mContentModel->setContentList(list);
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

    // add any game files to the combo box
    foreach(const QString gameFileName, mContentModel->gameFiles())
    {
        if (ui.gameFileView->findText(gameFileName) == -1)
        {
            ui.gameFileView->addItem(gameFileName);
        }
    }

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

    if (index != oldIndex)
    {
        if (oldIndex > -1)
        {
            setGameFileSelected(oldIndex, false);
        }

        oldIndex = index;

        setGameFileSelected(index, true);
        mContentModel->checkForLoadOrderErrors();
    }

    emit signalCurrentGamefileIndexChanged (index);
}

void ContentSelectorView::ContentSelector::setGameFileSelected(int index, bool selected)
{
    QString fileName = ui.gameFileView->itemText(index);
    const ContentSelectorModel::EsmFile* file = mContentModel->item(fileName);
    if (file != NULL)
    {
        QModelIndex index(mContentModel->indexFromItem(file));
        mContentModel->setData(index, selected, Qt::UserRole + 1);
    }
}

void ContentSelectorView::ContentSelector::slotAddonTableItemActivated(const QModelIndex &index)
{
    QModelIndex sourceIndex = mAddonProxyModel->mapToSource (index);

    if (!mContentModel->isEnabled (sourceIndex))
        return;

    Qt::CheckState checkState = Qt::Unchecked;

    if (mContentModel->data(sourceIndex, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        checkState = Qt::Checked;

    mContentModel->setData(sourceIndex, checkState, Qt::CheckStateRole);
}
