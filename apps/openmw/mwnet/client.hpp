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
        uint64_t mClientId = 0;

        yojimbo::Address mAddress;
        yojimbo::Address mDestination;

        std::unique_ptr<yojimbo::Client> mClient;

        std::unique_ptr<yojimbo::Client> createClientInstance();

        void updateConnection() override;

        void processMessages() override;

        void processMessage(yojimbo::Message* message);

        void clientConnected(int clientIndex) override {}

        void clientDisconnected(int clientIndex) override {}

    public:
        yojimbo::Client* getClient() override { return mClient.get(); }

        void queueMessage(MessageEntry message) override { mMessageQueue.push_back(std::move(message)); }

        Client();

        bool tick() override;

        // std::unique_ptr<yojimbo::Client>* getClient() override { return &mClient; }
    };
}

#endif // MWNET_CLIENT_H_
