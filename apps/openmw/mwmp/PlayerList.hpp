#ifndef OPENMW_PLAYERLIST_HPP
#define OPENMW_PLAYERLIST_HPP

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
}

#endif //OPENMW_PLAYERLIST_HPP
