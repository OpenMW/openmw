
#include "regionmapsubview.hpp"

#include "regionmap.hpp"

CSVWorld::RegionMapSubView::RegionMapSubView (CSMWorld::UniversalId universalId,
    CSMDoc::Document& document)
: CSVDoc::SubView (universalId)
{
    mRegionMap = new RegionMap (universalId, document, this);

    setWidget (mRegionMap);
}

void CSVWorld::RegionMapSubView::setEditLock (bool locked)
{
    mRegionMap->setEditLock (locked);
}