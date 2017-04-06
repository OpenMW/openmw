#ifndef OPENMW_DEDICATEDACTOR_HPP
#define OPENMW_DEDICATEDACTOR_HPP

#include <components/openmw-mp/Base/BaseActor.hpp>
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

        MWWorld::Ptr getPtr();
        void setPtr(const MWWorld::Ptr& newPtr);

    private:
        MWWorld::Ptr ptr;
    };
}

#endif //OPENMW_DEDICATEDACTOR_HPP
