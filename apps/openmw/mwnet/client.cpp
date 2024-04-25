#include <apps/openmw/mwnet/connectionbase.hpp>
#include <apps/openmw/mwnet/networkmessages.hpp>
#include <csignal>
#include <stdexcept>
#include <time.h>

#include <yojimbo.h>

#include "client.hpp"
#include <components/debug/debuglog.hpp>

static volatile int quit = 0;

// void clientInterruptHandler(int)
// {
//     quit = 1;
// }

// Upgrade later to accept a destination & source address
MWNet::Client::Client()
    : mAddress(MWNet::LocalHost, MWNet::DefaultClientPort)
    , mDestination(MWNet::LocalHost, MWNet::DefaultServerPort)
{
    if (!InitializeYojimbo())
    {
        throw std::logic_error("error: failed to initialize Yojimbo!\n");
    }

    mAdapter = std::make_unique<MWNet::ClientAdapter>(*this);

    mClient = createClientInstance();

    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);

    srand((unsigned int)time(NULL));

    // signal(SIGINT, clientInterruptHandler);

    Address serverAddress(MWNet::LocalHost, MWNet::DefaultServerPort);

    uint64_t clientId = 0;
    yojimbo_random_bytes((uint8_t*)&clientId, 8);

    mClient->InsecureConnect(MWNet::DefaultPrivateKey, clientId, serverAddress);

    if (!mClient->IsConnected())
    {
        Log(Debug::Warning) << "Client was not connected on network instantiation.";
        return;
    }

    char addressString[256];
    mClient->GetAddress().ToString(addressString, sizeof(addressString));
    Log(Debug::Warning) << "Connection successful, Client id is" << clientId << " on address " << addressString;
}

std::unique_ptr<yojimbo::Client> MWNet::Client::createClientInstance()
{
    Log(Debug::Info) << "started client on port " << MWNet::DefaultClientPort << " (insecure)";

    return std::make_unique<yojimbo::Client>(yojimbo::GetDefaultAllocator(),
        yojimbo::Address(MWNet::LocalHost, MWNet::DefaultClientPort), mConfig, *mAdapter, 0.0);
}

bool MWNet::Client::tick()
{
    if (quit || mClient->ConnectionFailed())
    {
        if (mClient->IsConnected())
        {
            mClient->Disconnect();
        }
        return false;
    }

    double currentTime = yojimbo_time();
    if (mTime <= currentTime)
    {
        updateConnection();
    }
    else
    {
        yojimbo_sleep(mTime - currentTime);
    }

    return true;
}

void MWNet::Client::updateConnection()
{
    if (!mClient->IsConnected() && !mClient->IsConnecting())
    {
        return;
    }

    mTime += MWNet::TickRate;
    mClient->AdvanceTime(mTime);
    mClient->ReceivePackets();
    processMessages();
    mClient->SendPackets();
}

void MWNet::Client::processMessages()
{
    if (!mClient->IsConnected())
    {
        return;
    }

    for (auto entry : mMessageQueue)
    {
        try
        {
            Log(Debug::Warning) << "Evaluating message type: " << entry->messageType;
            switch (entry->messageType)
            {
                case UnorderedSyncedMessage::GLOBAL_EVENT_QUEUED:
                {
                    GlobalEventDataMessageEntry* messageEntry = static_cast<GlobalEventDataMessageEntry*>(entry.get());
                    GlobalEventQueuedMessage* message
                        = static_cast<GlobalEventQueuedMessage*>(mClient->CreateMessage(entry->messageType));
                    message->eventName = messageEntry->eventName;
                    message->eventData = messageEntry->eventData;
                    mClient->SendMessage(messageEntry->channelName, message);
                    break;
                }
                case UnorderedSyncedMessage::PLAYER_LOGIN_MESSAGE:
                    // mClient->SendMessage(entry.channelName, entry.message);
                    break;
                default:
                    break;
            }
        }
        catch (...)
        {
            Log(Debug::Error)
                << "Failed to send message network message; It may have been queued with an incorrect type.";
        }
    }

    mMessageQueue.clear();

    for (int i = 0; i < mConfig.numChannels; ++i)
    {
        yojimbo::Message* message = mClient->ReceiveMessage(i);

        while (message)
        {
            processMessage(message);

            mClient->ReleaseMessage(message);

            message = mClient->ReceiveMessage(i);
        }
    }
}

void MWNet::Client::processMessage(yojimbo::Message* message)
{
    switch (message->GetType())
    {
        case (int)TestMessageType::TEST_MESSAGE:
            // processTestMessage((TestMessage*)message);
            break;
        default:
            break;
    }
}
