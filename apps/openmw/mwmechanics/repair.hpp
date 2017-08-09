#ifndef OPENMW_MWMECHANICS_REPAIR_H
#define OPENMW_MWMECHANICS_REPAIR_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{

    class Repair
    {
    public:
        void setTool (const MWWorld::Ptr& tool) { mTool = tool; }
        MWWorld::Ptr getTool() { return mTool; }

        void repair (const MWWorld::Ptr& itemToRepair);

    private:
        MWWorld::Ptr mTool;
    };

}

#endif
