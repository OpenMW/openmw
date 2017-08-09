#ifndef CSM_TOOLS_PATHGRIDCHECK_H
#define CSM_TOOLS_PATHGRIDCHECK_H

#include "../world/collection.hpp"

#include "../doc/stage.hpp"

namespace CSMWorld
{
    struct Pathgrid;
    template<typename T, typename AT>
    class SubCellCollection;
}

namespace CSMTools
{
    struct Point
    {
        unsigned char mConnectionNum;
        std::vector<int> mOtherIndex;
        Point() : mConnectionNum(0), mOtherIndex(0) {}
    };

    class PathgridCheckStage : public CSMDoc::Stage
    {
        const CSMWorld::SubCellCollection<CSMWorld::Pathgrid,
              CSMWorld::IdAccessor<CSMWorld::Pathgrid> >& mPathgrids;

    public:

        PathgridCheckStage (const CSMWorld::SubCellCollection<CSMWorld::Pathgrid,
            CSMWorld::IdAccessor<CSMWorld::Pathgrid> >& pathgrids);

        virtual int setup();

        virtual void perform (int stage, CSMDoc::Messages& messages);
    };
}

#endif // CSM_TOOLS_PATHGRIDCHECK_H
