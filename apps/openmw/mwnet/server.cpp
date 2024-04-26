#include "server.hpp"

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

    srand(time(nullptr));

    mServer->Start(DefaultMaxClients);

    if (!mServer->IsRunning())
    {
        throw std::logic_error("error: failed to start server!\n");
    }

    std::string addressString(sizeof(yojimbo::Address), '\0');
    mServer->GetAddress().ToString(addressString.data(), sizeof(yojimbo::Address));
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

        for (int channelIndex = 0; channelIndex < ChannelId::NUM_MWNET_CHANNELS; ++channelIndex)
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

bool MWNet::Server::processMessage(
    const unsigned int clientIndex, const unsigned int channelIndex, yojimbo::Message* message)
{
    const unsigned int messageType = message->GetType();

    if (messageType >= MessageId::NUM_MWNET_MESSAGES)
    {
        Log(Debug::Error) << "SERVER: received unknown message type: " << messageType << ", disconnecting "
                          << clientIndex;
        mServer->DisconnectClient(clientIndex);
        return false;
    }

    switch (channelIndex)
    {
        case ChannelId::EVENTSQUEUE:
        {
            mMessageHandlers[messageType](clientIndex, message);
            return true;
        }
        case ChannelId::GAMESTATE:
        {
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

void MWNet::Server::clientConnected(const unsigned int clientIndex)
{
    Log(Debug::Info) << "SERVER: client connected: " << clientIndex;
}

void MWNet::Server::clientDisconnected(const unsigned int clientIndex)
{
    Log(Debug::Info) << "SERVER: client disconnected: " << clientIndex;
}
