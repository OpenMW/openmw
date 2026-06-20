#ifndef MESSAGEENTRY_H_
#define MESSAGEENTRY_H_

#include "networkmessages.hpp"

namespace MWNet
{

    struct MessageEntry
    {
        const ChannelId channelName;
        const MessageId messageType;

        virtual ~MessageEntry() = default;
        MessageEntry(ChannelId channel, MessageId type)
            : channelName(channel)
            , messageType(type)
        {
        }
    };

    struct GlobalEventDataMessageEntry : public MessageEntry
    {
        const std::string eventName;
        const LuaUtil::BinaryData eventData;

        GlobalEventDataMessageEntry(const std::string& name, const LuaUtil::BinaryData& data)
            : MessageEntry(ChannelId::EVENTSQUEUE, MessageId::GLOBAL_EVENT_QUEUED)
            , eventName(name)
            , eventData(data)
        {
        }
    };

    struct UseOrActivationMessageEntry : public MessageEntry
    {
        const bool isActivation;
        const bool force;
        const MWLua::GObject object;
        const MWLua::GObject actor;

        UseOrActivationMessageEntry(
            const MWWorld::Ptr& objectPtr, const MWWorld::Ptr& actorPtr, const bool activation, const bool forced = false)
            : MessageEntry(ChannelId::EVENTSQUEUE, MessageId::USE_OR_ACTIVATE_REQUEST)
            , isActivation(activation)
            , force(forced)
            , object(MWLua::GObject(objectPtr))
            , actor(MWLua::GObject(actorPtr))
        {
        }
    };

    struct ServerMessageEntry : public MessageEntry
    {
        const unsigned int clientId;

        ServerMessageEntry(ChannelId channel, MessageId type, uint client = -1)
            : MessageEntry(channel, type)
            , clientId(client)
        {
        }
    };

    template <typename T>
    T downcastMessageEntry(MessageEntry* messageEntry)
    {
        T newMessageEntry = dynamic_cast<T>(messageEntry);

        if (!newMessageEntry)
        {
            throw std::runtime_error("Failed to downcast queued message entry!");
        }

        return newMessageEntry;
    }
}

#endif // MESSAGEENTRY_H_
