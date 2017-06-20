#ifndef OPENMW_PROCESSORGAMESETTINGS_HPP
#define OPENMW_PROCESSORGAMESETTINGS_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorGameSettings : public PlayerProcessor
    {
    public:
        ProcessorGameSettings()
        {
            BPP_INIT(ID_GAME_SETTINGS)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {

        }
    };
}

#endif //OPENMW_PROCESSORGAMESETTINGS_HPP
