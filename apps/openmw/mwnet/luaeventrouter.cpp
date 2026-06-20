#include "luaeventrouter.hpp"

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"

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
