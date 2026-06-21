#ifndef GAME_MWBASE_LUAEVENTROUTER_H
#define GAME_MWBASE_LUAEVENTROUTER_H

#include <string>

namespace MWBase
{
    class LuaEventRouter
    {
    public:
        virtual ~LuaEventRouter() = default;

        virtual void receiveNetworkedGlobalEvent(
            unsigned int clientIndex, const std::string& eventName, const std::string& eventData)
            = 0;

        virtual void sendGlobalEventToAuthoritativeRuntime(std::string eventName, std::string eventData) = 0;
    };
}

#endif // GAME_MWBASE_LUAEVENTROUTER_H
