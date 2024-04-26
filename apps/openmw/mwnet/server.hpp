#ifndef MWNET_SERVER_H_
#define MWNET_SERVER_H_
#include <functional>
#include <memory>
#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"

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
        std::unique_ptr<yojimbo::Server> mServer;

        template <typename T>
        T verifyMessage(yojimbo::Message* message, const unsigned int clientIndex)
        {
            T newMessage = dynamic_cast<T>(message);

            if (!newMessage)
            {
                mServer->DisconnectClient(clientIndex);
                throw std::runtime_error(
                    "SERVER: Disconnected client " + std::to_string(clientIndex) + " for sending an invalid message.");
            }

            return newMessage;
        }

        const std::array<std::function<void(const unsigned int clientIndex, yojimbo::Message*)>,
            MessageId::NUM_MWNET_MESSAGES>
            mMessageHandlers = {
                [this](const unsigned int clientIndex, yojimbo::Message* message) {
                    const auto* verifiedMessage = verifyMessage<PlayerLoginMessage*>(message, clientIndex);

                    Log(Debug::Warning) << "SERVER: received login message from client " << clientIndex << ": "
                                        << verifiedMessage->player;
                },
                [this](const unsigned int clientIndex, yojimbo::Message* message) {
                    const auto* verifiedMessage = verifyMessage<LuaScriptIdMessage*>(message, clientIndex);

                    Log(Debug::Warning) << "SERVER: received scriptId message from client " << clientIndex
                                        << ": for script: " << verifiedMessage->scriptId;
                },
                [this](const unsigned int clientIndex, yojimbo::Message* message) {
                    const auto* verifiedMessage = verifyMessage<UseOrActivateRequestMessage*>(message, clientIndex);

                    Log(Debug::Warning) << "SERVER: received " << (verifiedMessage->isActivation ? "activate" : "use")
                                        << " request from client " << clientIndex << ": "
                                        << verifiedMessage->actor.toString()
                                        << " wants to activate: " << verifiedMessage->object.toString()
                                        << "\nBlanket granting permission because our shit is broke.";

                    // HACK: This makes the behavior of activation work for players or the calling actor. Which is
                    // great, kind of. The problem is that we've removed the handling of activation from global
                    // scripts in doing this.
                    // However there is presently no means by which we can actually allow global scripts to interact
                    // with the player, or any object for that matter.
                    // This is just a stub to make things more usable for now.
                    auto* activationResponseMessage = dynamic_cast<UseOrActivateRequestMessage*>(
                        mServer->CreateMessage(clientIndex, MessageId::USE_OR_ACTIVATE_REQUEST));

                    if (!activationResponseMessage)
                    {
                        throw std::runtime_error(
                            "SERVER: Activation/Use response failed, unable to allocate response message");
                    }

                    activationResponseMessage->actor = verifiedMessage->actor;
                    activationResponseMessage->object = verifiedMessage->object;
                    activationResponseMessage->isActivation = verifiedMessage->isActivation;

                    if (!verifiedMessage->isActivation)
                    {
                        activationResponseMessage->force = verifiedMessage->force;
                    }

                    mServer->SendMessage(clientIndex, ChannelId::EVENTSQUEUE, activationResponseMessage);
                },
                [this](const unsigned int clientIndex, yojimbo::Message* message) {
                    const auto* verifiedMessage = verifyMessage<GlobalEventQueuedMessage*>(message, clientIndex);

                    Log(Debug::Warning) << "SERVER: received Global event queued from client " << clientIndex << ": "
                                        << verifiedMessage->eventName << " with data: " << verifiedMessage->eventData
                                        << ", of size: " << verifiedMessage->eventData.size();

                    const auto luaMgr = MWBase::Environment::get().getLuaManager();
                    luaMgr->queueGlobalEventMessage(verifiedMessage->eventName, verifiedMessage->eventData);
                },
            };

        std::unique_ptr<yojimbo::Server> createServerInstance();

        void updateConnection() override;

        void processMessages() override;

        void clientConnected(const unsigned int clientIndex) override;

        void clientDisconnected(const unsigned int clientIndex) override;

        bool processMessage(const unsigned int clientIndex, const unsigned int channelIndex, yojimbo::Message* message);

    public:
        Server();

        bool tick() override;
    };
}

#endif // MWNET_SERVER_H_
