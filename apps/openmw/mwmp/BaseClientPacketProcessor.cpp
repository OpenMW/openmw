//
// Created by koncord on 18.04.17.
//

#include "BaseClientPacketProcessor.hpp"
#include "Main.hpp"

using namespace mwmp;

RakNet::RakNetGUID BaseClientPacketProcessor::guid;
RakNet::RakNetGUID BaseClientPacketProcessor::myGuid;
RakNet::SystemAddress BaseClientPacketProcessor::serverAddr;
bool BaseClientPacketProcessor::request;

LocalPlayer *BaseClientPacketProcessor::getLocalPlayer()
{
    return Main::get().getLocalPlayer();
}
