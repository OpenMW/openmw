#ifndef NETWORKMANAGER_H_
#define NETWORKMANAGER_H_

#include <components/esm3/loadnpc.hpp>
#include <components/lua/serialization.hpp>

#include "../mwbase/environment.hpp"
#include "../mwnet/connectionbase.hpp"

#include "client.hpp"
#include "server.hpp"

namespace MWNet
{
    class NetworkManager
    {
        bool mIsServer = false;
        std::unique_ptr<Connection> mConnection;

    public:
        NetworkManager(bool isServer)
            : mIsServer(isServer)
        {
            if (isServer)
            {
                mConnection = std::make_unique<MWNet::Server>();
            }
            else
            {
                mConnection = std::make_unique<MWNet::Client>();
            }
        }

        void queueGlobalEventMessage(const std::string& eventName, const LuaUtil::BinaryData& eventData)
        {
            const std::shared_ptr<GlobalEventDataMessageEntry> messageEntry
                = std::make_shared<GlobalEventDataMessageEntry>(eventName, eventData);

            mConnection->queueMessage(std::move(messageEntry));
        }

        void queueUseOrActivateMessage(
            const MWLua::GObject& object, const MWLua::GObject& actor, bool isActivate, bool force = false)
        {
            const std::shared_ptr<UseOrActivationMessageEntry> messageEntry
                = std::make_shared<UseOrActivationMessageEntry>(object, actor, isActivate, force);

            mConnection->queueMessage(std::move(messageEntry));
        }

        void queuePlayerLoginMessage(MWWorld::Ptr playerRef) {}

        bool update()
        {
            if (!mConnection->tick())
            {
                ShutdownYojimbo();
                return false;
            }
            return true;
        }

        bool isServer() { return mIsServer; }

        Connection& getConnection() { return *mConnection; }
    };
}

#endif // NETWORKMANAGER_H_
