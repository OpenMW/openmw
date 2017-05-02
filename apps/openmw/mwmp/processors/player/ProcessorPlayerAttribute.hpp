//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERATTRIBUTE_HPP
#define OPENMW_PROCESSORPLAYERATTRIBUTE_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"
#include "apps/openmw/mwmechanics/npcstats.hpp"
#include "apps/openmw/mwclass/npc.hpp"

namespace mwmp
{
    class ProcessorPlayerAttribute : public PlayerProcessor
    {
    public:
        ProcessorPlayerAttribute()
        {
            BPP_INIT(ID_PLAYER_ATTRIBUTE)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (isRequest())
                    static_cast<LocalPlayer *>(player)->updateAttributes(true);
                else
                    static_cast<LocalPlayer *>(player)->setAttributes();
            }
            else if (player != 0)
            {
                MWWorld::Ptr ptrPlayer = static_cast<DedicatedPlayer *>(player)->getPtr();
                MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
                MWMechanics::AttributeValue attributeValue;

                for (int i = 0; i < 8; ++i)
                {
                    attributeValue.readState(player->creatureStats.mAttributes[i]);
                    ptrCreatureStats->setAttribute(i, attributeValue);
                }
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERATTRIBUTE_HPP
