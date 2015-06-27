
#include "recordbuttonbar.hpp"

#include <QHBoxLayout>
#include <QToolButton>

#include "../../model/world/idtable.hpp"
#include "../../model/world/commanddispatcher.hpp"

#include "../world/tablebottombox.hpp"

CSVWorld::RecordButtonBar::RecordButtonBar (CSMWorld::IdTable& table, TableBottomBox *bottomBox,
    CSMWorld::CommandDispatcher *commandDispatcher, QWidget *parent)
: QWidget (parent), mTable (table), mBottom (bottomBox), mCommandDispatcher (commandDispatcher)
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
    QToolButton* cloneButton = new QToolButton (this);
    cloneButton->setIcon(QIcon(":/edit-clone.png"));
    cloneButton->setToolTip ("Clone record");
    buttonsLayout->addWidget(cloneButton);
    
    QToolButton* addButton = new QToolButton (this);
    addButton->setIcon(QIcon(":/add.png"));
    addButton->setToolTip ("Add new record");
    buttonsLayout->addWidget(addButton);
    
    QToolButton* deleteButton = new QToolButton (this);
    deleteButton->setIcon(QIcon(":/edit-delete.png"));
    deleteButton->setToolTip ("Delete record");
    buttonsLayout->addWidget(deleteButton);
    
    QToolButton* revertButton = new QToolButton (this);
    revertButton->setIcon(QIcon(":/edit-undo.png"));
    revertButton->setToolTip ("Revert record");
    buttonsLayout->addWidget(revertButton);
    
    setLayout (buttonsLayout);

    // disabling and connections
    if(!mBottom || !mBottom->canCreateAndDelete())
    {
        cloneButton->setDisabled (true);
        addButton->setDisabled (true);
        deleteButton->setDisabled (true);
    }
    else
    {
        connect (addButton, SIGNAL (clicked()), mBottom, SLOT (createRequest()));
        connect (cloneButton, SIGNAL (clicked()), this, SIGNAL (cloneRequest()));
    }

    connect (nextButton, SIGNAL (clicked()), this, SIGNAL (nextId()));
    connect (prevButton, SIGNAL (clicked()), this, SIGNAL (prevId()));

    if (!mCommandDispatcher)
    {
        revertButton->setDisabled (true);
        deleteButton->setDisabled (true);
    }
    else
    {
        connect (revertButton, SIGNAL (clicked()), mCommandDispatcher, SLOT (executeRevert()));
        connect (deleteButton, SIGNAL (clicked()), mCommandDispatcher, SLOT (executeDelete()));
    }
}
