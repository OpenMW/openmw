#ifndef SERVER_H_
#define SERVER_H_
#include <memory>
#include <yojimbo.h>

namespace MWNet
{
    class Server
    {
        Server();

        std::unique_ptr<yojimbo::Server> CreateServerInstance();

        yojimbo::Adapter mAdapter;

    public:
        void ClientConnected(int clientIndex);

        void ClientDisconnected(int clientIndex);

        int run();

        yojimbo::Server getServer() { return *mServer; }

        yojimbo::Adapter getAdapter() { return mAdapter; }

        constexpr static const int DefaultPort = 40000;
        constexpr static const int DefaultMaxClients = 64;

    private:
        std::unique_ptr<yojimbo::Server> mServer;
    };
}

#endif // SERVER_H_
