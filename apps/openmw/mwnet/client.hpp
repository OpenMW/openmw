#ifndef MWNET_CLIENT_H_
#define MWNET_CLIENT_H_
#include <memory>
#include <yojimbo.h>

#include "networkmessages.hpp"

namespace MWNet
{
    class Client : public MWNet::Connection
    {
        std::unique_ptr<yojimbo::Client> createClientInstance();

        std::unique_ptr<yojimbo::Client> mClient;

    public:
        Client();

        int tick() override;

        void updateConnection() override;

        void clientConnected(int clientIndex) override;

        void clientDisconnected(int clientIndex) override;

        yojimbo::Client& getClient() { return *mClient; }
    };
}

#endif // MWNET_CLIENT_H_
