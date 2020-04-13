#include "contentselector.hpp"

#include <components/contentselector/model/esmfile.hpp>

#include <QSortFilterProxyModel>

#include <QMenu>
#include <QContextMenuEvent>

#include <QClipboard>
#include <QModelIndex>

ContentSelectorView::ContentSelector::ContentSelector(QWidget *parent) :
    QObject(parent)
{
    ui.setupUi(parent);
    ui.addonView->setDragDropMode(QAbstractItemView::InternalMove);

    buildContentModel();
    buildGameFileView();
    buildAddonView();
}

void ContentSelectorView::ContentSelector::buildContentModel()
{
    QIcon warningIcon(ui.addonView->style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(QSize(16, 15)));
    mContentModel = new ContentSelectorModel::ContentModel(this, warningIcon);
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
    buildContextMenu();
}

void ContentSelectorView::ContentSelector::buildContextMenu()
{
    ui.addonView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.addonView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotShowContextMenu(const QPoint&)));

    mContextMenu = new QMenu(ui.addonView);
    mContextMenu->addAction(tr("&Check Selected"), this, SLOT(slotCheckMultiSelectedItems()));
    mContextMenu->addAction(tr("&Uncheck Selected"), this, SLOT(slotUncheckMultiSelectedItems()));
    mContextMenu->addAction(tr("&Copy Path(s) to Clipboard"), this, SLOT(slotCopySelectedItemsPaths()));
}

void ContentSelectorView::ContentSelector::setProfileContent(const QStringList &fileList)
{
    clearCheckStates();

    for (const QString &filepath : fileList)
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

void ContentSelectorView::ContentSelector::setEncoding(const QString &encoding)
{
    mContentModel->setEncoding(encoding);
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
    for (const QString& gameFileName : mContentModel->gameFiles())
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

void ContentSelectorView::ContentSelector::clearFiles()
{
    mContentModel->clearFiles();
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
    if (file != nullptr)
    {
        QModelIndex index2(mContentModel->indexFromItem(file));
        mContentModel->setData(index2, selected, Qt::UserRole + 1);
    }
}

void ContentSelectorView::ContentSelector::slotAddonTableItemActivated(const QModelIndex &index)
{
    // toggles check state when an AddOn file is double clicked or activated by keyboard
    QModelIndex sourceIndex = mAddonProxyModel->mapToSource (index);

    if (!mContentModel->isEnabled (sourceIndex))
        return;

    Qt::CheckState checkState = Qt::Unchecked;

    if (mContentModel->data(sourceIndex, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        checkState = Qt::Checked;

    mContentModel->setData(sourceIndex, checkState, Qt::CheckStateRole);
}

void ContentSelectorView::ContentSelector::slotShowContextMenu(const QPoint& pos)
{
    QPoint globalPos = ui.addonView->viewport()->mapToGlobal(pos);
    mContextMenu->exec(globalPos);
}

void ContentSelectorView::ContentSelector::setCheckStateForMultiSelectedItems(bool checked)
{
    Qt::CheckState checkState = checked ? Qt::Checked : Qt::Unchecked;
    for (const QModelIndex& index : ui.addonView->selectionModel()->selectedIndexes())
    {
        QModelIndex sourceIndex = mAddonProxyModel->mapToSource(index);
        if (mContentModel->data(sourceIndex, Qt::CheckStateRole).toInt() != checkState)
        {
            mContentModel->setData(sourceIndex, checkState, Qt::CheckStateRole);
        }
    }
}

void ContentSelectorView::ContentSelector::slotUncheckMultiSelectedItems()
{
    setCheckStateForMultiSelectedItems(false);
}

void ContentSelectorView::ContentSelector::slotCheckMultiSelectedItems()
{
    setCheckStateForMultiSelectedItems(true);
}

void ContentSelectorView::ContentSelector::slotCopySelectedItemsPaths()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString filepaths;
    for (const QModelIndex& index : ui.addonView->selectionModel()->selectedIndexes())
    {
        int row = mAddonProxyModel->mapToSource(index).row();
        const ContentSelectorModel::EsmFile *file = mContentModel->item(row);
        filepaths += file->filePath() + "\n";
    }

    if (!filepaths.isEmpty())
    {
        clipboard->setText(filepaths);
    }
}
