#ifndef CSV_WORLD_REGIONMAPSUBVIEW_H
#define CSV_WORLD_REGIONMAPSUBVIEW_H

#include "../doc/subview.hpp"

class QTableView;

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class RegionMapSubView : public CSVDoc::SubView
    {
            QTableView *mTable;

        public:

            RegionMapSubView (CSMWorld::UniversalId universalId, CSMDoc::Document& document);

            virtual void setEditLock (bool locked);

            virtual void updateUserSetting
                                (const QString& key, const QStringList &list);
    };
}

#endif
