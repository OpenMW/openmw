#ifndef NETWORKMANAGER_H_
#define NETWORKMANAGER_H_

#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwmechanics/creaturestats.hpp>
#include <apps/openmw/mwnet/connectionbase.hpp>
#include <apps/openmw/mwnet/networkmessages.hpp>
#include <apps/openmw/mwworld/cellstore.hpp>
#include <apps/openmw/mwworld/class.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>
#include <atomic>
#include <components/esm3/loadnpc.hpp>
#include <components/lua/serialization.hpp>
#include <memory>

#include "../mwbase/environment.hpp"
#include "client.hpp"
#include "server.hpp"

namespace MWNet
{
    class NetworkManager
    {
        bool mIsServer = false;
        std::atomic_bool mIsWriting = false;
        std::unique_ptr<Connection> mConnection;

    public:
        NetworkManager(bool isServer)
            : mIsServer(isServer)
        {
            if (isServer)
            {
                mConnection = std::make_unique<MWNet::Server>();
            }
            else
            {
                mConnection = std::make_unique<MWNet::Client>();
            }
        }

        bool queueClientMessage(int channelName, yojimbo::Message* message)
        {
            if (mIsServer)
            {
                throw std::runtime_error("Cannot queue client message on server");
            }

            if (channelName < 0 || channelName >= ChannelName::NUM_MWNET_CHANNELS)
            {
                Log(Debug::Error) << "Invalid channel name: " << channelName;
                return false;
            }

            while (mIsWriting.load(std::memory_order_relaxed))
            {
                Log(Debug::Warning) << "Network thread is running...\n";
                continue;
            }

            mIsWriting.store(true, std::memory_order_relaxed);

            Log(Debug::Warning) << "Thread is locked, attempting message send";

            mConnection->queueMessage(MessageEntry(channelName, message));

            mIsWriting.store(false, std::memory_order_relaxed);

            return true;
        }

        void queueGlobalEventMessage(const std::string& eventName, const LuaUtil::BinaryData& eventData)
        {
            while (mIsWriting.load(std::memory_order_relaxed))
            {
                continue;
            }

            mIsWriting.store(true, std::memory_order_relaxed);

            const std::shared_ptr<GlobalEventDataMessageEntry> messageEntry
                = std::make_shared<GlobalEventDataMessageEntry>(eventName, eventData);

            mConnection->queueMessage(std::move(messageEntry));

            mIsWriting.store(false, std::memory_order_relaxed);
        }

        void queueUseOrActivateMessage(
            const MWLua::GObject& object, const MWLua::GObject& actor, bool isActivate, bool force = false)
        {
            while (mIsWriting.load(std::memory_order_relaxed))
            {
                continue;
            }

            mIsWriting.store(true, std::memory_order_relaxed);

            const std::shared_ptr<UseOrActivationMessageEntry> messageEntry
                = std::make_shared<UseOrActivationMessageEntry>(object, actor, isActivate, force);

            mConnection->queueMessage(std::move(messageEntry));

            mIsWriting.store(false, std::memory_order_relaxed);
        }

        void queuePlayerLoginMessage(MWWorld::Ptr playerRef)
        {
            if (mIsServer)
            {
                throw std::runtime_error("Cannot queue player login message on client");
            }

            while (mIsWriting.load(std::memory_order_relaxed))
            {
                continue;
            }

            mIsWriting.store(true, std::memory_order_relaxed);

            yojimbo::Client* client = mConnection->getClient();

            auto& esmStore = MWBase::Environment::get().getWorld()->getStore();
            const ESM::NPC* playerRecord = esmStore.get<ESM::NPC>().find(ESM::RefId::stringRefId("Player"));

            LuaScriptIdMessage* playerLoginMessage
                = static_cast<LuaScriptIdMessage*>(client->CreateMessage(UnorderedSyncedMessage::LUA_SCRIPT_ID));

            playerLoginMessage->scriptId = playerRecord->mName;

            mConnection->queueMessage(MessageEntry(ChannelName::EVENTSQUEUE, playerLoginMessage));

            mIsWriting.store(false, std::memory_order_relaxed);
        }

        bool update()
        {
            if (!mConnection->tick())
            {
                ShutdownYojimbo();
                return false;
            }
            return true;
        }

        bool isServer() { return mIsServer; }

        Connection& getConnection() { return *mConnection; }
    };
}

#endif // NETWORKMANAGER_H_
