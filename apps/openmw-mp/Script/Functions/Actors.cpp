#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BaseActor.hpp>

#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Utils.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>

#include "Actors.hpp"

using namespace mwmp;

BaseActorList scriptActorList;

BaseActor tempActor;
const BaseActor emptyActor = {};

void ActorFunctions::InitScriptActorList(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    scriptActorList.cell.blank();
    scriptActorList.baseActors.clear();
    scriptActorList.guid = player->guid;
}

unsigned int ActorFunctions::GetActorListSize() noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->count;
}

unsigned char ActorFunctions::GetLastActorListAction() noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->action;
}

const char *ActorFunctions::GetActorCell(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).cell.getDescription().c_str();
}

const char *ActorFunctions::GetActorRefId(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).refId.c_str();
}

int ActorFunctions::GetActorRefNumIndex(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).refNumIndex;
}

int ActorFunctions::GetActorMpNum(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).mpNum;
}

double ActorFunctions::GetActorPosX(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).position.pos[0];
}

double ActorFunctions::GetActorPosY(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).position.pos[1];
}

double ActorFunctions::GetActorPosZ(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).position.pos[2];
}

double ActorFunctions::GetActorRotX(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).position.rot[0];
}

double ActorFunctions::GetActorRotY(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).position.rot[1];
}

double ActorFunctions::GetActorRotZ(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).position.rot[2];
}

void ActorFunctions::SetScriptActorListCell(const char* cellDescription) noexcept
{
    scriptActorList.cell = Utils::getCellFromDescription(cellDescription);
}

void ActorFunctions::SetScriptActorListAction(unsigned char action) noexcept
{
    scriptActorList.action = action;
}

void ActorFunctions::SetActorRefId(const char* refId) noexcept
{
    tempActor.refId = refId;
}

void ActorFunctions::SetActorRefNumIndex(int refNumIndex) noexcept
{
    tempActor.refNumIndex = refNumIndex;
}

void ActorFunctions::SetActorMpNum(int mpNum) noexcept
{
    tempActor.mpNum = mpNum;
}

void ActorFunctions::AddActor() noexcept
{
    scriptActorList.baseActors.push_back(tempActor);

    tempActor = emptyActor;
}

void ActorFunctions::SendActorList() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_LIST)->setActorList(&scriptActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_LIST)->Send(scriptActorList.guid);
}

void ActorFunctions::SendActorAuthority() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_AUTHORITY)->setActorList(&scriptActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_AUTHORITY)->Send(scriptActorList.guid);
}

