#include <iostream>
#include <Kbhit.h>
#include <RakSleep.h>
#include "MasterServer.hpp"

using namespace RakNet;
using namespace std;

int main()
{
    MasterServer masterServer(2000, 25560);

    masterServer.Start();

    /*while(true)
    {
        if(kbhit())
        {
            if(getch() == 'e')
            {
                cout << endl;
                masterServer.Stop(true);
                break;
            }
        }
        RakSleep(100);
    }*/
    masterServer.Wait();
    return 0;
}