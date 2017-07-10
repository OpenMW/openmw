#ifndef OPENMW_PROCESSORCONSOLECOMMAND_HPP
#define OPENMW_PROCESSORCONSOLECOMMAND_HPP

#include "../WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorConsoleCommand : public WorldProcessor
    {
    public:
        ProcessorConsoleCommand()
        {
            BPP_INIT(ID_CONSOLE_COMMAND)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received %s", strPacketID.c_str());
            //event.runConsoleCommand();
        }
    };
}

#endif //OPENMW_PROCESSORCONSOLECOMMAND_HPP
