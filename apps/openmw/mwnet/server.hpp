#ifndef MWNET_SERVER_H_
#define MWNET_SERVER_H_
#include <functional>
#include <memory>

#include <yojimbo.h>

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwlua/luamanagerimp.hpp"

#include "connectionbase.hpp"
#include "networkmessages.hpp"

namespace MWNet
{
    class ServerAdapter : public MWNet::BaseAdapter
    {
        MWNet::Connection& mServer;

        void OnServerClientConnected(int clientIndex) override { mServer.clientConnected(clientIndex); }

        void OnServerClientDisconnected(int clientIndex) override { mServer.clientDisconnected(clientIndex); }

    public:
        ServerAdapter(MWNet::Connection& server)
            : mServer(server)
        {
        }
    };

    class Server : public MWNet::Connection
    {
        template <typename T>
        T verifyMessage(yojimbo::Message* message, int clientIndex)
        {
            T newMessage = dynamic_cast<T>(message);

            if (!newMessage)
            {
                Log(Debug::Error) << "SERVER: received invalid message from client " << clientIndex
                                  << ", disconnecting\n";
                mServer->DisconnectClient(clientIndex);
            }

            return newMessage;
        }

        const std::array<std::function<void(int clientIndex, yojimbo::Message*)>,
            UnorderedSyncedMessage::NUM_UNORDERED_SYNC_MESSAGES>
            mMessageHandlers = {
                [this](int clientIndex, yojimbo::Message* message) {
                    PlayerLoginMessage* verifiedMessage = verifyMessage<PlayerLoginMessage*>(message, clientIndex);
                    if (verifiedMessage)
                    {
                        Log(Debug::Warning) << "SERVER: received login message from client " << clientIndex << ": "
                                            << verifiedMessage->player;
                    }
                },
                [this](int clientIndex, yojimbo::Message* message) {
                    LuaScriptIdMessage* verifiedMessage = verifyMessage<LuaScriptIdMessage*>(message, clientIndex);
                    if (verifiedMessage)
                    {
                        Log(Debug::Warning) << "SERVER: received scriptId message from client " << clientIndex
                                            << ": for script: " << verifiedMessage->scriptId;
                    }
                },
                [this](int clientIndex, yojimbo::Message* message) {
                    UseOrActivateRequestMessage* verifiedMessage
                        = verifyMessage<UseOrActivateRequestMessage*>(message, clientIndex);
                    if (verifiedMessage)
                    {
                        Log(Debug::Warning) << "SERVER: received use or activation request from client " << clientIndex
                                            << ": " << verifiedMessage->actor.toString()
                                            << " wants to activate: " << verifiedMessage->object.toString()
                                            << "\nBlanket granting permission because our shit is broke.";
                        // HACK: This makes the behavior of activation work for players or the calling actor. Which is
                        // great, kind of. The problem is that we've removed the handling of activation from global
                        // scripts in doing this.
                        // However there is presently no means by which we can actually allow global scripts to interact
                        // with the player. This is just a stub to make things more usable for now.
                        UseOrActivateRequestMessage* activationResponseMessage
                            = static_cast<UseOrActivateRequestMessage*>(
                                mServer->CreateMessage(clientIndex, UnorderedSyncedMessage::USE_OR_ACTIVATE_REQUEST));
                        activationResponseMessage->actor = verifiedMessage->actor;
                        activationResponseMessage->object = verifiedMessage->object;
                        activationResponseMessage->isActivation = verifiedMessage->isActivation;

                        if (!verifiedMessage->isActivation)
                        {
                            activationResponseMessage->force = verifiedMessage->force;
                        }

                        mServer->SendMessage(clientIndex, ChannelName::EVENTSQUEUE, activationResponseMessage);
                    }
                },
                [this](int clientIndex, yojimbo::Message* message) {
                    GlobalEventQueuedMessage* verifiedMessage
                        = verifyMessage<GlobalEventQueuedMessage*>(message, clientIndex);
                    if (verifiedMessage)
                    {
                        Log(Debug::Warning)
                            << "SERVER: received Global event queued from client " << clientIndex << ": "
                            << verifiedMessage->eventName << " with data: " << verifiedMessage->eventData
                            << ", of size: " << verifiedMessage->eventData.size();
                        MWBase::LuaManager* luaMan = MWBase::Environment::get().getLuaManager();
                        luaMan->queueGlobalEventMessage(verifiedMessage->eventName, verifiedMessage->eventData);
                    }
                },
            };

        std::unique_ptr<yojimbo::Server> createServerInstance();

        std::unique_ptr<yojimbo::Server> mServer;

        void updateConnection() override;

        void processMessages() override;

        void clientConnected(int clientIndex) override;

        void clientDisconnected(int clientIndex) override;

        bool processMessage(int clientIndex, int channelIndex, yojimbo::Message* message);

    public:
        Server();

        bool tick() override;
    };
}

#endif // MWNET_SERVER_H_
