//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERLEVEL_HPP
#define OPENMW_PROCESSORPLAYERLEVEL_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerLevel : public PlayerProcessor
    {
    public:
        ProcessorPlayerLevel()
        {
            BPP_INIT(ID_PLAYER_LEVEL)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if(isRequest())
                    static_cast<LocalPlayer *>(player)->updateLevel(true);
                else
                    static_cast<LocalPlayer *>(player)->setLevel();
            }
            else if (player != 0)
            {
                MWWorld::Ptr ptrPlayer =  static_cast<DedicatedPlayer *>(player)->getPtr();
                MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);

                ptrCreatureStats->setLevel(player->creatureStats.mLevel);
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERLEVEL_HPP
