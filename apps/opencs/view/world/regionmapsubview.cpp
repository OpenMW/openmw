
#include "regionmapsubview.hpp"

#include "regionmap.hpp"

CSVWorld::RegionMapSubView::RegionMapSubView (CSMWorld::UniversalId universalId,
    CSMDoc::Document& document)
: CSVDoc::SubView (universalId)
{
    mRegionMap = new RegionMap (document.getData().getTableModel (universalId), this);

    setWidget (mRegionMap);
}

void CSVWorld::RegionMapSubView::setEditLock (bool locked)
{
    mRegionMap->setEditLock (locked);
}