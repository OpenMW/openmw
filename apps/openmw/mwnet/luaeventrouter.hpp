#ifndef MWNET_LUAEVENTROUTER_H_
#define MWNET_LUAEVENTROUTER_H_

#include "../mwbase/luaeventrouter.hpp"

#include <string>

namespace MWNet
{
    class LuaEventRouter : public MWBase::LuaEventRouter
    {
    public:
        void receiveNetworkedGlobalEvent(
            unsigned int clientIndex, const std::string& eventName, const std::string& eventData) override;

        void sendGlobalEventToAuthoritativeRuntime(std::string eventName, std::string eventData) override;
    };
}

#endif // MWNET_LUAEVENTROUTER_H_
