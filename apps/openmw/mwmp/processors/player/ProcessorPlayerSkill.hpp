//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERSKILL_HPP
#define OPENMW_PROCESSORPLAYERSKILL_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerSkill : public PlayerProcessor
    {
    public:
        ProcessorPlayerSkill()
        {
            BPP_INIT(ID_PLAYER_SKILL)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (isRequest())
                    static_cast<LocalPlayer *>(player)->updateSkills(true);
                else
                    static_cast<LocalPlayer *>(player)->setSkills();
            }
            else if (player != 0)
            {
                MWWorld::Ptr ptrPlayer = static_cast<DedicatedPlayer *>(player)->getPtr();
                MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
                MWMechanics::SkillValue skillValue;

                for (int i = 0; i < 27; ++i)
                {
                    skillValue.readState(player->npcStats.mSkills[i]);
                    ptrNpcStats->setSkill(i, skillValue);
                }
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERSKILL_HPP
