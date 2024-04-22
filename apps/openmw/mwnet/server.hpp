#ifndef MWNET_SERVER_H_
#define MWNET_SERVER_H_
#include <memory>
#include <yojimbo.h>

#include "networkmessages.hpp"

namespace MWNet
{
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
