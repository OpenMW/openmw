
#include "regionmapsubview.hpp"

#include "regionmap.hpp"
#include "../../model/settings/usersettings.hpp"

CSVWorld::RegionMapSubView::RegionMapSubView (CSMWorld::UniversalId universalId,
    CSMDoc::Document& document)
: CSVDoc::SubView (universalId)
{
    mRegionMap = new RegionMap (universalId, document, this);

    int minWidth = 325;
    if(CSMSettings::UserSettings::instance().hasSettingDefinitions("SubView/minimum width"))
        minWidth = CSMSettings::UserSettings::instance().settingValue("SubView/minimum width").toInt();
    else
        CSMSettings::UserSettings::instance().setDefinitions("SubView/minimum width", (QStringList() << "minWidth"));
    mRegionMap->setMinimumWidth(minWidth);

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
