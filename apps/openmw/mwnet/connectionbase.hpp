#ifndef CONNECTIONBASE_H_
#define CONNECTIONBASE_H_
#include <memory>

#include "networkmessages.hpp"

namespace MWNet
{
    enum ChannelName
    {
        EVENTSQUEUE = 0,
        GAMESTATE = 1
    };

    constexpr const int DefaultClientPort = 30000;
    constexpr const int DefaultServerPort = 40000;
    constexpr const int DefaultMaxClients = 64;
    constexpr const int MaxPacketSize = 16 * 1024;
    constexpr const int MaxSnapshotSize = 8 * 1024;
    constexpr const int MaxBlockSize = 10 * 1024;
    constexpr const short MAX_RETRIES = 128;
    constexpr const uint8_t DefaultPrivateKey[yojimbo::KeyBytes] = { 0 };
    constexpr const double TickRate = 1.0 / 60.0;
    constexpr const char* LocalHost("127.0.0.1");

    class BaseAdapter : public Adapter
    {
    public:
        MessageFactory* CreateMessageFactory(Allocator& allocator)
        {
            return YOJIMBO_NEW(allocator, TestMessageFactory, allocator);
        }

        virtual void OnServerClientConnected(int clientIndex) {}
        virtual void OnServerClientDisconnected(int clientIndex) {}
    };

    struct GameConnectionConfig : yojimbo::ClientServerConfig
    {
        GameConnectionConfig()
        {
            numChannels = 2;
            channel[ChannelName::EVENTSQUEUE].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
            channel[ChannelName::GAMESTATE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
        }
    };

    class Connection
    {
    public:
        short mRetries = 0;
        double mTime;
        std::unique_ptr<BaseAdapter> mAdapter;
        GameConnectionConfig mConfig = GameConnectionConfig();
        virtual ~Connection() = default;
        virtual bool tick() = 0;
        virtual void updateConnection() = 0;
        virtual void processMessages() = 0;
        virtual void clientConnected(int clientIndex) = 0;
        virtual void clientDisconnected(int clientIndex) = 0;
        BaseAdapter getAdapter() { return *mAdapter; }
    };
}

#endif // CONNECTIONBASE_H_
