#include <apps/openmw/mwnet/networkmessages.hpp>
#include <csignal>
#include <stdexcept>
#include <time.h>

#include <yojimbo.h>

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
    // Actually process messages in between
    mServer->SendPackets();
}

void MWNet::Server::processMessages()
{
    for (unsigned int i = 0; i < MWNet::DefaultMaxClients; i++)
    {
        if (!mServer->IsClientConnected(i))
        {
            continue;
        }

        for (int j = 0; j < mConfig.numChannels; j++)
        {
            yojimbo::Message* message = mServer->ReceiveMessage(i, j);
            while (message)
            {
                processMessage(i, message);

                mServer->ReleaseMessage(i, message);

                message = mServer->ReceiveMessage(i, j);
            }
        }
    }
}

void MWNet::Server::processMessage(int clientIndex, yojimbo::Message* message)
{
    switch (message->GetType())
    {
        case (int)TestMessageType::TEST_MESSAGE:
            processTestMessage(clientIndex, (TestMessage*)message);
            break;
        default:
            break;
    }
}

void MWNet::Server::processTestMessage(int clientIndex, TestMessage* message)
{
    Log(Debug::Info) << "SERVER: received test message from client " << clientIndex << ": " << message->sequence;

    TestMessage* response = (TestMessage*)mServer->CreateMessage((int)TestMessageType::TEST_MESSAGE, clientIndex);
    response->sequence = 24;
    mServer->SendMessage((int)yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED, clientIndex, response);
}

void MWNet::Server::clientConnected(int clientIndex)
{
    Log(Debug::Info) << "SERVER: client connected: " << clientIndex;
}

void MWNet::Server::clientDisconnected(int clientIndex)
{
    Log(Debug::Info) << "SERVER: client disconnected: " << clientIndex;
}
