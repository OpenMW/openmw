//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORGAMETIME_HPP
#define OPENMW_PROCESSORGAMETIME_HPP


#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwbase/environment.hpp>
#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorGameTime : public PlayerProcessor
    {
    public:
        ProcessorGameTime()
        {
            BPP_INIT(ID_GAME_TIME)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                MWBase::World *world = MWBase::Environment::get().getWorld();
                if (player->hour != -1)
                    world->setHour(player->hour);
                else if (player->day != -1)
                    world->setDay(player->day);
                else if (player->month != -1)
                    world->setMonth(player->month);
            }
        }
    };
}



#endif //OPENMW_PROCESSORGAMETIME_HPP
