#ifndef MWNET_CLIENT_H_
#define MWNET_CLIENT_H_
#include <memory>
#include <yojimbo.h>

#include "connectionbase.hpp"

namespace MWNet
{
    class ClientAdapter : public MWNet::BaseAdapter
    {
        MWNet::Connection& mClient;

    public:
        ClientAdapter(MWNet::Connection& client)
            : mClient(client)
        {
        }
    };

    class Client : public MWNet::Connection
    {
        std::unique_ptr<yojimbo::Client> createClientInstance();

        std::unique_ptr<yojimbo::Client> mClient;

        void updateConnection() override;

        void processMessages() override;

        void processMessage(yojimbo::Message* message);

        void processTestMessage(TestMessage* message);

        void clientConnected(int clientIndex) override {}

        void clientDisconnected(int clientIndex) override {}

    public:
        Client();

        bool tick() override;

        // std::unique_ptr<yojimbo::Client>* getClient() override { return &mClient; }
    };
}

#endif // MWNET_CLIENT_H_
