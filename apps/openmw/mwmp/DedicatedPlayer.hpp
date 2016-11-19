//
// Created by koncord on 02.01.16.
//

#ifndef OPENMW_PLAYER_HPP
#define OPENMW_PLAYER_HPP

#include <components/esm/loadnpc.hpp>
#include "../mwworld/manualref.hpp"
#include <map>
#include "../mwmechanics/aisequence.hpp"
#include <RakNetTypes.h>

#include <components/openmw-mp/Base/BasePlayer.hpp>
#include <components/esm/custommarkerstate.hpp>

namespace MWMechanics
{
    class Actor;
}

namespace mwmp
{
    struct DedicatedPlayer;

    class Players
    {
    public:
        static DedicatedPlayer *newPlayer(RakNet::RakNetGUID guid);
        static void createPlayer(RakNet::RakNetGUID guid);
        static void disconnectPlayer(RakNet::RakNetGUID guid);
        static void cleanUp();
        static DedicatedPlayer *getPlayer(RakNet::RakNetGUID guid);
        static DedicatedPlayer *getPlayer(const MWWorld::Ptr &ptr);
        static void update(float dt);
    private:
        static std::map<RakNet::RakNetGUID, DedicatedPlayer *> players;
    };

    class DedicatedPlayer : public BasePlayer
    {
        friend class Players;
    public:

        MWWorld::Ptr getPtr();
        MWWorld::Ptr getLiveCellPtr();
        MWWorld::ManualRef* getRef();
        void move(float dt);
        void updateDrawState();
        void updateEquipment();

        void updateCell();

        void updateMarker();
        void removeMarker();
        void setMarkerState(bool state);
        void updateActor(MWMechanics::Actor *actor);
    private:
        DedicatedPlayer(RakNet::RakNetGUID guid);
        virtual ~DedicatedPlayer();
        void updatePtr(MWWorld::Ptr newPtr);
        const std::string getAnim();
        int state;
        MWWorld::ManualRef* reference;

        MWWorld::Ptr ptr;

        ESM::CustomMarker marker;
        bool markerEnabled;
    };
}
#endif //OPENMW_PLAYER_HPP
