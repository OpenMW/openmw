#ifndef OPENMW_PROCESSORGAMEWEATHER_HPP
#define OPENMW_PROCESSORGAMEWEATHER_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorGameWeather : public PlayerProcessor
    {
    public:
        ProcessorGameWeather()
        {
            BPP_INIT(ID_GAME_WEATHER)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            // Placeholder to be filled in later
        }
    };
}

#endif //OPENMW_PROCESSORGAMEWEATHER_HPP
