#ifndef CSM_WOLRD_PATHGRIDPOINTSWRAP_H
#define CSM_WOLRD_PATHGRIDPOINTSWRAP_H

#include <components/esm/loadpgrd.hpp>

#include "nestedtablewrapper.hpp"

namespace CSMWorld
{
    struct PathgridPointsWrap : public NestedTableWrapperBase
    {
        ESM::Pathgrid mRecord;

        PathgridPointsWrap(ESM::Pathgrid pathgrid)
            : mRecord(pathgrid) {}

        virtual ~PathgridPointsWrap() {}

        virtual int size() const
        {
            return mRecord.mPoints.size(); // used in IdTree::setNestedTable()
        }
    };
}

#endif // CSM_WOLRD_PATHGRIDPOINTSWRAP_H
