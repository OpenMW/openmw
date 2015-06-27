
#include "recordbuttonbar.hpp"

#include <QHBoxLayout>
#include <QToolButton>

#include "../../model/world/idtable.hpp"
#include "../../model/world/commanddispatcher.hpp"

#include "../world/tablebottombox.hpp"

void CSVWorld::RecordButtonBar::updateModificationButtons()
{
    bool createAndDeleteDisabled = !mBottom || !mBottom->canCreateAndDelete() || mLocked;

    mCloneButton->setDisabled (createAndDeleteDisabled);
    mAddButton->setDisabled (createAndDeleteDisabled);
    mDeleteButton->setDisabled (createAndDeleteDisabled);

    bool commandDisabled = !mCommandDispatcher || mLocked;
    
    mRevertButton->setDisabled (commandDisabled);
    mDeleteButton->setDisabled (commandDisabled);
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
    QToolButton* prevButton = new QToolButton (this);
    prevButton->setIcon(QIcon(":/go-previous.png"));
    prevButton->setToolTip ("Switch to previous record");
    buttonsLayout->addWidget (prevButton, 0);
    
    QToolButton* nextButton = new QToolButton (this);
    nextButton->setIcon(QIcon(":/go-next.png"));
    nextButton->setToolTip ("Switch to next record");
    buttonsLayout->addWidget (nextButton, 1);
    
    buttonsLayout->addStretch(2);

    // optional buttons of the right section
    if (mTable.getFeatures() & CSMWorld::IdTable::Feature_Preview)
    {
        QToolButton* previewButton = new QToolButton (this);
        previewButton->setIcon(QIcon(":/edit-preview.png"));
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
    mCloneButton->setIcon(QIcon(":/edit-clone.png"));
    mCloneButton->setToolTip ("Clone record");
    buttonsLayout->addWidget(mCloneButton);
    
    mAddButton = new QToolButton (this);
    mAddButton->setIcon(QIcon(":/add.png"));
    mAddButton->setToolTip ("Add new record");
    buttonsLayout->addWidget(mAddButton);
    
    mDeleteButton = new QToolButton (this);
    mDeleteButton->setIcon(QIcon(":/edit-delete.png"));
    mDeleteButton->setToolTip ("Delete record");
    buttonsLayout->addWidget(mDeleteButton);
    
    mRevertButton = new QToolButton (this);
    mRevertButton->setIcon(QIcon(":/edit-undo.png"));
    mRevertButton->setToolTip ("Revert record");
    buttonsLayout->addWidget(mRevertButton);
    
    setLayout (buttonsLayout);

    // connections
    if(mBottom && mBottom->canCreateAndDelete())
    {
        connect (mAddButton, SIGNAL (clicked()), mBottom, SLOT (createRequest()));
        connect (mCloneButton, SIGNAL (clicked()), this, SLOT (cloneRequest()));
    }

    connect (nextButton, SIGNAL (clicked()), this, SIGNAL (nextId()));
    connect (prevButton, SIGNAL (clicked()), this, SIGNAL (prevId()));

    if (mCommandDispatcher)
    {
        connect (mRevertButton, SIGNAL (clicked()), mCommandDispatcher, SLOT (executeRevert()));
        connect (mDeleteButton, SIGNAL (clicked()), mCommandDispatcher, SLOT (executeDelete()));
    }

    updateModificationButtons();
}

void CSVWorld::RecordButtonBar::setEditLock (bool locked)
{
    mLocked = locked;
    updateModificationButtons();
}

void CSVWorld::RecordButtonBar::universalIdChanged (const CSMWorld::UniversalId& id)
{
    mId = id;
}

void CSVWorld::RecordButtonBar::cloneRequest()
{
    if (mBottom)
    {
        int typeColumn = mTable.searchColumnIndex (CSMWorld::Columns::ColumnId_RecordType);

        if (typeColumn!=-1)
        {
            QModelIndex typeIndex = mTable.getModelIndex (mId.getId(), typeColumn);
            CSMWorld::UniversalId::Type type = static_cast<CSMWorld::UniversalId::Type> (
                mTable.data (typeIndex).toInt());
            
            mBottom->cloneRequest (mId.getId(), type);
        }
    }
}
