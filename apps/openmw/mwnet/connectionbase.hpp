#ifndef CONNECTIONBASE_H_
#define CONNECTIONBASE_H_

#include <memory>
#include <vector>

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"

#include "messageentry.hpp"

namespace MWNet
{
    constexpr const int DefaultServerPort = 40000;
    constexpr const int DefaultMaxClients = 64;
    constexpr const int MaxPacketSize = 16 * 1024;
    constexpr const int MaxSnapshotSize = 8 * 1024;
    constexpr const int MaxBlockSize = 10 * 1024;
    constexpr const uint8_t DefaultPrivateKey[yojimbo::KeyBytes] = { 0 };
    constexpr const double TickRate = 1.0 / 60.0;
    constexpr const char* LocalHost("127.0.0.1");

    class BaseAdapter : public yojimbo::Adapter
    {
    public:
        yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator)
        {
            return YOJIMBO_NEW(allocator, MWNetUnorderedMessageFactory, allocator);
        }

        virtual void OnServerClientConnected(int clientIndex) {}
        virtual void OnServerClientDisconnected(int clientIndex) {}
    };

    struct GameConnectionConfig : yojimbo::ClientServerConfig
    {
        GameConnectionConfig()
        {
            numChannels = 2;
            channel[ChannelId::EVENTSQUEUE].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
            channel[ChannelId::GAMESTATE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
        }
    };

    class Connection
    {
    protected:
        double mTime;
        GameConnectionConfig mConfig = GameConnectionConfig();
        std::unique_ptr<BaseAdapter> mAdapter;
        std::vector<std::shared_ptr<MessageEntry>> mMessageQueue;
        std::mutex mMessageQueueMutex;

        virtual void updateConnection() = 0;
        virtual void processOutgoingMessages() = 0;
        virtual void processIncomingMessages() = 0;

    public:
        virtual ~Connection() = default;
        virtual bool tick() = 0;
        virtual void clientConnected(const unsigned int clientIndex) {}
        virtual void clientDisconnected(const unsigned int clientIndex) {}

        void queueMessage(const std::shared_ptr<MessageEntry> messageEntry)
        {
            std::lock_guard<std::mutex> lock(mMessageQueueMutex);
            mMessageQueue.push_back(messageEntry);
        }
    };
}

#endif // CONNECTIONBASE_H_
