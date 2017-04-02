//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORLEVEL_HPP
#define OPENMW_PROCESSORLEVEL_HPP


#include "apps/openmw-mp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorLevel : public PlayerProcessor
    {
    public:
        ProcessorLevel()
        {
            BPP_INIT(ID_PLAYER_LEVEL)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            if (!player.creatureStats.mDead)
            {
                packet.setPlayer(&player);
                packet.Read();
                //packet.Send(&player, true);

                Script::Call<Script::CallbackIdentity("OnPlayerLevelChange")>(player.getId());
            }
        }
    };
}

#endif //OPENMW_PROCESSORLEVEL_HPP
