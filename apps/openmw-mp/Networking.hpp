//
// Created by koncord on 12.01.16.
//

#ifndef OPENMW_NETWORKING_HPP
#define OPENMW_NETWORKING_HPP

#include <components/openmw-mp/Controllers/PlayerPacketController.hpp>
#include <components/openmw-mp/Controllers/ActorPacketController.hpp>
#include <components/openmw-mp/Controllers/WorldPacketController.hpp>
#include <components/openmw-mp/Packets/PacketPreInit.hpp>
#include "Player.hpp"

class MasterClient;
namespace  mwmp
{
    class Networking
    {
    public:
        Networking(RakNet::RakPeerInterface *peer);
        ~Networking();

        void newPlayer(RakNet::RakNetGUID guid);
        void disconnectPlayer(RakNet::RakNetGUID guid);
        void kickPlayer(RakNet::RakNetGUID guid);

        void processPlayerPacket(RakNet::Packet *packet);
        void processActorPacket(RakNet::Packet *packet);
        void processWorldPacket(RakNet::Packet *packet);
        void update(RakNet::Packet *packet);

        unsigned short numberOfConnections() const;
        unsigned int maxConnections() const;
        int getAvgPing(RakNet::AddressOrGUID) const;

        int mainLoop();

        void stopServer(int code);

        PlayerPacketController *getPlayerPacketController() const;
        ActorPacketController *getActorPacketController() const;
        WorldPacketController *getWorldPacketController() const;

        BaseActorList *getLastActorList();
        BaseEvent *getLastEvent();

        int getCurrentMpNum();
        void setCurrentMpNum(int value);
        int incrementMpNum();

        MasterClient *getMasterClient();
        void InitQuery(std::string queryAddr, unsigned short queryPort);
        void setServerPassword(std::string passw) noexcept;
        bool isPassworded() const;

        static const Networking &get();
        static Networking *getPtr();

        void postInit();
    private:
        PacketPreInit::PluginContainer getPluginListSample();
        std::string serverPassword;
        static Networking *sThis;

        RakNet::RakPeerInterface *peer;
        RakNet::BitStream bsOut;
        TPlayers *players;
        MasterClient *mclient;

        BaseActorList baseActorList;
        BaseEvent baseEvent;

        PlayerPacketController *playerPacketController;
        ActorPacketController *actorPacketController;
        WorldPacketController *worldPacketController;

        bool running;
        int exitCode;
        PacketPreInit::PluginContainer samples;
    };
}


#endif //OPENMW_NETWORKING_HPP
