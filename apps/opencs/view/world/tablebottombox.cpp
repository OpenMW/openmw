#include "tablebottombox.hpp"

#include <sstream>

#include <QStatusBar>
#include <QStackedLayout>
#include <QLabel>
#include <QEvent>
#include <QKeyEvent>

#include "creator.hpp"

void CSVWorld::TableBottomBox::updateSize()
{
    // Make sure that the size of the bottom box is determined by the currently visible widget
    for (int i = 0; i < mLayout->count(); ++i)
    {
        QSizePolicy::Policy verPolicy = QSizePolicy::Ignored;
        if (mLayout->widget(i) == mLayout->currentWidget())
        {
            verPolicy = QSizePolicy::Expanding;
        }
        mLayout->widget(i)->setSizePolicy(QSizePolicy::Expanding, verPolicy);
    }
}

void CSVWorld::TableBottomBox::updateStatus()
{
    if (mShowStatusBar)
    {
        if (!mStatusMessage.isEmpty())
        {
            mStatus->setText (mStatusMessage);
            return;
        }

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

        if (mHasPosition)
        {
            if (!first)
                stream << " -- ";

            stream << "(" << mRow << ", " << mColumn << ")";
        }

        mStatus->setText (QString::fromUtf8 (stream.str().c_str()));
    }
}

void CSVWorld::TableBottomBox::extendedConfigRequest(CSVWorld::ExtendedCommandConfigurator::Mode mode,
                                                     const std::vector<std::string> &selectedIds)
{
    mExtendedConfigurator->configure (mode, selectedIds);
    mLayout->setCurrentWidget (mExtendedConfigurator);
    mEditMode = EditMode_ExtendedConfig;
    setVisible (true);
    mExtendedConfigurator->setFocus();
}

CSVWorld::TableBottomBox::TableBottomBox (const CreatorFactoryBase& creatorFactory,
                                          CSMDoc::Document& document,
                                          const CSMWorld::UniversalId& id,
                                          QWidget *parent)
: QWidget (parent), mShowStatusBar (false), mEditMode(EditMode_None), mHasPosition(false), mRow(0), mColumn(0)
{
    for (int i=0; i<4; ++i)
        mStatusCount[i] = 0;

    setVisible (false);

    mLayout = new QStackedLayout;
    mLayout->setContentsMargins (0, 0, 0, 0);
    connect (mLayout, SIGNAL (currentChanged (int)), this, SLOT (currentWidgetChanged (int)));

    mStatus = new QLabel;

    mStatusBar = new QStatusBar(this);

    mStatusBar->addWidget (mStatus);

    mLayout->addWidget (mStatusBar);

    setLayout (mLayout);

    mCreator = creatorFactory.makeCreator (document, id);

    if (mCreator)
    {
        mCreator->installEventFilter(this);
        mLayout->addWidget (mCreator);

        connect (mCreator, SIGNAL (done()), this, SLOT (requestDone()));

        connect (mCreator, SIGNAL (requestFocus (const std::string&)),
            this, SIGNAL (requestFocus (const std::string&)));
    }

    mExtendedConfigurator = new ExtendedCommandConfigurator (document, id, this);
    mExtendedConfigurator->installEventFilter(this);
    mLayout->addWidget (mExtendedConfigurator);
    connect (mExtendedConfigurator, SIGNAL (done()), this, SLOT (requestDone()));

    updateSize();
}

void CSVWorld::TableBottomBox::setEditLock (bool locked)
{
    if (mCreator)
        mCreator->setEditLock (locked);
    mExtendedConfigurator->setEditLock (locked);
}

CSVWorld::TableBottomBox::~TableBottomBox()
{
    delete mCreator;
}

bool CSVWorld::TableBottomBox::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape)
        {
            requestDone();
            return true;
        }
    }
    return QWidget::eventFilter(object, event);
}

void CSVWorld::TableBottomBox::setStatusBar (bool show)
{
    if (show!=mShowStatusBar)
    {
        setVisible (show || (mEditMode != EditMode_None));

        mShowStatusBar = show;

        if (show)
            updateStatus();
    }
}

bool CSVWorld::TableBottomBox::canCreateAndDelete() const
{
    return mCreator;
}

void CSVWorld::TableBottomBox::requestDone()
{
    if (!mShowStatusBar)
        setVisible (false);
    else
        updateStatus();

    mLayout->setCurrentWidget (mStatusBar);
    mEditMode = EditMode_None;
}

void CSVWorld::TableBottomBox::currentWidgetChanged(int /*index*/)
{
    updateSize();
}

void CSVWorld::TableBottomBox::setStatusMessage (const QString& message)
{
    mStatusMessage = message;
    updateStatus();
}

void CSVWorld::TableBottomBox::selectionSizeChanged (int size)
{
    if (mStatusCount[3]!=size)
    {
        mStatusMessage = "";
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
    {
        mStatusMessage = "";
        updateStatus();
    }
}

void CSVWorld::TableBottomBox::positionChanged (int row, int column)
{
    mRow = row;
    mColumn = column;
    mHasPosition = true;
    updateStatus();
}

void CSVWorld::TableBottomBox::noMorePosition()
{
    mHasPosition = false;
    updateStatus();
}

void CSVWorld::TableBottomBox::createRequest()
{
    mCreator->reset();
    mCreator->toggleWidgets(true);
    mLayout->setCurrentWidget (mCreator);
    setVisible (true);
    mEditMode = EditMode_Creation;
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
    mEditMode = EditMode_Creation;
    mCreator->focus();
}

void CSVWorld::TableBottomBox::touchRequest(const std::vector<CSMWorld::UniversalId>& ids)
{
    mCreator->touch(ids);
}

void CSVWorld::TableBottomBox::extendedDeleteConfigRequest(const std::vector<std::string> &selectedIds)
{
    extendedConfigRequest(ExtendedCommandConfigurator::Mode_Delete, selectedIds);
}

void CSVWorld::TableBottomBox::extendedRevertConfigRequest(const std::vector<std::string> &selectedIds)
{
    extendedConfigRequest(ExtendedCommandConfigurator::Mode_Revert, selectedIds);
}
