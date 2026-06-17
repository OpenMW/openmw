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

    mServer = std::make_unique<yojimbo::Server>(yojimbo::GetDefaultAllocator(), MWNet::DefaultPrivateKey,
        yojimbo::Address(MWNet::LocalHost, MWNet::DefaultServerPort), mConfig, *mAdapter, 0.0);

    Log(Debug::Info) << "started server on port " << mServer->GetAddress().GetPort() << " (insecure)";

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
    processIncomingMessages();
    processOutgoingMessages();
    mServer->SendPackets();
}

void MWNet::Server::processIncomingMessages()
{
    for (unsigned int clientIndex = 0; clientIndex < MWNet::DefaultMaxClients; ++clientIndex)
    {
        if (!mServer->IsClientConnected(clientIndex))
        {
            continue;
        }

        bool disconnectClient = false;

        for (int channelIndex = 0; channelIndex < ChannelId::NUM_MWNET_CHANNELS; ++channelIndex)
        {
            if (disconnectClient)
            {
                break;
            }

            yojimbo::Message* message = mServer->ReceiveMessage(clientIndex, channelIndex);

            while (message)
            {
                disconnectClient = processIncomingMessage(message, channelIndex, clientIndex);

                mServer->ReleaseMessage(clientIndex, message);

                if (disconnectClient)
                {
                    break;
                }

                message = mServer->ReceiveMessage(clientIndex, channelIndex);
            }
        }
    }
}

bool MWNet::Server::processIncomingMessage(
    yojimbo::Message* message, const unsigned int channelIndex, const unsigned int clientIndex)
{
    if (channelIndex >= ChannelId::NUM_MWNET_CHANNELS)
    {
        Log(Debug::Error) << "SERVER: received message on unknown channel: " << channelIndex << ", disconnecting "
                          << clientIndex;
        mServer->DisconnectClient(clientIndex);
        return true;
    }

    const unsigned int messageType = message->GetType();

    if (messageType >= MessageId::NUM_MWNET_MESSAGES)
    {
        Log(Debug::Error) << "SERVER: received unknown message type: " << messageType << ", disconnecting "
                          << clientIndex;
        mServer->DisconnectClient(clientIndex);
        return true;
    }

    try
    {
        mIncomingMessageHandlers[messageType](clientIndex, message);
        return false;
    }
    catch (std::runtime_error& e)
    {
        Log(Debug::Error) << "Error processing incoming message from " << clientIndex
                          << ", disconnecting them due to: " << e.what();
        mServer->DisconnectClient(clientIndex);
        return true;
    }
}

void MWNet::Server::processOutgoingMessages()
{
    for (auto& entry : mMessageQueue)
    {
        unsigned int messageType = entry->messageType;

        if (messageType >= MessageId::NUM_MWNET_MESSAGES)
        {
            Log(Debug::Error) << "Invalid message type queued: " << messageType;
            continue;
        }

        try
        {
            mOutgoingMessageHandlers[messageType](entry.get());
        }
        catch (std::runtime_error& e)
        {
            Log(Debug::Error) << "Error processing outgoing message: " << e.what();
        }
    }

    mMessageQueue.clear();
}

void MWNet::Server::clientConnected(const unsigned int clientIndex)
{
    Log(Debug::Info) << "SERVER: client connected: " << clientIndex;
}

void MWNet::Server::clientDisconnected(const unsigned int clientIndex)
{
    Log(Debug::Info) << "SERVER: client disconnected: " << clientIndex;
}
