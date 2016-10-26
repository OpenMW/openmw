//
// Created by koncord on 02.01.16.
//

#ifndef OPENMW_PLAYER_HPP
#define OPENMW_PLAYER_HPP

#include <components/esm/loadnpc.hpp>
#include <apps/openmw/mwworld/manualref.hpp>
#include <map>
#include <apps/openmw/mwmechanics/aisequence.hpp>
#include <RakNetTypes.h>

#include <components/openmw-mp/Base/BasePlayer.hpp>
#include <components/esm/custommarkerstate.hpp>


namespace mwmp
{
    struct DedicatedPlayer;

    class Players
    {
    public:
        static DedicatedPlayer *NewPlayer(RakNet::RakNetGUID guid);
        static void CreatePlayer(RakNet::RakNetGUID guid);
        static void DisconnectPlayer(RakNet::RakNetGUID guid);
        static void CleanUp();
        static DedicatedPlayer *GetPlayer(RakNet::RakNetGUID guid);
        static DedicatedPlayer *GetPlayer(const MWWorld::Ptr &ptr);
        static void Update(float dt);
    private:
        static std::map<uint64_t, DedicatedPlayer *> players;
    };

    class DedicatedPlayer : public BasePlayer
    {
        friend class Players;
    public:

        MWWorld::Ptr getPtr();
        MWWorld::Ptr getLiveCellPtr();
        MWWorld::ManualRef* getRef();
        void Move(float dt);
        void UpdateDrawState();
        void UpdateInventory();

        void updateCell();

        void updateMarker();
        void removeMarker();
        void setMarkerState(bool state);
    private:
        DedicatedPlayer(RakNet::RakNetGUID guid);
        virtual ~DedicatedPlayer();
        void UpdatePtr(MWWorld::Ptr newPtr);
        const std::string GetAnim();
        int state;
        MWWorld::ManualRef* reference;

        MWWorld::Ptr ptr;

        ESM::CustomMarker marker;
        bool markerEnabled;
    };
}
#endif //OPENMW_PLAYER_HPP
