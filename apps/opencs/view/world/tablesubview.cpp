#include "tablesubview.hpp"

#include <QVBoxLayout>
#include <QEvent>
#include <QHeaderView>
#include <QApplication>
#include <QDesktopWidget>
#include <QDropEvent>

#include "../../model/doc/document.hpp"
#include "../../model/world/tablemimedata.hpp"

#include "../doc/sizehint.hpp"
#include "../filter/filterbox.hpp"
#include "table.hpp"
#include "tablebottombox.hpp"
#include "creator.hpp"

CSVWorld::TableSubView::TableSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    const CreatorFactoryBase& creatorFactory, bool sorting)
: SubView (id)
{
    QVBoxLayout *layout = new QVBoxLayout;

    layout->addWidget (mBottom =
        new TableBottomBox (creatorFactory, document, id, this), 0);

    layout->insertWidget (0, mTable =
        new Table (id, mBottom->canCreateAndDelete(), sorting, document), 2);

    mFilterBox = new CSVFilter::FilterBox (document.getData(), this);

    layout->insertWidget (0, mFilterBox);

    CSVDoc::SizeHintWidget *widget = new CSVDoc::SizeHintWidget;

    widget->setLayout (layout);

    setWidget (widget);
    // prefer height of the screen and full width of the table
    const QRect rect = QApplication::desktop()->screenGeometry(this);
    int frameHeight = 40; // set a reasonable default
    QWidget *topLevel = QApplication::topLevelAt(pos());
    if (topLevel)
        frameHeight = topLevel->frameGeometry().height() - topLevel->height();
    widget->setSizeHint(QSize(mTable->horizontalHeader()->length(), rect.height()-frameHeight));

    connect (mTable, SIGNAL (editRequest (const CSMWorld::UniversalId&, const std::string&)),
        this, SLOT (editRequest (const CSMWorld::UniversalId&, const std::string&)));

    connect (mTable, SIGNAL (selectionSizeChanged (int)),
        mBottom, SLOT (selectionSizeChanged (int)));
    connect (mTable, SIGNAL (tableSizeChanged (int, int, int)),
        mBottom, SLOT (tableSizeChanged (int, int, int)));

    mTable->tableSizeUpdate();
    mTable->selectionSizeUpdate();
    mTable->viewport()->installEventFilter(this);
    mBottom->installEventFilter(this);
    mFilterBox->installEventFilter(this);

    if (mBottom->canCreateAndDelete())
    {
        connect (mTable, SIGNAL (createRequest()), mBottom, SLOT (createRequest()));

        connect (mTable, SIGNAL (cloneRequest(const CSMWorld::UniversalId&)), this,
                 SLOT(cloneRequest(const CSMWorld::UniversalId&)));

        connect (this, SIGNAL(cloneRequest(const std::string&, const CSMWorld::UniversalId::Type)),
                mBottom, SLOT(cloneRequest(const std::string&, const CSMWorld::UniversalId::Type)));

        connect (mTable, SIGNAL(touchRequest(const std::vector<CSMWorld::UniversalId>&)),
            mBottom, SLOT(touchRequest(const std::vector<CSMWorld::UniversalId>&)));

        connect (mTable, SIGNAL(extendedDeleteConfigRequest(const std::vector<std::string> &)),
            mBottom, SLOT(extendedDeleteConfigRequest(const std::vector<std::string> &)));
        connect (mTable, SIGNAL(extendedRevertConfigRequest(const std::vector<std::string> &)),
            mBottom, SLOT(extendedRevertConfigRequest(const std::vector<std::string> &)));
    }
    connect (mBottom, SIGNAL (requestFocus (const std::string&)),
        mTable, SLOT (requestFocus (const std::string&)));

    connect (mFilterBox,
        SIGNAL (recordFilterChanged (std::shared_ptr<CSMFilter::Node>)),
        mTable, SLOT (recordFilterChanged (std::shared_ptr<CSMFilter::Node>)));

    connect(mFilterBox, SIGNAL(recordDropped(std::vector<CSMWorld::UniversalId>&, Qt::DropAction)),
        this, SLOT(createFilterRequest(std::vector<CSMWorld::UniversalId>&, Qt::DropAction)));

    connect (mTable, SIGNAL (closeRequest()), this, SLOT (closeRequest()));
}

void CSVWorld::TableSubView::setEditLock (bool locked)
{
    mTable->setEditLock (locked);
    mBottom->setEditLock (locked);
}

void CSVWorld::TableSubView::editRequest (const CSMWorld::UniversalId& id, const std::string& hint)
{
    focusId (id, hint);
}

void CSVWorld::TableSubView::setStatusBar (bool show)
{
    mBottom->setStatusBar (show);
}

void CSVWorld::TableSubView::useHint (const std::string& hint)
{
    if (hint.empty())
        return;

    if (hint[0]=='f' && hint.size()>=2)
        mFilterBox->setRecordFilter (hint.substr (2));
}

void CSVWorld::TableSubView::cloneRequest(const CSMWorld::UniversalId& toClone)
{
    emit cloneRequest(toClone.getId(), toClone.getType());
}

void CSVWorld::TableSubView::createFilterRequest (std::vector< CSMWorld::UniversalId>& types, Qt::DropAction action)
{
    std::vector<std::pair<std::string, std::vector<std::string> > > filterSource;

    std::vector<std::string> refIdColumns = mTable->getColumnsWithDisplay(CSMWorld::TableMimeData::convertEnums(CSMWorld::UniversalId::Type_Referenceable));
    bool hasRefIdDisplay = !refIdColumns.empty();

    for (std::vector<CSMWorld::UniversalId>::iterator it(types.begin()); it != types.end(); ++it)
    {
        CSMWorld::UniversalId::Type type = it->getType();
        std::vector<std::string> col = mTable->getColumnsWithDisplay(CSMWorld::TableMimeData::convertEnums(type));
        if(!col.empty())
        {
            filterSource.push_back(std::make_pair(it->getId(), col));
        }

        if(hasRefIdDisplay && CSMWorld::TableMimeData::isReferencable(type))
        {
            filterSource.push_back(std::make_pair(it->getId(), refIdColumns));
        }
    }

    mFilterBox->createFilterRequest(filterSource, action);
}

bool CSVWorld::TableSubView::eventFilter (QObject* object, QEvent* event)
{
    if (event->type() == QEvent::Drop)
    {
        if (QDropEvent* drop = dynamic_cast<QDropEvent*>(event))
        {
            const CSMWorld::TableMimeData* tableMimeData = dynamic_cast<const CSMWorld::TableMimeData*>(drop->mimeData());
            if (!tableMimeData) // May happen when non-records (e.g. plain text) are dragged and dropped
                return false;

            bool handled = tableMimeData->holdsType(CSMWorld::UniversalId::Type_Filter);
            if (handled)
            {
                mFilterBox->setRecordFilter(tableMimeData->returnMatching(CSMWorld::UniversalId::Type_Filter).getId());
            }
            return handled;
        }
    }
    return false;
}

void CSVWorld::TableSubView::requestFocus (const std::string& id)
{
    mTable->requestFocus(id);
}
