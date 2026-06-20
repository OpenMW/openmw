#ifndef NETWORKMANAGER_H_
#define NETWORKMANAGER_H_

#include <components/esm3/loadnpc.hpp>
#include <components/lua/serialization.hpp>

#include "../mwbase/environment.hpp"

#include "client.hpp"
#include "connectionbase.hpp"
#include "server.hpp"

namespace MWNet
{
    class NetworkManager
    {
    public:
        enum class Role
        {
            Host,
            Client,
            DedicatedServer,
        };

    private:
        bool mIsDedicatedServer = false;
        std::unique_ptr<Server> mServer;
        std::unique_ptr<Client> mClient;

    public:
        NetworkManager(Role role)
        {
            switch (role)
            {
                case Role::Host:
                    Log(Debug::Info) << "Host mode: starting authoritative server and local client endpoints in-process";
                    // The current transport still uses localhost yojimbo endpoints; threading and protocol separation
                    // are intentionally left for later networking work.
                    mServer = std::make_unique<MWNet::Server>();
                    mClient = std::make_unique<MWNet::Client>();
                    break;
                case Role::Client:
                    mClient = std::make_unique<MWNet::Client>();
                    break;
                case Role::DedicatedServer:
                    mIsDedicatedServer = true;
                    mServer = std::make_unique<MWNet::Server>();
                    break;
            }
        }

        bool update()
        {
            bool running = true;

            if (mServer)
                running = mServer->tick() && running;
            if (mClient)
                running = mClient->tick() && running;

            if (!running)
            {
                ShutdownYojimbo();
                return false;
            }
            return true;
        }

        bool isDedicatedServer() const { return mIsDedicatedServer; }

        void queueMessage(const std::shared_ptr<MessageEntry> messageEntry)
        {
            if (mClient)
                mClient->queueMessage(messageEntry);
            else if (mServer)
                mServer->queueMessage(messageEntry);
        }
    };
}

#endif // NETWORKMANAGER_H_
