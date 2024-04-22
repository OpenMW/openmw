#include <csignal>
#include <stdexcept>
#include <time.h>

#include <yojimbo.h>

#include "client.hpp"
#include <components/debug/debuglog.hpp>

static volatile int quit = 0;

void clientInterruptHandler(int)
{
    quit = 1;
}

MWNet::Client::Client()
{
    if (!InitializeYojimbo())
    {
        throw std::logic_error("error: failed to initialize Yojimbo!\n");
    }

    mAdapter = std::make_unique<MWNet::ClientAdapter>(*this);

    mClient = createClientInstance();

    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);

    srand((unsigned int)time(NULL));

    signal(SIGINT, clientInterruptHandler);

    Address serverAddress(MWNet::LocalHost, MWNet::DefaultServerPort);

    uint64_t clientId = 0;
    yojimbo_random_bytes((uint8_t*)&clientId, 8);
    Log(Debug::Warning) << "Client id is" << clientId;

    mClient->InsecureConnect(MWNet::DefaultPrivateKey, clientId, serverAddress);

    char addressString[256];
    mClient->GetAddress().ToString(addressString, sizeof(addressString));
    Log(Debug::Warning) << "Client address is " << addressString;
}

std::unique_ptr<yojimbo::Client> MWNet::Client::createClientInstance()
{
    Log(Debug::Info) << "started client on port " << MWNet::DefaultClientPort << " (insecure)";

    std::unique_ptr<yojimbo::Client> client = std::make_unique<yojimbo::Client>(yojimbo::GetDefaultAllocator(),
        yojimbo::Address(MWNet::LocalHost, MWNet::DefaultClientPort), mConfig, *mAdapter, 0.0);

    if (!client)
    {
        throw std::logic_error("MWNet: failed to create yojimbo client!\n");
    }

    return client;
}

int MWNet::Client::tick()
{
    if (quit || mClient->ConnectionFailed() || mClient->IsDisconnected())
    {
        if (mClient->IsConnected())
        {
            mClient->Disconnect();
        }
        return 1;
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

    return 0;
}

void MWNet::Client::updateConnection()
{
    mTime += MWNet::TickRate;
    mClient->AdvanceTime(mTime);
    mClient->ReceivePackets();
    processMessages();

    TestMessage* message = (TestMessage*)mClient->CreateMessage((int)TestMessageType::TEST_MESSAGE);
    message->sequence = 42;
    mClient->SendMessage((int)yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED, message);

    // Actually process messages in between
    mClient->SendPackets();
}

void MWNet::Client::processMessages()
{
    if (!mClient->IsConnected())
    {
        return;
    }

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
            processTestMessage((TestMessage*)message);
            break;
        default:
            break;
    }
}

// void MWNet::Client::sendMessage

void MWNet::Client::processTestMessage(TestMessage* message)
{
    Log(Debug::Info) << "CLIENT: received test message from server: " << message->sequence;
}
