//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERSKILL_HPP
#define OPENMW_PROCESSORPLAYERSKILL_HPP

#include "apps/openmw-mp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerSkill : public PlayerProcessor
    {
    public:
        ProcessorPlayerSkill()
        {
            BPP_INIT(ID_PLAYER_SKILL)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            if (!player.creatureStats.mDead)
            {
                packet.Read();
                //myPacket->Send(player, true);
                player.sendToLoaded(&packet);

                Script::Call<Script::CallbackIdentity("OnPlayerSkillsChange")>(player.getId());
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERSKILL_HPP
