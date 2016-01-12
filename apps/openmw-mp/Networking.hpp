//
// Created by koncord on 12.01.16.
//

#ifndef OPENMW_NETWORKING_HPP
#define OPENMW_NETWORKING_HPP

#include <components/openmw-mp/PacketsController.hpp>

namespace  mwmp
{
    class Networking
    {
    public:
        Networking(RakNet::RakPeerInterface *peer);
        ~Networking();

        void NewPlayer(RakNet::RakNetGUID guid);
        void DisconnectPlayer(RakNet::RakNetGUID guid);
        void KickPlayer(RakNet::RakNetGUID guid);
        void Update(RakNet::Packet *packet);

        int MainLoop();

        void StopServer(int code);

        PacketsController *GetController() const;
        static const Networking &Get();
        static Networking *GetPtr();

    private:
        static Networking *sThis;
        RakNet::RakPeerInterface *peer;
        RakNet::BitStream bsOut;
        TPlayers *players;

        PacketsController *controller;

        bool running;
        int exitCode;
    };
}


#endif //OPENMW_NETWORKING_HPP
