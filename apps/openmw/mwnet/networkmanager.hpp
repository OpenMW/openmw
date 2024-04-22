#ifndef NETWORKMANAGER_H_
#define NETWORKMANAGER_H_

#include <memory>

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
