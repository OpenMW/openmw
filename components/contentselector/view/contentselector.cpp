#include "contentselector.hpp"

#include "../model/contentmodel.hpp"
#include "../model/esmfile.hpp"

#include <QSortFilterProxyModel>

#include <QDebug>
#include <QMenu>
#include <QContextMenuEvent>
#include <QGridLayout>
#include <assert.h>

#include "../../../apps/opencs/view/doc/filewidget.hpp"
#include "../../../apps/opencs/view/doc/adjusterwidget.hpp"

ContentSelectorView::ContentSelector *ContentSelectorView::ContentSelector::mInstance = 0;
QStringList ContentSelectorView::ContentSelector::mFilePaths;

void ContentSelectorView::ContentSelector::configure(QWidget *subject, unsigned char flags)
{
    assert(!mInstance);
    mInstance = new ContentSelector (subject, flags);
}

ContentSelectorView::ContentSelector& ContentSelectorView::ContentSelector::instance()
{
    assert(mInstance);
    return *mInstance;
}

ContentSelectorView::ContentSelector::ContentSelector(QWidget *parent, unsigned char flags) :
    QWidget(parent), mFlags (flags),
    mAdjusterWidget (0), mFileWidget (0)
{
    ui.setupUi (this);

    parent->setLayout(new QGridLayout());
    parent->layout()->addWidget(this);

    buildContentModel();
    buildGameFileView();
    buildAddonView();
    buildProfilesView();
    buildNewAddonView();
    buildLoadAddonView();

    /*
    //mContentModel->sort(3);  // Sort by date accessed

*/
}

bool ContentSelectorView::ContentSelector::isFlagged(SelectorFlags flag) const
{
    return (mFlags & flag);
}

void ContentSelectorView::ContentSelector::buildContentModel()
{
    if (!isFlagged (Flag_Content))
            return;

    mContentModel = new ContentSelectorModel::ContentModel();

    if (mFilePaths.size()>0)
    {
        foreach (const QString &path, mFilePaths)
            mContentModel->addFiles(path);

        mFilePaths.clear();
    }

    ui.gameFileView->setCurrentIndex(-1);
    mContentModel->uncheckAll();
}

void ContentSelectorView::ContentSelector::buildGameFileView()
{
    if (!isFlagged (Flag_Content))
    {
        ui.gameFileView->setVisible(false);
        return;
    }

    mGameFileProxyModel = new QSortFilterProxyModel(this);
    mGameFileProxyModel->setFilterRegExp(QString::number((int)ContentSelectorModel::ContentType_GameFile));
    mGameFileProxyModel->setFilterRole (Qt::UserRole);
    mGameFileProxyModel->setSourceModel (mContentModel);

    ui.gameFileView->setPlaceholderText(QString("Select a game file..."));
    ui.gameFileView->setModel(mGameFileProxyModel);

    connect(ui.gameFileView, SIGNAL(currentIndexChanged(int)), this, SLOT (slotCurrentGameFileIndexChanged(int)));

    ui.gameFileView->setCurrentIndex(-1);
}

void ContentSelectorView::ContentSelector::buildAddonView()
{
    if (!isFlagged (Flag_Content))
    {
        ui.addonView->setVisible(false);
        return;
    }

    mAddonProxyModel = new QSortFilterProxyModel(this);
    mAddonProxyModel->setFilterRegExp (QString::number((int)ContentSelectorModel::ContentType_Addon));
    mAddonProxyModel->setFilterRole (Qt::UserRole);
    mAddonProxyModel->setDynamicSortFilter (true);
    mAddonProxyModel->setSourceModel (mContentModel);

    ui.addonView->setModel(mAddonProxyModel);

    connect(ui.addonView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotAddonTableItemClicked(const QModelIndex &)));
}

void ContentSelectorView::ContentSelector::buildProfilesView()
{
    if (!isFlagged (Flag_Profile))
    {
        ui.profileGroupBox->setVisible(false);
        return;
    }

    ui.profilesComboBox->setPlaceholderText(QString("Select a profile..."));
    connect(ui.profilesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentProfileIndexChanged(int)));
}

void ContentSelectorView::ContentSelector::buildLoadAddonView()
{
    if (!isFlagged (Flag_LoadAddon))
    {
        ui.projectGroupBox->setVisible (false);
        return;
    }

    ui.projectCreateButton->setVisible (false);
   // ui.projectButtonBox->setStandardButtons(QDialogButtonBox::Open | QDialogButtonBox::Cancel);
    ui.projectGroupBox->setTitle ("");

    connect(ui.projectButtonBox, SIGNAL(accepted()), this, SIGNAL(accepted()));
    connect(ui.projectButtonBox, SIGNAL(rejected()), this, SIGNAL(rejected()));
}

void ContentSelectorView::ContentSelector::buildNewAddonView()
{
    if (!isFlagged (Flag_NewAddon))
    {
        ui.profileGroupBox->setVisible (false);
        return;
    }

    mFileWidget = new CSVDoc::FileWidget (this);
    mAdjusterWidget = new CSVDoc::AdjusterWidget (this);

    mFileWidget->setType(true);
    mFileWidget->extensionLabelIsVisible(false);

    ui.fileWidgetFrame->layout()->addWidget(mFileWidget);
    ui.adjusterWidgetFrame->layout()->addWidget(mAdjusterWidget);

    ui.projectButtonBox->setStandardButtons(QDialogButtonBox::Cancel);
    ui.projectButtonBox->addButton(ui.projectCreateButton, QDialogButtonBox::ActionRole);

    connect (mFileWidget, SIGNAL (nameChanged (const QString&, bool)),
        mAdjusterWidget, SLOT (setName (const QString&, bool)));

    connect (mAdjusterWidget, SIGNAL (stateChanged(bool)), this, SLOT (slotUpdateCreateButton(bool)));

    connect(ui.projectCreateButton, SIGNAL(clicked()), this, SIGNAL(accepted()));
    connect(ui.projectButtonBox, SIGNAL(rejected()), this, SIGNAL(rejected()));
}

QString ContentSelectorView::ContentSelector::filename() const
{
    QString filepath = "";

    if (mAdjusterWidget)
        filepath = QString::fromAscii(mAdjusterWidget->getPath().c_str());

    return filepath;
}

QStringList ContentSelectorView::ContentSelector::selectedFiles() const
{
    QStringList filePaths;

    if (mContentModel)
    {
        foreach (ContentSelectorModel::EsmFile *file, mContentModel->checkedItems())
            filePaths.append(file->path());
    }

    return filePaths;
}


void ContentSelectorView::ContentSelector::addFiles(const QString &path)
{
    // if the model hasn't been instantiated, queue the path
    if (!mInstance)
        mFilePaths.append(path);
    else
    {
        mInstance->mContentModel->addFiles(path);
        mInstance->ui.gameFileView->setCurrentIndex(-1);
        mInstance->mContentModel->uncheckAll();
    }
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
    emit signalProfileChanged(index);
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

    slotUpdateCreateButton(true);
}

void ContentSelectorView::ContentSelector::slotAddonTableItemClicked(const QModelIndex &index)
{
    QAbstractItemModel *const model = ui.addonView->model();

    if (model->data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        model->setData(index, Qt::Checked, Qt::CheckStateRole);
    else
        model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
}

void ContentSelectorView::ContentSelector::slotUpdateOpenButton(const QStringList &items)
{
    QPushButton *openButton = ui.projectButtonBox->button(QDialogButtonBox::Open);

    if (!openButton)
        return;

    openButton->setEnabled(!items.isEmpty());
}

void ContentSelectorView::ContentSelector::slotUpdateCreateButton(bool)
{
    //enable only if a game file is selected and the adjuster widget is non-empty
    bool validGameFile = (ui.gameFileView->currentIndex() != -1);
    bool validFilename = false;

    if (mAdjusterWidget)
        validFilename = mAdjusterWidget->isValid();

    ui.projectCreateButton->setEnabled(validGameFile && validFilename);
}
