//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERATTACK_HPP
#define OPENMW_PROCESSORPLAYERATTACK_HPP

#include "apps/openmw/mwmp/Main.hpp"
#include "../PlayerProcessor.hpp"
#include "apps/openmw/mwmp/MechanicsHelper.hpp"
#include "apps/openmw/mwbase/world.hpp"
#include "apps/openmw/mwworld/containerstore.hpp"
#include "apps/openmw/mwworld/inventorystore.hpp"
#include "apps/openmw/mwmechanics/combat.hpp"

#include "apps/openmw/mwbase/environment.hpp"

namespace mwmp
{
    class ProcessorPlayerAttack : public PlayerProcessor
    {
    public:
        ProcessorPlayerAttack()
        {
            BPP_INIT(ID_PLAYER_ATTACK)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (player != 0)
                Main::get().getMechanicsHelper()->processAttack(player->attack, static_cast<DedicatedPlayer*>(player)->getPtr());
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERATTACK_HPP
