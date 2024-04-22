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

    mServer = createServerInstance();

    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);

    srand((unsigned int)time(NULL));

    mServer->Start(DefaultMaxClients);

    char addressString[256];
    mServer->GetAddress().ToString(addressString, sizeof(addressString));
    Log(Debug::Info) << "server address is " << addressString;

    signal(SIGINT, server_interrupt_handler);
}

std::unique_ptr<yojimbo::Server> MWNet::Server::createServerInstance()
{
    Log(Debug::Info) << "started server on port " << MWNet::DefaultServerPort << " (insecure)";

    yojimbo::ClientServerConfig config;

    std::unique_ptr<yojimbo::Server> server = std::make_unique<yojimbo::Server>(yojimbo::GetDefaultAllocator(),
        MWNet::DefaultPrivateKey, yojimbo::Address(MWNet::LocalHost, MWNet::DefaultServerPort), config, mAdapter, 0.0);

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
        updateConnection();
    }
    else
    {
        yojimbo_sleep(mTime - currentTime);
    }

    return 0;
}

void MWNet::Server::updateConnection()
{
    mTime += MWNet::TickRate;
    mServer->AdvanceTime(mTime);
    mServer->ReceivePackets();
    // Actually process messages in between
    mServer->SendPackets();
}

void MWNet::Server::clientConnected(int clientIndex)
{
    Log(Debug::Info) << "client connected: " << clientIndex;
}

void MWNet::Server::clientDisconnected(int clientIndex)
{
    Log(Debug::Info) << "client disconnected: " << clientIndex;
}
