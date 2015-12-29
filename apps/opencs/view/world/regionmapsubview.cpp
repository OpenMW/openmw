#include "regionmapsubview.hpp"

#include "regionmap.hpp"

CSVWorld::RegionMapSubView::RegionMapSubView (CSMWorld::UniversalId universalId,
    CSMDoc::Document& document)
: CSVDoc::SubView (universalId)
{
    mRegionMap = new RegionMap (universalId, document, this);

    setWidget (mRegionMap);

    connect (mRegionMap, SIGNAL (editRequest (const CSMWorld::UniversalId&, const std::string&)),
        this, SLOT (editRequest (const CSMWorld::UniversalId&, const std::string&)));
}

void CSVWorld::RegionMapSubView::setEditLock (bool locked)
{
    mRegionMap->setEditLock (locked);
}

void CSVWorld::RegionMapSubView::editRequest (const CSMWorld::UniversalId& id,
    const std::string& hint)
{
    focusId (id, hint);
}
