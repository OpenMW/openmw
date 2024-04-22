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
    : mAdapter(MWNet::GameAdapter)
    , mTime(0)
{
    if (!InitializeYojimbo())
    {
        throw std::logic_error("error: failed to initialize Yojimbo!\n");
    }

    mServer = CreateServerInstance();

    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);

    srand((unsigned int)time(NULL));

    mServer->Start(DefaultMaxClients);

    char addressString[256];
    mServer->GetAddress().ToString(addressString, sizeof(addressString));
    Log(Debug::Info) << "server address is " << addressString << "\n";

    signal(SIGINT, server_interrupt_handler);
}

std::unique_ptr<yojimbo::Server> MWNet::Server::CreateServerInstance()
{
    Log(Debug::Info) << "started server on port " << DefaultPort << " (insecure)";

    yojimbo::ClientServerConfig config;

    std::unique_ptr<yojimbo::Server> server = std::make_unique<yojimbo::Server>(yojimbo::GetDefaultAllocator(),
        MWNet::DefaultPrivateKey, yojimbo::Address(MWNet::LocalHost, DefaultPort), config, mAdapter, 0.0);

    if (!server)
    {
        throw std::logic_error("MWNet: failed to create server!\n");
    }

    return server;
}

int MWNet::Server::tick()
{
    if (quit)
    {
        mServer->Stop();
        return 1;
    }
    else if (!mServer->IsRunning())
    {
        return 1;
    }

    double currentTime = yojimbo_time();
    if (mTime <= currentTime)
    {
        mTime += MWNet::TickRate;
        mServer->AdvanceTime(mTime);
        mServer->ReceivePackets();
        // Actually process messages in between
        mServer->SendPackets();
    }
    else
    {
        yojimbo_sleep(mTime - currentTime);
    }

    return 0;
}

void MWNet::Server::ClientConnected(int clientIndex)
{
    Log(Debug::Info) << "client connected: " << clientIndex << "\n";
}

void MWNet::Server::ClientDisconnected(int clientIndex)
{
    Log(Debug::Info) << "client disconnected: " << clientIndex << "\n";
}
