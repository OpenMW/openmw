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

    class PlayerList
    {
    public:

        static void update(float dt);

        static void createPlayer(RakNet::RakNetGUID guid);
        static DedicatedPlayer *newPlayer(RakNet::RakNetGUID guid);

        static void disconnectPlayer(RakNet::RakNetGUID guid);
        static void cleanUp();

        static DedicatedPlayer *getPlayer(RakNet::RakNetGUID guid);
        static DedicatedPlayer *getPlayer(const MWWorld::Ptr &ptr);

        static bool isDedicatedPlayer(const MWWorld::Ptr &ptr);

    private:

        static std::map<RakNet::RakNetGUID, DedicatedPlayer *> players;
    };

    class DedicatedPlayer : public BasePlayer
    {
        friend class PlayerList;

    public:

        void move(float dt);
        void updateAnimFlags();
        void updateEquipment();
        void updateCell();

        void updateMarker();
        void removeMarker();
        void setMarkerState(bool state);
        void updateActor(MWMechanics::Actor *actor);

        MWWorld::Ptr getPtr();
        MWWorld::Ptr getLiveCellPtr();
        MWWorld::ManualRef* getRef();

    private:

        DedicatedPlayer(RakNet::RakNetGUID guid);
        virtual ~DedicatedPlayer();

        void updatePtr(MWWorld::Ptr newPtr);

        int state;
        MWWorld::ManualRef* reference;

        MWWorld::Ptr ptr;

        ESM::CustomMarker marker;
        bool markerEnabled;
    };
}
#endif //OPENMW_DEDICATEDPLAYER_HPP
