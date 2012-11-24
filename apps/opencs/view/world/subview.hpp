#ifndef CSV_WORLD_SUBVIEW_H
#define CSV_WORLD_SUBVIEW_H

#include "../../model/world/universalid.hpp"

#include <QDockWidget>

namespace CSVWorld
{
    class SubView : public QDockWidget
    {
            Q_OBJECT

            CSMWorld::UniversalId mUniversalId;

            // not implemented
            SubView (const SubView&);
            SubView& operator= (SubView&);

        public:

            SubView (const CSMWorld::UniversalId& id);

            CSMWorld::UniversalId getUniversalId() const;
    };
}

#endif