#ifndef MWNET_CLIENT_H_
#define MWNET_CLIENT_H_
#include <memory>
#include <yojimbo.h>

#include "networkmessages.hpp"

namespace MWNet
{
    class Client
    {
        std::unique_ptr<yojimbo::Client> CreateClientInstance();

        BaseAdapter mAdapter;
        std::unique_ptr<yojimbo::Client> mClient;
        double mTime;

    public:
        Client();

        int tick();

        void ClientConnected(int clientIndex);

        void ClientDisconnected(int clientIndex);

        yojimbo::Client& getClient() { return *mClient; }

        yojimbo::Adapter getAdapter() { return mAdapter; }
    };
}

#endif // MWNET_CLIENT_H_
