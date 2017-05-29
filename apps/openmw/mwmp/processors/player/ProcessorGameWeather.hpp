#ifndef OPENMW_PROCESSORGAMEWEATHER_HPP
#define OPENMW_PROCESSORGAMEWEATHER_HPP

#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorGameWeather : public PlayerProcessor
    {
    public:
        ProcessorGameWeather()
        {
            BPP_INIT(ID_GAME_WEATHER)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            // Placeholder to be filled in later
        }
    };
}

#endif //OPENMW_PROCESSORGAMEWEATHER_HPP
