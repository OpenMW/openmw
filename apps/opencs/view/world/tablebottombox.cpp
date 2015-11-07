
#include "tablebottombox.hpp"

#include <sstream>

#include <QStatusBar>
#include <QStackedLayout>
#include <QLabel>

#include "creator.hpp"

void CSVWorld::TableBottomBox::updateStatus()
{
    if (mShowStatusBar)
    {
        static const char *sLabels[4] = { "record", "deleted", "touched", "selected" };
        static const char *sLabelsPlural[4] = { "records", "deleted", "touched", "selected" };

        std::ostringstream stream;

        bool first = true;

        for (int i=0; i<4; ++i)
        {
            if (mStatusCount[i]>0)
            {
                if (first)
                    first = false;
                else
                    stream << ", ";

                stream
                    << mStatusCount[i] << ' '
                    << (mStatusCount[i]==1 ? sLabels[i] : sLabelsPlural[i]);
            }
        }

        mStatus->setText (QString::fromUtf8 (stream.str().c_str()));
    }
}

CSVWorld::TableBottomBox::TableBottomBox (const CreatorFactoryBase& creatorFactory,
    CSMWorld::Data& data, QUndoStack& undoStack, const CSMWorld::UniversalId& id, QWidget *parent)
: QWidget (parent), mShowStatusBar (false), mCreating (false)
{
    for (int i=0; i<4; ++i)
        mStatusCount[i] = 0;

    setVisible (false);

    mLayout = new QStackedLayout;
    mLayout->setContentsMargins (0, 0, 0, 0);

    mStatus = new QLabel;

    mStatusBar = new QStatusBar;

    mStatusBar->addWidget (mStatus);

    mLayout->addWidget (mStatusBar);

    setLayout (mLayout);

    mCreator = creatorFactory.makeCreator (data, undoStack, id);

    if (mCreator)
    {
        mLayout->addWidget (mCreator);

        connect (mCreator, SIGNAL (done()), this, SLOT (createRequestDone()));

        connect (mCreator, SIGNAL (requestFocus (const std::string&)),
            this, SIGNAL (requestFocus (const std::string&)));
    }
}

void CSVWorld::TableBottomBox::setEditLock (bool locked)
{
    if (mCreator)
        mCreator->setEditLock (locked);
}

CSVWorld::TableBottomBox::~TableBottomBox()
{
    delete mCreator;
}

void CSVWorld::TableBottomBox::setStatusBar (bool show)
{
    if (show!=mShowStatusBar)
    {
        setVisible (show || mCreating);

        mShowStatusBar = show;

        if (show)
            updateStatus();
    }
}

bool CSVWorld::TableBottomBox::canCreateAndDelete() const
{
    return mCreator;
}

void CSVWorld::TableBottomBox::createRequestDone()
{
    if (!mShowStatusBar)
        setVisible (false);
    else
        updateStatus();

    mLayout->setCurrentWidget (mStatusBar);

    mCreating = false;
}

void CSVWorld::TableBottomBox::selectionSizeChanged (int size)
{
    if (mStatusCount[3]!=size)
    {
        mStatusCount[3] = size;
        updateStatus();
    }
}

void CSVWorld::TableBottomBox::tableSizeChanged (int size, int deleted, int modified)
{
    bool changed = false;

    if (mStatusCount[0]!=size)
    {
        mStatusCount[0] = size;
        changed = true;
    }

    if (mStatusCount[1]!=deleted)
    {
        mStatusCount[1] = deleted;
        changed = true;
    }

    if (mStatusCount[2]!=modified)
    {
        mStatusCount[2] = modified;
        changed = true;
    }

    if (changed)
        updateStatus();
}

void CSVWorld::TableBottomBox::createRequest()
{
    mCreator->reset();
    mCreator->toggleWidgets(true);
    mLayout->setCurrentWidget (mCreator);
    setVisible (true);
    mCreating = true;
    mCreator->focus();
}

void CSVWorld::TableBottomBox::cloneRequest(const std::string& id, 
                                            const CSMWorld::UniversalId::Type type) 
{
    mCreator->reset();
    mCreator->cloneMode(id, type);
    mLayout->setCurrentWidget(mCreator);
    mCreator->toggleWidgets(false);
    setVisible (true);
    mCreating = true;
    mCreator->focus();
}
