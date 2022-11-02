#include "regionmapsubview.hpp"

#include <apps/opencs/view/doc/subview.hpp>

#include "regionmap.hpp"

CSVWorld::RegionMapSubView::RegionMapSubView(CSMWorld::UniversalId universalId, CSMDoc::Document& document)
    : CSVDoc::SubView(universalId)
{
    mRegionMap = new RegionMap(universalId, document, this);

    setWidget(mRegionMap);

    connect(mRegionMap, &RegionMap::editRequest, this, &RegionMapSubView::editRequest);
}

void CSVWorld::RegionMapSubView::setEditLock(bool locked)
{
    mRegionMap->setEditLock(locked);
}

void CSVWorld::RegionMapSubView::editRequest(const CSMWorld::UniversalId& id, const std::string& hint)
{
    focusId(id, hint);
}
