#include "luaeventrouter.hpp"

#include <components/debug/debuglog.hpp>

#include <memory>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"

#include "messageentry.hpp"
#include "networkmanager.hpp"

void MWNet::LuaEventRouter::receiveNetworkedGlobalEvent(
    unsigned int clientIndex, const std::string& eventName, const std::string& eventData)
{
    const auto& environment = MWBase::Environment::get();
    if (!environment.hasAuthoritativeLuaManager())
    {
        Log(Debug::Error) << "LuaEventRouter: dropping networked global event '" << eventName << "' from client "
                          << clientIndex
                          << " because authoritative Lua manager is not registered yet (transitional Host state)";
        return;
    }

    // Future replication/fanout belongs at this boundary, not in Lua manager-to-manager calls.
    environment.getAuthoritativeLuaManager()->queueNetworkedGlobalEvent(eventName, eventData);
}

void MWNet::LuaEventRouter::sendGlobalEventToAuthoritativeRuntime(std::string eventName, std::string eventData)
{
    const auto& environment = MWBase::Environment::get();
    if (environment.hasAuthoritativeLuaManager())
        environment.getAuthoritativeLuaManager()->queueNetworkedGlobalEvent(eventName, eventData);
    else
    {
        const auto globalEventMessage
            = std::make_shared<MWNet::GlobalEventDataMessageEntry>(std::move(eventName), std::move(eventData));
        environment.getNetworkManager()->queueMessage(globalEventMessage);
    }
}
