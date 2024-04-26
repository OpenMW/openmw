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
        MessageEntry(ChannelId channelName, MessageId messageType)
            : channelName(channelName)
            , messageType(messageType)
        {
        }
    };

    struct GlobalEventDataMessageEntry : public MessageEntry
    {
        const std::string eventName;
        const LuaUtil::BinaryData eventData;

        GlobalEventDataMessageEntry(const std::string& eventName, const LuaUtil::BinaryData& eventData)
            : MessageEntry(ChannelId::EVENTSQUEUE, MessageId::GLOBAL_EVENT_QUEUED)
            , eventName(eventName)
            , eventData(eventData)
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
            const MWWorld::Ptr& object, const MWWorld::Ptr& actor, const bool isActivation, const bool force = false)
            : MessageEntry(ChannelId::EVENTSQUEUE, MessageId::USE_OR_ACTIVATE_REQUEST)
            , isActivation(isActivation)
            , force(force)
            , object(MWLua::GObject(object))
            , actor(MWLua::GObject(actor))
        {
        }
    };

    struct ServerMessageEntry : public MessageEntry
    {
        const unsigned int clientId;

        ServerMessageEntry(ChannelId channelName, MessageId messageType, uint client = -1)
            : MessageEntry(channelName, messageType)
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
