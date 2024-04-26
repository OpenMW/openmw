#ifndef CONNECTIONBASE_H_
#define CONNECTIONBASE_H_
#include <memory>
#include <time.h>
#include <vector>

#include <components/lua/serialization.hpp>

#include "networkmessages.hpp"

namespace MWNet
{
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

    struct MessageEntry
    {
        ChannelName channelName;
        UnorderedSyncedMessage messageType;

        MessageEntry(ChannelName channelName, UnorderedSyncedMessage messageType)
            : channelName(channelName)
            , messageType(messageType)
        {
        }
        virtual ~MessageEntry() = default;
    };

    struct GlobalEventDataMessageEntry : public MessageEntry
    {
        std::string eventName;
        LuaUtil::BinaryData eventData;

        GlobalEventDataMessageEntry(const std::string& eventName, const LuaUtil::BinaryData& eventData)
            : MessageEntry(ChannelName::EVENTSQUEUE, UnorderedSyncedMessage::GLOBAL_EVENT_QUEUED)
            , eventName(eventName)
            , eventData(eventData)
        {
        }
    };

    struct UseOrActivationMessageEntry : public MessageEntry
    {
        bool isActivation;
        bool force;
        MWLua::GObject object;
        MWLua::GObject actor;

        UseOrActivationMessageEntry(
            const MWLua::GObject& object, const MWLua::GObject& actor, const bool isActivation, const bool force)
            : MessageEntry(ChannelName::EVENTSQUEUE, UnorderedSyncedMessage::USE_OR_ACTIVATE_REQUEST)
            , isActivation(isActivation)
            , force(force)
            , object(object)
            , actor(actor)
        {
        }
    };

    struct ServerMessageEntry : public MessageEntry
    {
        uint clientId;

        ServerMessageEntry(uint client, ChannelName channelName, UnorderedSyncedMessage messageType)
            : MessageEntry(channelName, messageType)
            , clientId(client)
        {
        }
    };

    class BaseAdapter : public Adapter
    {
    public:
        MessageFactory* CreateMessageFactory(Allocator& allocator)
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
        std::vector<std::shared_ptr<MessageEntry>> mMessageQueue;
        virtual ~Connection() = default;
        virtual bool tick() = 0;
        virtual void updateConnection() = 0;
        virtual void processMessages() = 0;
        virtual void clientConnected(const unsigned int clientIndex) {}
        virtual void clientDisconnected(const unsigned int clientIndex) {}
        virtual void queueMessage(const std::shared_ptr<MessageEntry> message) {}
        virtual yojimbo::Client* getClient() { return nullptr; }
        BaseAdapter getAdapter() { return *mAdapter; }
    };
}

#endif // CONNECTIONBASE_H_
