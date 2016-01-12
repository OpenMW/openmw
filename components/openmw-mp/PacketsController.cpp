//
// Created by koncord on 15.01.16.
//

#include "Packets/PacketPosition.hpp"
#include "Packets/PacketBaseInfo.hpp"
#include "components/openmw-mp/Packets/PacketEquiped.hpp"
#include "Packets/PacketAttributesAndStats.hpp"
#include "Packets/PacketAttack.hpp"
#include "Packets/PacketMainStats.hpp"
#include "Packets/PacketResurrect.hpp"
#include "Packets/PacketDie.hpp"
#include "Packets/PacketCell.hpp"
#include "Packets/PacketSendMyID.hpp"
#include "Packets/PacketDisconnect.hpp"
#include "Packets/PacketDrawState.hpp"
#include "Packets/PacketChatMessage.hpp"
#include "Packets/PacketCharGen.hpp"
#include "Packets/PacketAttribute.hpp"
#include "Packets/PacketSkill.hpp"
#include "Packets/PacketHandshake.hpp"

#include "PacketsController.hpp"


mwmp::PacketsController::PacketsController(RakNet::RakPeerInterface *peer)
{
    packetPosition = new PacketPosition(peer);
    packetCell = new PacketCell(peer);
    packetBaseInfo = new PacketBaseInfo(peer);
    packetEquiped = new PacketEquiped(peer);
    attributesAndStats = new PacketAttributesAndStats(peer);
    packetAttack = new PacketAttack(peer);
    packetMainStats = new PacketMainStats(peer);
    packetResurrect = new PacketResurrect(peer);
    packetDie = new PacketDie(peer);
    packetDrawState = new PacketDrawState(peer);

    packetSendMyID = new PacketSendMyID(peer);
    packetDisconnect = new PacketDisconnect(peer);

    packetChatMessage = new PacketChatMessage(peer);
    packetCharGen = new PacketCharGen(peer);

    packetAttribute = new PacketAttribute(peer);
    packetSkill = new PacketSkill(peer);

    packetHandshake = new PacketHandshake(peer);
}


mwmp::BasePacket *mwmp::PacketsController::GetPacket(RakNet::MessageID id)
{
    BasePacket * packet;
    switch(id)
    {
        case ID_GAME_UPDATE_POS:
            packet = packetPosition;
            break;
        case ID_GAME_CELL:
            packet = packetCell;
            break;
        case ID_GAME_BASE_INFO:
            packet = packetBaseInfo;
            break;
        case ID_GAME_UPDATE_EQUIPED:
            packet = packetEquiped;
            break;
        case ID_GAME_UPDATE_SKILLS:
            packet = attributesAndStats;
            break;
        case ID_GAME_ATTACK:
            packet = packetAttack;
            break;
        case ID_GAME_UPDATE_BASESTATS:
            packet = packetMainStats;
            break;
        case ID_GAME_RESURRECT:
            packet = packetResurrect;
            break;
        case ID_GAME_DIE:
            packet = packetDie;
            break;
        case ID_GAME_DRAWSTATE:
            packet = packetDrawState;
            break;
        case ID_USER_MYID:
            packet = packetSendMyID;
            break;
        case ID_USER_DISCONNECTED:
            packet = packetDisconnect;
            break;
        case ID_CHAT_MESSAGE:
            packet = packetChatMessage;
            break;
        case ID_GAME_CHARGEN:
            packet = packetCharGen;
            break;
        case ID_GAME_ATTRIBUTE:
            packet = packetAttribute;
            break;
        case ID_GAME_SKILL:
            packet = packetSkill;
            break;
        case ID_HANDSHAKE:
            packet = packetHandshake;
            break;
        default:
            packet = 0;
    }
    return packet;
}

void mwmp::PacketsController::SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    packetPosition->SetStreams(inStream, outStream);
    packetCell->SetStreams(inStream, outStream);
    packetBaseInfo->SetStreams(inStream, outStream);
    packetEquiped->SetStreams(inStream, outStream);
    attributesAndStats->SetStreams(inStream, outStream);
    packetAttack->SetStreams(inStream, outStream);
    packetMainStats->SetStreams(inStream, outStream);
    packetResurrect->SetStreams(inStream, outStream);
    packetDie->SetStreams(inStream, outStream);
    packetDrawState->SetStreams(inStream, outStream);

    packetSendMyID->SetStreams(inStream, outStream);
    packetDisconnect->SetStreams(inStream, outStream);

    packetChatMessage->SetStreams(inStream, outStream);
    packetCharGen->SetStreams(inStream, outStream);

    packetAttribute->SetStreams(inStream, outStream);
    packetSkill->SetStreams(inStream, outStream);

    packetHandshake->SetStreams(inStream, outStream);
}