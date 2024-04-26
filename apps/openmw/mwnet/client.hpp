#ifndef MWNET_CLIENT_H_
#define MWNET_CLIENT_H_
#include <memory>

#include "connectionbase.hpp"
#include "networkmessages.hpp"

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

    public:
        Client();

        void queueMessage(const std::shared_ptr<MessageEntry> message) override { mMessageQueue.push_back(message); }

        bool tick() override;

        yojimbo::Client* getClient() override { return mClient.get(); }
    };
}

#endif // MWNET_CLIENT_H_
