#ifndef OPENMW_DEDICATEDACTOR_HPP
#define OPENMW_DEDICATEDACTOR_HPP

#include <components/openmw-mp/Base/BaseActor.hpp>
#include "../mwmechanics/aisequence.hpp"
#include "../mwworld/manualref.hpp"

namespace mwmp
{
    class DedicatedActor : public BaseActor
    {
    public:

        DedicatedActor();
        virtual ~DedicatedActor();

        void update();
        void move();
        void setDrawState();
        void setMovementFlags();
        void setAnimation();

        MWWorld::Ptr getPtr();
        void setPtr(const MWWorld::Ptr& newPtr);

    private:
        MWWorld::Ptr ptr;
    };
}

#endif //OPENMW_DEDICATEDACTOR_HPP
