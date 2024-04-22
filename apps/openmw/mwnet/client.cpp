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

    mClient = createClientInstance();

    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);

    srand((unsigned int)time(NULL));

    signal(SIGINT, clientInterruptHandler);

    Address serverAddress(MWNet::LocalHost, MWNet::DefaultServerPort);

    uint64_t clientId = 0;
    yojimbo_random_bytes((uint8_t*)&clientId, 8);
    Log(Debug::Warning) << "Client id is" << clientId << "\n";

    mClient->InsecureConnect(MWNet::DefaultPrivateKey, clientId, serverAddress);

    char addressString[256];
    mClient->GetAddress().ToString(addressString, sizeof(addressString));
    Log(Debug::Warning) << "Client address is " << addressString << "\n";
}

std::unique_ptr<yojimbo::Client> MWNet::Client::createClientInstance()
{
    Log(Debug::Info) << "started client on port " << MWNet::DefaultClientPort << " (insecure)";

    yojimbo::ClientServerConfig config;

    std::unique_ptr<yojimbo::Client> client = std::make_unique<yojimbo::Client>(yojimbo::GetDefaultAllocator(),
        yojimbo::Address(MWNet::LocalHost, MWNet::DefaultClientPort), config, mAdapter, 0.0);

    if (!client)
    {
        throw std::logic_error("MWNet: failed to create server!\n");
    }

    return client;
}

int MWNet::Client::tick()
{
    if (quit)
    {
        return 1;
    }
    else if (mClient->ConnectionFailed() || mClient->IsDisconnected())
    {
        mClient->Disconnect();
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
    // Actually process messages in between
    mClient->SendPackets();
}

void MWNet::Client::clientConnected(int clientIndex)
{
    Log(Debug::Info) << "client connected: " << clientIndex << "\n";
}

void MWNet::Client::clientDisconnected(int clientIndex)
{
    Log(Debug::Info) << "client disconnected: " << clientIndex << "\n";
}
