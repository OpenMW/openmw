#include "client.hpp"

// Upgrade later to accept a destination & source address
MWNet::Client::Client()
    : mAddress(MWNet::LocalHost, 0)
    , mDestination(MWNet::LocalHost, MWNet::DefaultServerPort)
{
    if (!InitializeYojimbo())
    {
        throw std::logic_error("error: failed to initialize Yojimbo!\n");
    }

    mAdapter = std::make_unique<MWNet::ClientAdapter>(*this);

    mClient = std::make_unique<yojimbo::Client>(yojimbo::GetDefaultAllocator(), mAddress, mConfig, *mAdapter, 0.0);

    Log(Debug::Info) << "started client on port " << mClient->GetAddress().GetPort() << " (insecure)";

    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);

    srand(time(nullptr));

    yojimbo::Address serverAddress(MWNet::LocalHost, MWNet::DefaultServerPort);

    yojimbo_random_bytes((uint8_t*)&mClientId, 8);

    mClient->InsecureConnect(MWNet::DefaultPrivateKey, mClientId, serverAddress);

    if (!mClient->IsConnected())
    {
        Log(Debug::Warning) << "Client was not connected on network instantiation.";
        return;
    }

    // This is presently dead code since usually the client isn't able to connect to the server this early on
    std::string addressString(sizeof(yojimbo::Address), '\0');
    mClient->GetAddress().ToString(addressString.data(), sizeof(yojimbo::Address));
    Log(Debug::Info) << "Connection successful, Client id is" << mClientId << " on address " << addressString;
}

bool MWNet::Client::tick()
{
    if (mClient->ConnectionFailed())
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

void MWNet::Client::updateConnection()
{
    if (!mClient->IsConnected() && !mClient->IsConnecting())
    {
        return;
    }

    mTime += MWNet::TickRate;
    mClient->AdvanceTime(mTime);
    mClient->ReceivePackets();
    processIncomingMessages();
    processOutgoingMessages();
    mClient->SendPackets();
}

void MWNet::Client::processIncomingMessages()
{
    if (!mClient->IsConnected())
    {
        return;
    }

    bool disconnected = false;

    for (int channelIndex = 0; channelIndex < ChannelId::NUM_MWNET_CHANNELS; ++channelIndex)
    {
        if (disconnected)
        {
            break;
        }

        yojimbo::Message* message = mClient->ReceiveMessage(channelIndex);

        while (message)
        {
            disconnected = processIncomingMessage(message);

            mClient->ReleaseMessage(message);

            if (disconnected)
            {
                break;
            }

            message = mClient->ReceiveMessage(channelIndex);
        }
    }
}

bool MWNet::Client::processIncomingMessage(yojimbo::Message* message)
{
    unsigned int messageType = message->GetType();

    if (messageType >= MessageId::NUM_MWNET_MESSAGES)
    {
        Log(Debug::Error) << "Invalid message type received: " << messageType << ", disconnecting";
        mClient->Disconnect();
        return true;
    }

    try
    {
        mIncomingMessageHandlers[messageType](message);
        return false;
    }
    catch (std::runtime_error& e)
    {
        Log(Debug::Error) << "Error processing incoming message, disconnecting due to: " << e.what();
        mClient->Disconnect();
        return true;
    }
}

void MWNet::Client::processOutgoingMessages()
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
