#include "contentselector.hpp"

#include "ui_contentselector.h"

#include <components/contentselector/model/esmfile.hpp>

#include <QClipboard>
#include <QMenu>
#include <QModelIndex>
#include <QProgressDialog>
#include <QSortFilterProxyModel>

ContentSelectorView::ContentSelector::ContentSelector(QWidget* parent, bool showOMWScripts)
    : QObject(parent)
    , ui(std::make_unique<Ui::ContentSelector>())
{
    ui->setupUi(parent);
    ui->addonView->setDragDropMode(QAbstractItemView::InternalMove);

    if (!showOMWScripts)
    {
        ui->languageComboBox->setHidden(true);
        ui->refreshButton->setHidden(true);
    }

    buildContentModel(showOMWScripts);
    buildGameFileView();
    buildAddonView();
}

ContentSelectorView::ContentSelector::~ContentSelector() = default;

void ContentSelectorView::ContentSelector::buildContentModel(bool showOMWScripts)
{
    QIcon warningIcon(ui->addonView->style()->standardIcon(QStyle::SP_MessageBoxWarning));
    QIcon errorIcon(ui->addonView->style()->standardIcon(QStyle::SP_MessageBoxCritical));
    mContentModel = new ContentSelectorModel::ContentModel(this, warningIcon, errorIcon, showOMWScripts);
}

void ContentSelectorView::ContentSelector::buildGameFileView()
{
    ui->gameFileView->addItem(tr("<No game file>"));
    ui->gameFileView->setVisible(true);

    connect(ui->gameFileView, qOverload<int>(&ComboBox::currentIndexChanged), this,
        &ContentSelector::slotCurrentGameFileIndexChanged);

    ui->gameFileView->setCurrentIndex(0);
}

class AddOnProxyModel : public QSortFilterProxyModel
{
public:
    explicit AddOnProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        static const QString ContentTypeAddon = QString::number((int)ContentSelectorModel::ContentType_Addon);

        QModelIndex nameIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        const QString userRole = sourceModel()->data(nameIndex, Qt::UserRole).toString();

        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent) && userRole == ContentTypeAddon;
    }
};

bool ContentSelectorView::ContentSelector::isGamefileSelected() const
{
    return ui->gameFileView->currentIndex() > 0;
}

QWidget* ContentSelectorView::ContentSelector::uiWidget() const
{
    return ui->contentGroupBox;
}

QComboBox* ContentSelectorView::ContentSelector::languageBox() const
{
    return ui->languageComboBox;
}

QToolButton* ContentSelectorView::ContentSelector::refreshButton() const
{
    return ui->refreshButton;
}

QLineEdit* ContentSelectorView::ContentSelector::searchFilter() const
{
    return ui->searchFilter;
}

void ContentSelectorView::ContentSelector::buildAddonView()
{
    ui->addonView->setVisible(true);

    mAddonProxyModel = new AddOnProxyModel(this);
    mAddonProxyModel->setFilterRegularExpression(searchFilter()->text());
    mAddonProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mAddonProxyModel->setDynamicSortFilter(true);
    mAddonProxyModel->setSourceModel(mContentModel);

    connect(ui->searchFilter, &QLineEdit::textEdited, mAddonProxyModel, &QSortFilterProxyModel::setFilterWildcard);
    connect(ui->searchFilter, &QLineEdit::textEdited, this, &ContentSelector::slotSearchFilterTextChanged);

    ui->addonView->setModel(mAddonProxyModel);

    connect(ui->addonView, &QTableView::activated, this, &ContentSelector::slotAddonTableItemActivated);
    connect(mContentModel, &ContentSelectorModel::ContentModel::dataChanged, this,
        &ContentSelector::signalAddonDataChanged);
    connect(mContentModel, &ContentSelectorModel::ContentModel::dataChanged, this, &ContentSelector::slotRowsMoved);
    buildContextMenu();
}

void ContentSelectorView::ContentSelector::buildContextMenu()
{
    ui->addonView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->addonView, &QTableView::customContextMenuRequested, this, &ContentSelector::slotShowContextMenu);

    mContextMenu = new QMenu(ui->addonView);
    mContextMenu->addAction(tr("&Check Selected"), this, SLOT(slotCheckMultiSelectedItems()));
    mContextMenu->addAction(tr("&Uncheck Selected"), this, SLOT(slotUncheckMultiSelectedItems()));
    mContextMenu->addAction(tr("&Copy Path(s) to Clipboard"), this, SLOT(slotCopySelectedItemsPaths()));
}

void ContentSelectorView::ContentSelector::setNonUserContent(const QStringList& fileList)
{
    mContentModel->setNonUserContent(fileList);
}

void ContentSelectorView::ContentSelector::setProfileContent(const QStringList& fileList)
{
    clearCheckStates();

    for (const QString& filepath : fileList)
    {
        const ContentSelectorModel::EsmFile* file = mContentModel->item(filepath);
        if (file && file->isGameFile())
        {
            setGameFile(filepath);
            break;
        }
    }

    setContentList(fileList);
}

void ContentSelectorView::ContentSelector::setGameFile(const QString& filename)
{
    int index = 0;

    if (!filename.isEmpty())
    {
        const ContentSelectorModel::EsmFile* file = mContentModel->item(filename);
        index = ui->gameFileView->findText(file->fileName());

        // verify that the current index is also checked in the model
        if (!mContentModel->isChecked(file) && !mContentModel->setCheckState(file, true))
        {
            // throw error in case file not found?
            return;
        }
    }

    ui->gameFileView->setCurrentIndex(index);
}

void ContentSelectorView::ContentSelector::clearCheckStates()
{
    mContentModel->uncheckAll();
}

void ContentSelectorView::ContentSelector::setEncoding(const QString& encoding)
{
    mContentModel->setEncoding(encoding);
}

void ContentSelectorView::ContentSelector::setContentList(const QStringList& list)
{
    if (list.isEmpty())
    {
        slotCurrentGameFileIndexChanged(ui->gameFileView->currentIndex());
    }
    else
        mContentModel->setContentList(list);
}

ContentSelectorModel::ContentFileList ContentSelectorView::ContentSelector::selectedFiles() const
{
    if (!mContentModel)
        return ContentSelectorModel::ContentFileList();

    return mContentModel->checkedItems();
}

void ContentSelectorView::ContentSelector::addFiles(const QString& path, bool newfiles)
{
    mContentModel->addFiles(path, newfiles);

    // add any game files to the combo box
    for (const QString& gameFileName : mContentModel->gameFiles())
    {
        if (ui->gameFileView->findText(gameFileName) == -1)
        {
            ui->gameFileView->addItem(gameFileName);
        }
    }

    if (ui->gameFileView->currentIndex() != 0)
        ui->gameFileView->setCurrentIndex(0);

    mContentModel->uncheckAll();
}

void ContentSelectorView::ContentSelector::sortFiles()
{
    mContentModel->sortFiles();
}

bool ContentSelectorView::ContentSelector::containsDataFiles(const QString& path)
{
    return mContentModel->containsDataFiles(path);
}

void ContentSelectorView::ContentSelector::clearFiles()
{
    mContentModel->clearFiles();
}

QString ContentSelectorView::ContentSelector::currentFile() const
{
    QModelIndex currentIdx = ui->addonView->currentIndex();

    if (!currentIdx.isValid() && ui->gameFileView->currentIndex() > 0)
        return ui->gameFileView->currentText();

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
    }

    emit signalCurrentGamefileIndexChanged(index);
}

void ContentSelectorView::ContentSelector::setGameFileSelected(int index, bool selected)
{
    QString fileName = ui->gameFileView->itemText(index);
    const ContentSelectorModel::EsmFile* file = mContentModel->item(fileName);
    if (file != nullptr)
    {
        QModelIndex index2(mContentModel->indexFromItem(file));
        mContentModel->setData(index2, selected, Qt::UserRole + 1);
    }
    mContentModel->setCurrentGameFile(selected ? file : nullptr);
}

void ContentSelectorView::ContentSelector::slotAddonTableItemActivated(const QModelIndex& index)
{
    // toggles check state when an AddOn file is double clicked or activated by keyboard
    QModelIndex sourceIndex = mAddonProxyModel->mapToSource(index);

    if (!mContentModel->isEnabled(sourceIndex))
        return;

    Qt::CheckState checkState = Qt::Unchecked;

    if (mContentModel->data(sourceIndex, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        checkState = Qt::Checked;

    mContentModel->setData(sourceIndex, checkState, Qt::CheckStateRole);
}

void ContentSelectorView::ContentSelector::slotShowContextMenu(const QPoint& pos)
{
    QPoint globalPos = ui->addonView->viewport()->mapToGlobal(pos);
    mContextMenu->exec(globalPos);
}

void ContentSelectorView::ContentSelector::setCheckStateForMultiSelectedItems(Qt::CheckState checkState)
{
    const QModelIndexList selectedIndexes = ui->addonView->selectionModel()->selectedIndexes();

    QProgressDialog progressDialog("Updating content selection", {}, 0, static_cast<int>(selectedIndexes.size()));
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setValue(0);

    for (qsizetype i = 0, n = selectedIndexes.size(); i < n; ++i)
    {
        const QModelIndex sourceIndex = mAddonProxyModel->mapToSource(selectedIndexes[i]);

        if (mContentModel->data(sourceIndex, Qt::CheckStateRole).toInt() != checkState)
            mContentModel->setData(sourceIndex, checkState, Qt::CheckStateRole);

        progressDialog.setValue(static_cast<int>(i + 1));
    }
}

void ContentSelectorView::ContentSelector::slotUncheckMultiSelectedItems()
{
    setCheckStateForMultiSelectedItems(Qt::Unchecked);
}

void ContentSelectorView::ContentSelector::slotCheckMultiSelectedItems()
{
    setCheckStateForMultiSelectedItems(Qt::Checked);
}

void ContentSelectorView::ContentSelector::slotCopySelectedItemsPaths()
{
    QClipboard* clipboard = QApplication::clipboard();
    QStringList filepaths;
    for (const QModelIndex& index : ui->addonView->selectionModel()->selectedIndexes())
    {
        int row = mAddonProxyModel->mapToSource(index).row();
        const ContentSelectorModel::EsmFile* file = mContentModel->item(row);
        filepaths.push_back(file->filePath());
    }

    if (!filepaths.isEmpty())
    {
        clipboard->setText(filepaths.join("\n"));
    }
}

void ContentSelectorView::ContentSelector::slotSearchFilterTextChanged(const QString& newText)
{
    ui->addonView->setDragEnabled(newText.isEmpty());
}

void ContentSelectorView::ContentSelector::slotRowsMoved()
{
    ui->addonView->selectionModel()->clearSelection();
}
