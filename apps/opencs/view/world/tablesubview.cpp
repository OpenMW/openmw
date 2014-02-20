
#include "tablesubview.hpp"

#include <QVBoxLayout>

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
        new Table (id, document.getData(), document.getUndoStack(), mBottom->canCreateAndDelete(), sorting, document), 2);

    CSVFilter::FilterBox *filterBox = new CSVFilter::FilterBox (document.getData(), this);

    layout->insertWidget (0, filterBox);

    QWidget *widget = new QWidget;

    widget->setLayout (layout);

    setWidget (widget);

    connect (mTable, SIGNAL (editRequest (int)), this, SLOT (editRequest (int)));

    connect (mTable, SIGNAL (selectionSizeChanged (int)),
        mBottom, SLOT (selectionSizeChanged (int)));
    connect (mTable, SIGNAL (tableSizeChanged (int, int, int)),
        mBottom, SLOT (tableSizeChanged (int, int, int)));

    mTable->tableSizeUpdate();
    mTable->selectionSizeUpdate();

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

    connect (filterBox,
        SIGNAL (recordFilterChanged (boost::shared_ptr<CSMFilter::Node>)),
        mTable, SLOT (recordFilterChanged (boost::shared_ptr<CSMFilter::Node>)));

    connect(filterBox, SIGNAL(recordDropped(std::vector<CSMWorld::UniversalId>&)),
        this, SLOT(createFilterRequest(std::vector<CSMWorld::UniversalId>&)));

    connect(this, SIGNAL(createFilterRequest(std::vector<std::pair<std::string, std::vector<std::string> > >&)),
            filterBox, SIGNAL(createFilterRequest(std::vector<std::pair<std::string, std::vector<std::string> > >&)));
}

void CSVWorld::TableSubView::setEditLock (bool locked)
{
    mTable->setEditLock (locked);
    mBottom->setEditLock (locked);
}

void CSVWorld::TableSubView::editRequest (int row)
{
    focusId (mTable->getUniversalId (row));
}

void CSVWorld::TableSubView::updateEditorSetting(const QString &settingName, const QString &settingValue)
{
    mTable->updateEditorSetting(settingName, settingValue);
}

void CSVWorld::TableSubView::setStatusBar (bool show)
{
    mBottom->setStatusBar (show);
}

void CSVWorld::TableSubView::cloneRequest(const CSMWorld::UniversalId& toClone)
{
    emit cloneRequest(toClone.getId(), toClone.getType());
}

void CSVWorld::TableSubView::createFilterRequest (std::vector< CSMWorld::UniversalId>& types)
{
    std::vector<std::pair<std::string, std::vector<std::string> > > filterSource;

    for (std::vector<CSMWorld::UniversalId>::iterator it = types.begin(); it != types.end(); ++it)
    {
        std::pair<std::string, std::vector<std::string> > pair(                         //splited long line
        std::make_pair(it->getId(), mTable->getColumnsWithDisplay(CSMWorld::TableMimeData::convertEnums(it->getType()))));

        filterSource.push_back(pair);
    }
    emit createFilterRequest(filterSource);
}