#include <csignal>
#include <stdexcept>
#include <time.h>

#include <yojimbo.h>

#include "networkmessages.hpp"
#include "server.hpp"
#include <components/debug/debuglog.hpp>

static volatile int quit = 0;

void server_interrupt_handler(int)
{
    quit = 1;
}

MWNet::Server::Server()
{
    if (!InitializeYojimbo())
    {
        throw std::logic_error("error: failed to initialize Yojimbo!\n");
    }

    mAdapter = std::make_unique<MWNet::ServerAdapter>(*this);

    mServer = createServerInstance();

    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);

    srand((unsigned int)time(NULL));

    mServer->Start(DefaultMaxClients);

    if (!mServer->IsRunning())
    {
        throw std::logic_error("error: failed to start server!\n");
    }

    char addressString[256];
    mServer->GetAddress().ToString(addressString, sizeof(addressString));
    Log(Debug::Info) << "server address is " << addressString;

    signal(SIGINT, server_interrupt_handler);
}

std::unique_ptr<yojimbo::Server> MWNet::Server::createServerInstance()
{
    Log(Debug::Info) << "started server on port " << MWNet::DefaultServerPort << " (insecure)";

    return std::make_unique<yojimbo::Server>(yojimbo::GetDefaultAllocator(), MWNet::DefaultPrivateKey,
        yojimbo::Address(MWNet::LocalHost, MWNet::DefaultServerPort), mConfig, *mAdapter, 0.0);
}

bool MWNet::Server::tick()
{
    if (quit)
    {
        mServer->Stop();
        return false;
    }
    else if (!mServer->IsRunning())
    {
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

void MWNet::Server::updateConnection()
{
    mTime += MWNet::TickRate;
    mServer->AdvanceTime(mTime);
    mServer->ReceivePackets();
    processMessages();
    mServer->SendPackets();
}

void MWNet::Server::processMessages()
{
    for (unsigned int clientIndex = 0; clientIndex < MWNet::DefaultMaxClients; ++clientIndex)
    {
        if (!mServer->IsClientConnected(clientIndex))
        {
            continue;
        }

        for (int channelIndex = 0; channelIndex < ChannelName::NUM_MWNET_CHANNELS; ++channelIndex)
        {
            yojimbo::Message* message = mServer->ReceiveMessage(clientIndex, channelIndex);
            while (message)
            {
                processMessage(clientIndex, channelIndex, message);

                mServer->ReleaseMessage(clientIndex, message);

                message = mServer->ReceiveMessage(clientIndex, channelIndex);
            }
        }
    }
}

bool MWNet::Server::processMessage(int clientIndex, int channelIndex, yojimbo::Message* message)
{
    const uint messageType = message->GetType();

    switch (channelIndex)
    {
        case ChannelName::EVENTSQUEUE:
        {
            Log(Debug::Info) << "SERVER: received message on EVENTSQUEUE channel";

            if (messageType >= UnorderedSyncedMessage::NUM_UNORDERED_SYNC_MESSAGES)
            {
                Log(Debug::Error) << "SERVER: received unknown message type: " << messageType << ", disconnecting "
                                  << clientIndex;
                mServer->DisconnectClient(clientIndex);
                return false;
            }
            mMessageHandlers[messageType](clientIndex, message);
            return true;
        }
        case ChannelName::GAMESTATE:
        {
            Log(Debug::Info) << "SERVER: received message on GAMESTATE channel";

            if (messageType >= UnorderedSyncedMessage::NUM_UNORDERED_SYNC_MESSAGES)
            {
                Log(Debug::Error) << "SERVER: received unknown message type: " << messageType << ", disconnecting "
                                  << clientIndex;
                return false;
            }
            mMessageHandlers[messageType](clientIndex, message);
            return true;
        }
        default:
        {
            Log(Debug::Error) << "SERVER: received message on unknown channel: " << channelIndex << ", disconnecting "
                              << clientIndex;
            mServer->DisconnectClient(clientIndex);
            return false;
        }
    }
}

void MWNet::Server::clientConnected(int clientIndex)
{
    Log(Debug::Info) << "SERVER: client connected: " << clientIndex;
}

void MWNet::Server::clientDisconnected(int clientIndex)
{
    Log(Debug::Info) << "SERVER: client disconnected: " << clientIndex;
}
