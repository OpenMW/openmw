#ifndef CSV_WORLD_REGIONMAPSUBVIEW_H
#define CSV_WORLD_REGIONMAPSUBVIEW_H

#include <string>

#include "../doc/subview.hpp"

#include <apps/opencs/model/world/universalid.hpp>

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class RegionMap;

    class RegionMapSubView : public CSVDoc::SubView
    {
        Q_OBJECT

        RegionMap* mRegionMap;

    public:
        RegionMapSubView(CSMWorld::UniversalId universalId, CSMDoc::Document& document);

        void setEditLock(bool locked) override;

    private slots:

        void editRequest(const CSMWorld::UniversalId& id, const std::string& hint);
    };
}

#endif
