#ifndef MWNET_SERVER_H_
#define MWNET_SERVER_H_
#include <memory>
#include <yojimbo.h>

#include "networkmessages.hpp"

namespace MWNet
{
    class Server
    {
        std::unique_ptr<yojimbo::Server> CreateServerInstance();

        BaseAdapter mAdapter;
        std::unique_ptr<yojimbo::Server> mServer;
        double mTime;

    public:
        Server();

        int tick();

        void ClientConnected(int clientIndex);

        void ClientDisconnected(int clientIndex);

        yojimbo::Server getServer() { return *mServer; }

        yojimbo::Adapter getAdapter() { return mAdapter; }

        constexpr static const int DefaultPort = 40000;
        constexpr static const int DefaultMaxClients = 64;
    };
}

#endif // MWNET_SERVER_H_
