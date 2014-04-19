
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

<<<<<<< HEAD
}

void CSVWorld::RegionMapSubView::updateUserSetting
                                 (const QString &sname, const QStringList &list)
{}
=======
void CSVWorld::RegionMapSubView::editRequest (const CSMWorld::UniversalId& id,
    const std::string& hint)
{
    focusId (id, hint);
}
>>>>>>> 7eb6a2e52d659ab5ce60cfcfd1bf72ba8b1e2962
