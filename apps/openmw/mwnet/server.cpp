#include <csignal>
#include <stdexcept>
#include <time.h>

#include <yojimbo.h>

#include "server.hpp"
#include <components/debug/debuglog.hpp>

static volatile int quit = 0;

void interrupt_handler(int)
{
    quit = 1;
}

MWNet::Server::Server()
    : mAdapter(MWNet::GameAdapter)
{
    if (!InitializeYojimbo())
    {
        throw std::logic_error("error: failed to initialize Yojimbo!\n");
    }

    mServer = CreateServerInstance();

    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);

    // Seed the random number generator
    srand((unsigned int)time(NULL));
}

std::unique_ptr<yojimbo::Server> MWNet::Server::CreateServerInstance()
{
    Log(Debug::Info) << "started server on port " << DefaultPort << " (insecure)";

    yojimbo::ClientServerConfig config;

    std::unique_ptr<yojimbo::Server> server
        = std::make_unique<yojimbo::Server>(yojimbo::GetDefaultAllocator(), MWNet::DefaultPrivateKey,
            yojimbo::Address(MWNet::LocalHost, DefaultPort), config, mAdapter, MWNet::TimeAdvanceUnits);

    if (!server)
    {
        throw std::logic_error("MWNet: failed to create server!\n");
    }

    return server;
}

int MWNet::Server::run()
{
    double time = MWNet::TimeAdvanceUnits;

    mServer->Start(DefaultMaxClients);

    char addressString[256];
    mServer->GetAddress().ToString(addressString, sizeof(addressString));
    Log(Debug::Info) << "server address is " << addressString << "\n"; // I think log gives us a newline?

    signal(SIGINT, interrupt_handler);

    while (!quit)
    {
        mServer->SendPackets();

        mServer->ReceivePackets();

        time += MWNet::TickRate;

        mServer->AdvanceTime(time);

        if (!mServer->IsRunning())
            break;

        yojimbo_sleep(MWNet::TickRate);
    }

    mServer->Stop();

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
