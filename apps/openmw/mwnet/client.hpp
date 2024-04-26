#ifndef MWNET_CLIENT_H_
#define MWNET_CLIENT_H_

#include "connectionbase.hpp"
#include "networkmessages.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"

namespace MWNet
{
    class ClientAdapter : public MWNet::BaseAdapter
    {

    public:
        ClientAdapter(MWNet::Connection& client) {}
    };

    class Client : public MWNet::Connection
    {
        uint64_t mClientId = 0;

        yojimbo::Address mAddress;
        yojimbo::Address mDestination;

        std::unique_ptr<yojimbo::Client> mClient;

        template <typename T>
        T verifyMessage(yojimbo::Message* message)
        {
            T newMessage = dynamic_cast<T>(message);

            if (!newMessage)
            {
                mClient->Disconnect();
                throw std::runtime_error(
                    "CLIENT: Disconnecting due to receiving invalid messageType " + std::to_string(message->GetType()));
            }

            return newMessage;
        }

        const std::array<std::function<void(yojimbo::Message*)>, MessageId::NUM_MWNET_MESSAGES> mIncomingMessageHandlers
            = {
                  [this](yojimbo::Message* message) {
                      const auto* verifiedMessage = verifyMessage<PlayerLoginMessage*>(message);

                      Log(Debug::Warning) << "CLIENT: received login message: " << verifiedMessage->player;
                  },
                  [this](yojimbo::Message* message) {
                      const auto* verifiedMessage = verifyMessage<LuaScriptIdMessage*>(message);

                      Log(Debug::Warning) << "CLIENT: received scriptId message: " << verifiedMessage->scriptId;
                  },
                  [this](yojimbo::Message* message) {
                      const auto* activationMessage = verifyMessage<UseOrActivateRequestMessage*>(message);

                      Log(Debug::Warning)
                          << "CLIENT: received message: " << (activationMessage->isActivation ? "activation" : "use")
                          << " for object " << activationMessage->object.ptr().toString() << " by actor "
                          << activationMessage->actor.ptr().toString();

                      if (!activationMessage->object.ptr().getRefData().activate())
                          return;

                      MWBase::LuaManager* luaMan = MWBase::Environment::get().getLuaManager();
                      if (activationMessage->isActivation)
                      {
                          luaMan->addActivationAction(activationMessage->object.ptr(), activationMessage->actor.ptr());
                      }
                      else
                      {
                          luaMan->addUseAction(activationMessage->object.ptr(), activationMessage->actor.ptr(),
                              activationMessage->force);
                      }
                  },
                  [this](yojimbo::Message* message) {
                      throw std::runtime_error("CLIENT: received global event message! Disconnecting!");
                      mClient->Disconnect();
                  },
              };

        const std::array<std::function<void(MessageEntry* messageEntry)>, MessageId::NUM_MWNET_MESSAGES>
            mOutgoingMessageHandlers = {
                [](MessageEntry* messageEntry) {},
                [](MessageEntry* messageEntry) {},
                [this](MessageEntry* messageEntry) {
                    const auto* activateEntry = downcastMessageEntry<UseOrActivationMessageEntry*>(messageEntry);
                    auto* message = verifyMessage<UseOrActivateRequestMessage*>(
                        mClient->CreateMessage(activateEntry->messageType));

                    message->object = activateEntry->object;
                    message->actor = activateEntry->actor;
                    message->isActivation = activateEntry->isActivation;

                    if (!message->isActivation)
                    {
                        message->force = activateEntry->force;
                    }

                    mClient->SendMessage(messageEntry->channelName, message);
                },
                [this](MessageEntry* messageEntry) {
                    const auto* globalEntry = downcastMessageEntry<GlobalEventDataMessageEntry*>(messageEntry);
                    auto* message
                        = verifyMessage<GlobalEventQueuedMessage*>(mClient->CreateMessage(globalEntry->messageType));

                    message->eventName = globalEntry->eventName;
                    message->eventData = globalEntry->eventData;
                    mClient->SendMessage(messageEntry->channelName, message);
                },
            };

        void updateConnection() override;

        void processIncomingMessages() override;

        void processOutgoingMessages() override;

        bool processIncomingMessage(yojimbo::Message* message);

    public:
        Client();

        bool tick() override;
    };
}

#endif // MWNET_CLIENT_H_
