#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BaseActor.hpp>

#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Utils.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>

#include <components/esm/creaturestats.hpp>

#include "Actors.hpp"

using namespace mwmp;

BaseActorList scriptActorList;

BaseActor tempActor;
const BaseActor emptyActor = {};

static std::string tempCellDescription;

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
    tempCellDescription = mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).cell.getDescription();
    return tempCellDescription.c_str();
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

double ActorFunctions::GetActorHealthBase(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).creatureStats->mDynamic[0].mBase;
}

double ActorFunctions::GetActorHealthCurrent(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).creatureStats->mDynamic[0].mCurrent;
}

double ActorFunctions::GetActorMagickaBase(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).creatureStats->mDynamic[1].mBase;
}

double ActorFunctions::GetActorMagickaCurrent(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).creatureStats->mDynamic[1].mCurrent;
}

double ActorFunctions::GetActorFatigueBase(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).creatureStats->mDynamic[2].mBase;
}

double ActorFunctions::GetActorFatigueCurrent(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastActorList()->baseActors.at(i).creatureStats->mDynamic[2].mCurrent;
}

void ActorFunctions::SetScriptActorListCell(const char* cellDescription) noexcept
{
    scriptActorList.cell = Utils::getCellFromDescription(cellDescription);
}

void ActorFunctions::SetScriptActorListAction(unsigned char action) noexcept
{
    scriptActorList.action = action;
}

void ActorFunctions::SetActorCell(const char* cellDescription) noexcept
{
    tempActor.cell = Utils::getCellFromDescription(cellDescription);
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

void ActorFunctions::SetActorPosition(double x, double y, double z) noexcept
{
    tempActor.position.pos[0] = x;
    tempActor.position.pos[1] = y;
    tempActor.position.pos[2] = z;
}

void ActorFunctions::SetActorRotation(double x, double y, double z) noexcept
{
    tempActor.position.rot[0] = x;
    tempActor.position.rot[1] = y;
    tempActor.position.rot[2] = z;
}

void ActorFunctions::SetActorHealthBase(double value) noexcept
{
    tempActor.creatureStats->mDynamic[0].mBase = value;
}

void ActorFunctions::SetActorHealthCurrent(double value) noexcept
{
    tempActor.creatureStats->mDynamic[0].mCurrent = value;
}

void ActorFunctions::SetActorMagickaBase(double value) noexcept
{
    tempActor.creatureStats->mDynamic[1].mBase = value;
}

void ActorFunctions::SetActorMagickaCurrent(double value) noexcept
{
    tempActor.creatureStats->mDynamic[1].mCurrent = value;
}

void ActorFunctions::SetActorFatigueBase(double value) noexcept
{
    tempActor.creatureStats->mDynamic[2].mBase = value;
}

void ActorFunctions::SetActorFatigueCurrent(double value) noexcept
{
    tempActor.creatureStats->mDynamic[2].mCurrent = value;
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

void ActorFunctions::SendActorStatsDynamic() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_STATS_DYNAMIC)->setActorList(&scriptActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_STATS_DYNAMIC)->Send(scriptActorList.guid);
}

void ActorFunctions::SendActorCellChange() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_CELL_CHANGE)->setActorList(&scriptActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_CELL_CHANGE)->Send(scriptActorList.guid);
}

