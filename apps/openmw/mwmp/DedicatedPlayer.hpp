//
// Created by koncord on 02.01.16.
//

#ifndef OPENMW_DEDICATEDPLAYER_HPP
#define OPENMW_DEDICATEDPLAYER_HPP

#include <components/esm/custommarkerstate.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/openmw-mp/Base/BasePlayer.hpp>

#include "../mwmechanics/aisequence.hpp"

#include "../mwworld/manualref.hpp"

#include <map>
#include <RakNetTypes.h>

namespace MWMechanics
{
    class Actor;
}

namespace mwmp
{
    struct DedicatedPlayer;

    class DedicatedPlayer : public BasePlayer
    {
        friend class PlayerList;

    public:

        void update(float dt);

        void move(float dt);
        void setAnimFlags();
        void setEquipment();
        void setCell();
        void setShapeshift();

        void updateMarker();
        void removeMarker();
        void setMarkerState(bool state);

        MWWorld::Ptr getPtr();
        MWWorld::Ptr getLiveCellPtr();
        MWWorld::ManualRef* getRef();

        void setPtr(const MWWorld::Ptr& newPtr);

    private:

        DedicatedPlayer(RakNet::RakNetGUID guid);
        virtual ~DedicatedPlayer();

        int state;
        MWWorld::ManualRef* reference;

        MWWorld::Ptr ptr;

        ESM::CustomMarker marker;
        bool markerEnabled;
    };
}
#endif //OPENMW_DEDICATEDPLAYER_HPP
