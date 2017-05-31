#include <iostream>
#include <Kbhit.h>
#include <RakSleep.h>
#include "MasterServer.hpp"
#include "RestServer.hpp"

using namespace RakNet;
using namespace std;

unique_ptr<RestServer> restServer;
unique_ptr<MasterServer> masterServer;
bool run = true;

int main()
{
    masterServer.reset(new MasterServer(2000, 25560));
    restServer.reset(new RestServer(8080, masterServer->GetServers()));

    auto onExit = [](int /*sig*/){
        restServer->stop();
        masterServer->Stop(false);
        masterServer->Wait();
        run = false;
    };

    signal(SIGINT, onExit);
    signal(SIGTERM, onExit);

    masterServer->Start();

    thread server_thread([]() { restServer->start(); });

    server_thread.join();
    masterServer->Wait();

    return 0;
}
