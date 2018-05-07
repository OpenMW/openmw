#include "recordbuttonbar.hpp"

#include <QHBoxLayout>
#include <QToolButton>

#include "../../model/world/idtable.hpp"
#include "../../model/world/commanddispatcher.hpp"

#include "../../model/prefs/state.hpp"

#include "../world/tablebottombox.hpp"

void CSVWorld::RecordButtonBar::updateModificationButtons()
{
    bool createAndDeleteDisabled = !mBottom || !mBottom->canCreateAndDelete() || mLocked;

    mCloneButton->setDisabled (createAndDeleteDisabled);
    mAddButton->setDisabled (createAndDeleteDisabled);

    bool commandDisabled = !mCommandDispatcher || mLocked;

    mRevertButton->setDisabled (commandDisabled);
    mDeleteButton->setDisabled (commandDisabled || createAndDeleteDisabled);
}

void CSVWorld::RecordButtonBar::updatePrevNextButtons()
{
    int rows = mTable.rowCount();

    if (rows<=1)
    {
        mPrevButton->setDisabled (true);
        mNextButton->setDisabled (true);
    }
    else if (CSMPrefs::get()["General Input"]["cycle"].isTrue())
    {
        mPrevButton->setDisabled (false);
        mNextButton->setDisabled (false);
    }
    else
    {
        int row = mTable.getModelIndex (mId.getId(), 0).row();

        mPrevButton->setDisabled (row<=0);
        mNextButton->setDisabled (row>=rows-1);
    }
}

CSVWorld::RecordButtonBar::RecordButtonBar (const CSMWorld::UniversalId& id,
    CSMWorld::IdTable& table, TableBottomBox *bottomBox,
    CSMWorld::CommandDispatcher *commandDispatcher, QWidget *parent)
: QWidget (parent), mId (id), mTable (table), mBottom (bottomBox),
  mCommandDispatcher (commandDispatcher), mLocked (false)
{
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins (0, 0, 0, 0);

    // left section
    mPrevButton = new QToolButton (this);
    mPrevButton->setIcon(QIcon(":record-previous"));
    mPrevButton->setToolTip ("Switch to previous record");
    buttonsLayout->addWidget (mPrevButton, 0);

    mNextButton = new QToolButton (this);
    mNextButton->setIcon(QIcon(":/record-next"));
    mNextButton->setToolTip ("Switch to next record");
    buttonsLayout->addWidget (mNextButton, 1);

    buttonsLayout->addStretch(2);

    // optional buttons of the right section
    if (mTable.getFeatures() & CSMWorld::IdTable::Feature_Preview)
    {
        QToolButton* previewButton = new QToolButton (this);
        previewButton->setIcon(QIcon(":edit-preview"));
        previewButton->setToolTip ("Open a preview of this record");
        buttonsLayout->addWidget(previewButton);
        connect (previewButton, SIGNAL(clicked()), this, SIGNAL (showPreview()));
    }

    if (mTable.getFeatures() & CSMWorld::IdTable::Feature_View)
    {
        QToolButton* viewButton = new QToolButton (this);
        viewButton->setIcon(QIcon(":/cell.png"));
        viewButton->setToolTip ("Open a scene view of the cell this record is located in");
        buttonsLayout->addWidget(viewButton);
        connect (viewButton, SIGNAL(clicked()), this, SIGNAL (viewRecord()));
    }

    // right section
    mCloneButton = new QToolButton (this);
    mCloneButton->setIcon(QIcon(":edit-clone"));
    mCloneButton->setToolTip ("Clone record");
    buttonsLayout->addWidget(mCloneButton);

    mAddButton = new QToolButton (this);
    mAddButton->setIcon(QIcon(":edit-add"));
    mAddButton->setToolTip ("Add new record");
    buttonsLayout->addWidget(mAddButton);

    mDeleteButton = new QToolButton (this);
    mDeleteButton->setIcon(QIcon(":edit-delete"));
    mDeleteButton->setToolTip ("Delete record");
    buttonsLayout->addWidget(mDeleteButton);

    mRevertButton = new QToolButton (this);
    mRevertButton->setIcon(QIcon(":edit-undo"));
    mRevertButton->setToolTip ("Revert record");
    buttonsLayout->addWidget(mRevertButton);

    setLayout (buttonsLayout);

    // connections
    if(mBottom && mBottom->canCreateAndDelete())
    {
        connect (mAddButton, SIGNAL (clicked()), mBottom, SLOT (createRequest()));
        connect (mCloneButton, SIGNAL (clicked()), this, SLOT (cloneRequest()));
    }

    connect (mNextButton, SIGNAL (clicked()), this, SLOT (nextId()));
    connect (mPrevButton, SIGNAL (clicked()), this, SLOT (prevId()));

    if (mCommandDispatcher)
    {
        connect (mRevertButton, SIGNAL (clicked()), mCommandDispatcher, SLOT (executeRevert()));
        connect (mDeleteButton, SIGNAL (clicked()), mCommandDispatcher, SLOT (executeDelete()));
    }

    connect (&mTable, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (rowNumberChanged (const QModelIndex&, int, int)));
    connect (&mTable, SIGNAL (rowsRemoved (const QModelIndex&, int, int)),
        this, SLOT (rowNumberChanged (const QModelIndex&, int, int)));

    connect (&CSMPrefs::State::get(), SIGNAL (settingChanged (const CSMPrefs::Setting *)),
        this, SLOT (settingChanged (const CSMPrefs::Setting *)));

    updateModificationButtons();
    updatePrevNextButtons();
}

void CSVWorld::RecordButtonBar::setEditLock (bool locked)
{
    mLocked = locked;
    updateModificationButtons();
}

void CSVWorld::RecordButtonBar::universalIdChanged (const CSMWorld::UniversalId& id)
{
    mId = id;
    updatePrevNextButtons();
}

void CSVWorld::RecordButtonBar::settingChanged (const CSMPrefs::Setting *setting)
{
    if (*setting=="General Input/cycle")
        updatePrevNextButtons();
}

void CSVWorld::RecordButtonBar::cloneRequest()
{
    if (mBottom)
    {
        int typeColumn = mTable.findColumnIndex (CSMWorld::Columns::ColumnId_RecordType);

        QModelIndex typeIndex = mTable.getModelIndex (mId.getId(), typeColumn);
        CSMWorld::UniversalId::Type type = static_cast<CSMWorld::UniversalId::Type> (
            mTable.data (typeIndex).toInt());

        mBottom->cloneRequest (mId.getId(), type);
    }
}

void CSVWorld::RecordButtonBar::nextId()
{
    int newRow = mTable.getModelIndex (mId.getId(), 0).row() + 1;

    if (newRow >= mTable.rowCount())
    {
        if (CSMPrefs::get()["General Input"]["cycle"].isTrue())
            newRow = 0;
        else
            return;
    }

    emit switchToRow (newRow);
}

void CSVWorld::RecordButtonBar::prevId()
{
    int newRow = mTable.getModelIndex (mId.getId(), 0).row() - 1;

    if (newRow < 0)
    {
        if (CSMPrefs::get()["General Input"]["cycle"].isTrue())
            newRow = mTable.rowCount()-1;
        else
            return;
    }

    emit switchToRow (newRow);
}

void CSVWorld::RecordButtonBar::rowNumberChanged (const QModelIndex& parent, int start, int end)
{
    updatePrevNextButtons();
}
