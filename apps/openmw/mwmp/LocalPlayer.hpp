//
// Created by koncord on 14.01.16.
//

#ifndef OPENMW_LOCALPLAYER_HPP
#define OPENMW_LOCALPLAYER_HPP

#include <components/openmw-mp/Base/BasePlayer.hpp>
#include <RakNetTypes.h>
#include <apps/openmw/mwmp/Networking.hpp>

namespace mwmp
{
    class LocalPlayer : public BasePlayer
    {
    public:

        LocalPlayer();
        virtual ~LocalPlayer();

        void Update();

        void updateDynamicStats(bool forceUpdate = false);
        void updatePosition(bool forceUpdate = false);
        void updateInventory(bool forceUpdate = false);
        void updateAttackState(bool forceUpdate = false);
        void updateDeadState(bool forceUpdate = false);
        void updateClassStats(bool forceUpdate = false);
        void updateCell(bool forceUpdate = false);
        void updateDrawStateAndFlags(bool forceUpdate = false);

        void setPosition();
        void setCell();

        void SetClass();
        void SendClass();
        void CharGen(int stageFirst, int stageEnd);

        bool CharGenThread(); // return true if CGStage::current == CGStage::end

        void updateChar();

        void SendAttack(char type);
        void PrepareAttack(char type, bool state);

    private:
        MWWorld::Ptr GetPlayerPtr();
        Networking *GetNetworking();

    };
}

#endif //OPENMW_LOCALPLAYER_HPP
