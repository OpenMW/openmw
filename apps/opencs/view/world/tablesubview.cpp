
#include "tablesubview.hpp"

#include <QVBoxLayout>
#include <QEvent>

#include "../../model/doc/document.hpp"
#include "../../model/world/tablemimedata.hpp"

#include "../filter/filterbox.hpp"
#include "table.hpp"
#include "tablebottombox.hpp"
#include "creator.hpp"

CSVWorld::TableSubView::TableSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    const CreatorFactoryBase& creatorFactory, bool sorting)
: SubView (id)
{
    QVBoxLayout *layout = new QVBoxLayout;

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    layout->addWidget (mBottom =
        new TableBottomBox (creatorFactory, document.getData(), document.getUndoStack(), id, this), 0);

    layout->insertWidget (0, mTable =
        new Table (id, mBottom->canCreateAndDelete(), sorting, document), 2);

    mFilterBox = new CSVFilter::FilterBox (document.getData(), this);

    layout->insertWidget (0, mFilterBox);

    QWidget *widget = new QWidget;

    widget->setLayout (layout);

    setWidget (widget);

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
    }
    connect (mBottom, SIGNAL (requestFocus (const std::string&)),
        mTable, SLOT (requestFocus (const std::string&)));

    connect (mFilterBox,
        SIGNAL (recordFilterChanged (boost::shared_ptr<CSMFilter::Node>)),
        mTable, SLOT (recordFilterChanged (boost::shared_ptr<CSMFilter::Node>)));

    connect(mFilterBox, SIGNAL(recordDropped(std::vector<CSMWorld::UniversalId>&, Qt::DropAction)),
        this, SLOT(createFilterRequest(std::vector<CSMWorld::UniversalId>&, Qt::DropAction)));
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

void CSVWorld::TableSubView::updateEditorSetting(const QString &settingName, const QString &settingValue)
{
    mTable->updateEditorSetting(settingName, settingValue);
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

    for (std::vector<CSMWorld::UniversalId>::iterator it = types.begin(); it != types.end(); ++it)
    {
        std::pair<std::string, std::vector<std::string> > pair(                         //splited long line
        std::make_pair(it->getId(), mTable->getColumnsWithDisplay(CSMWorld::TableMimeData::convertEnums(it->getType()))));

        filterSource.push_back(pair);
    }
    mFilterBox->createFilterRequest(filterSource, action);
}

bool CSVWorld::TableSubView::eventFilter (QObject* object, QEvent* event)
{
    if (event->type() == QEvent::Drop)
    {
        QDropEvent* drop = dynamic_cast<QDropEvent*>(event);
        const CSMWorld::TableMimeData* data = dynamic_cast<const CSMWorld::TableMimeData*>(drop->mimeData());
        bool handled = data->holdsType(CSMWorld::UniversalId::Type_Filter);
        if (handled)
        {
            mFilterBox->setRecordFilter(data->returnMatching(CSMWorld::UniversalId::Type_Filter).getId());
        }
        return handled;
    }
    return false;
}