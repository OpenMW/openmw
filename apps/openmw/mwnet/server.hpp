#ifndef MWNET_SERVER_H_
#define MWNET_SERVER_H_
#include <memory>
#include <yojimbo.h>

#include "networkmessages.hpp"

namespace MWNet
{
    class ServerAdapter : public MWNet::BaseAdapter
    {
        MWNet::Connection& mServer;

        void OnServerClientConnected(int clientIndex) override { mServer.clientConnected(clientIndex); }

        void OnServerClientDisconnected(int clientIndex) override { mServer.clientDisconnected(clientIndex); }

    public:
        ServerAdapter(MWNet::Connection& server)
            : mServer(server)
        {
        }
    };

    class Server : public MWNet::Connection
    {
        std::unique_ptr<yojimbo::Server> createServerInstance();

        std::unique_ptr<yojimbo::Server> mServer;

    public:
        Server();

        int tick() override;

        void updateConnection() override;

        void clientConnected(int clientIndex) override;

        void clientDisconnected(int clientIndex) override;

        yojimbo::Server& getServer() { return *mServer; }
    };
}

#endif // MWNET_SERVER_H_
