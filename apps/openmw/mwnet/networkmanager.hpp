#ifndef NETWORKMANAGER_H_
#define NETWORKMANAGER_H_

#include <components/esm3/loadnpc.hpp>
#include <components/lua/serialization.hpp>

#include <yojimbo.h>

#include <stdexcept>

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
        class YojimboRuntime
        {
        public:
            YojimboRuntime()
            {
                if (!InitializeYojimbo())
                    throw std::logic_error("error: failed to initialize Yojimbo!\n");
            }

            ~YojimboRuntime() { ShutdownYojimbo(); }
        };

        Role mRole;
        bool mIsDedicatedServer = false;
        YojimboRuntime mYojimboRuntime;
        std::unique_ptr<Server> mServer;
        std::unique_ptr<Client> mClient;

        static const char* roleName(Role role)
        {
            switch (role)
            {
                case Role::Host:
                    return "Host";
                case Role::Client:
                    return "Client";
                case Role::DedicatedServer:
                    return "DedicatedServer";
            }

            throw std::logic_error("Unhandled network role");
        }

    public:
        NetworkManager(Role role)
            : mRole(role)
        {
            Log(Debug::Info) << "NetworkManager role: " << roleName(mRole);
            switch (role)
            {
                case Role::Host:
                    Log(Debug::Info)
                        << "NetworkManager Host: creating authoritative server and local client endpoints in-process";
                    // The current transport still uses localhost yojimbo endpoints; threading and protocol separation
                    // are intentionally left for later networking work.
                    mServer = std::make_unique<MWNet::Server>();
                    mClient = std::make_unique<MWNet::Client>();
                    break;
                case Role::Client:
                    Log(Debug::Info) << "NetworkManager Client: creating client endpoint";
                    mClient = std::make_unique<MWNet::Client>();
                    break;
                case Role::DedicatedServer:
                    mIsDedicatedServer = true;
                    Log(Debug::Info) << "NetworkManager DedicatedServer: creating server endpoint only";
                    mServer = std::make_unique<MWNet::Server>();
                    break;
            }
        }

        ~NetworkManager()
        {
            Log(Debug::Info) << "NetworkManager " << roleName(mRole) << ": shutting down endpoints";
            mClient = nullptr;
            mServer = nullptr;
            Log(Debug::Info) << "NetworkManager " << roleName(mRole) << ": endpoints shut down";
        }

        bool update(double tickStep = MWNet::TickRate)
        {
            bool running = true;

            if (mServer)
                running = mServer->tick(tickStep) && running;
            if (mClient)
                running = mClient->tick(tickStep) && running;

            if (!running)
            {
                Log(Debug::Warning) << "NetworkManager " << roleName(mRole)
                                    << ": endpoint tick failed; transport will shut down with NetworkManager";
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
