#ifndef NETWORKMESSAGES_H
#define NETWORKMESSAGES_H

#include <string>

#include <yojimbo.h>

#include "../mwlua/object.hpp"
#include "esmserialize.hpp"

namespace MWNet
{
    constexpr uint64_t PROTOCOL_ID = 0xA455ULL;

    enum ChannelId
    {
        EVENTSQUEUE,
        GAMESTATE,
        NUM_MWNET_CHANNELS,
    };

    enum MessageId
    {
        PLAYER_LOGIN_MESSAGE,
        LUA_SCRIPT_ID,
        USE_OR_ACTIVATE_REQUEST,
        GLOBAL_EVENT_QUEUED,
        NUM_MWNET_MESSAGES,
    };

    class PlayerLoginMessage : public yojimbo::Message
    {
    public:
        std::string player;

        PlayerLoginMessage() {}

        template <typename Stream>
        bool Serialize(Stream& stream)
        {
            serialize_std_string(stream, player, MAX_STRING_LENGTH);
            return true;
        }

        YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
    };

    class LuaScriptIdMessage : public yojimbo::Message
    {
    public:
        std::string scriptId;

        LuaScriptIdMessage() {}

        template <typename Stream>
        bool Serialize(Stream& stream)
        {
            serialize_std_string(stream, scriptId, MAX_STRING_LENGTH);
            return true;
        }
        YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
    };

    class UseOrActivateRequestMessage : public yojimbo::Message
    {
    public:
        MWLua::GObject object;
        MWLua::GObject actor;
        bool isActivation;
        bool force;

        UseOrActivateRequestMessage() {}

        template <typename Stream>
        bool Serialize(Stream& stream)
        {
            serialize_bytes(stream, (uint8_t*)&object, sizeof(object));
            serialize_bytes(stream, (uint8_t*)&actor, sizeof(actor));
            serialize_bool(stream, isActivation);

            if (!isActivation)
            {
                serialize_bool(stream, force);
            }

            return true;
        }
        YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
    };

    class GlobalEventQueuedMessage : public yojimbo::Message
    {
    public:
        std::string eventName;
        LuaUtil::BinaryData eventData;

        GlobalEventQueuedMessage() {}

        template <typename Stream>
        bool Serialize(Stream& stream)
        {
            serialize_std_string(stream, eventName, MAX_STRING_LENGTH);
            serialize_lua_data(stream, eventData);
            return true;
        }

        YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
    };

    YOJIMBO_MESSAGE_FACTORY_START(MWNetUnorderedMessageFactory, NUM_MWNET_MESSAGES);
    YOJIMBO_DECLARE_MESSAGE_TYPE(PLAYER_LOGIN_MESSAGE, PlayerLoginMessage);
    YOJIMBO_DECLARE_MESSAGE_TYPE(LUA_SCRIPT_ID, LuaScriptIdMessage);
    YOJIMBO_DECLARE_MESSAGE_TYPE(USE_OR_ACTIVATE_REQUEST, UseOrActivateRequestMessage);
    YOJIMBO_DECLARE_MESSAGE_TYPE(GLOBAL_EVENT_QUEUED, GlobalEventQueuedMessage);
    YOJIMBO_MESSAGE_FACTORY_FINISH()
}

#endif // #ifndef NETWORKMESSAGES_H
