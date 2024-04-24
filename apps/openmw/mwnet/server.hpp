#ifndef MWNET_SERVER_H_
#define MWNET_SERVER_H_
#include <components/debug/debuglog.hpp>
#include <functional>
#include <memory>

#include <yojimbo.h>

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
                                            << verifiedMessage->player.toString();
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
