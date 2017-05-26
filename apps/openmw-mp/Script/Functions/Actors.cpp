#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BaseActor.hpp>

#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Utils.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>

#include <components/esm/creaturestats.hpp>

#include "Actors.hpp"

using namespace mwmp;

BaseActorList *readActorList;
BaseActorList writeActorList;

BaseActor tempActor;
const BaseActor emptyActor = {};

static std::string tempCellDescription;

void ActorFunctions::ReadLastActorList() noexcept
{
    readActorList = mwmp::Networking::getPtr()->getLastActorList();
}

void ActorFunctions::ReadCellActorList(const char* cellDescription) noexcept
{
    ESM::Cell esmCell = Utils::getCellFromDescription(cellDescription);
    Cell *serverCell = CellController::get()->getCell(&esmCell);
    readActorList = serverCell->getActorList();
}

void ActorFunctions::InitializeActorList(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    writeActorList.cell.blank();
    writeActorList.baseActors.clear();
    writeActorList.guid = player->guid;
}

unsigned int ActorFunctions::GetActorListSize() noexcept
{
    return readActorList->count;
}

unsigned char ActorFunctions::GetActorListAction() noexcept
{
    return readActorList->action;
}

const char *ActorFunctions::GetActorCell(unsigned int i) noexcept
{
    tempCellDescription = readActorList->baseActors.at(i).cell.getDescription();
    return tempCellDescription.c_str();
}

const char *ActorFunctions::GetActorRefId(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).refId.c_str();
}

int ActorFunctions::GetActorRefNumIndex(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).refNumIndex;
}

int ActorFunctions::GetActorMpNum(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).mpNum;
}

double ActorFunctions::GetActorPosX(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).position.pos[0];
}

double ActorFunctions::GetActorPosY(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).position.pos[1];
}

double ActorFunctions::GetActorPosZ(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).position.pos[2];
}

double ActorFunctions::GetActorRotX(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).position.rot[0];
}

double ActorFunctions::GetActorRotY(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).position.rot[1];
}

double ActorFunctions::GetActorRotZ(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).position.rot[2];
}

double ActorFunctions::GetActorHealthBase(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).creatureStats.mDynamic[0].mBase;
}

double ActorFunctions::GetActorHealthCurrent(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).creatureStats.mDynamic[0].mCurrent;
}

double ActorFunctions::GetActorMagickaBase(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).creatureStats.mDynamic[1].mBase;
}

double ActorFunctions::GetActorMagickaCurrent(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).creatureStats.mDynamic[1].mCurrent;
}

double ActorFunctions::GetActorFatigueBase(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).creatureStats.mDynamic[2].mBase;
}

double ActorFunctions::GetActorFatigueCurrent(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).creatureStats.mDynamic[2].mCurrent;
}

const char *ActorFunctions::GetActorEquipmentItemRefId(unsigned int i, unsigned short slot) noexcept
{
    return readActorList->baseActors.at(i).equipedItems[slot].refId.c_str();
}

int ActorFunctions::GetActorEquipmentItemCount(unsigned int i, unsigned short slot) noexcept
{
    return readActorList->baseActors.at(i).equipedItems[slot].count;
}

int ActorFunctions::GetActorEquipmentItemCharge(unsigned int i, unsigned short slot) noexcept
{
    return readActorList->baseActors.at(i).equipedItems[slot].charge;
}

bool ActorFunctions::DoesActorHavePosition(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).hasPositionData;
}

bool ActorFunctions::DoesActorHaveStatsDynamic(unsigned int i) noexcept
{
    return readActorList->baseActors.at(i).hasStatsDynamicData;
}

void ActorFunctions::SetActorListCell(const char* cellDescription) noexcept
{
    writeActorList.cell = Utils::getCellFromDescription(cellDescription);
}

void ActorFunctions::SetActorListAction(unsigned char action) noexcept
{
    writeActorList.action = action;
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
    tempActor.creatureStats.mDynamic[0].mBase = value;
}

void ActorFunctions::SetActorHealthCurrent(double value) noexcept
{
    tempActor.creatureStats.mDynamic[0].mCurrent = value;
}

void ActorFunctions::SetActorMagickaBase(double value) noexcept
{
    tempActor.creatureStats.mDynamic[1].mBase = value;
}

void ActorFunctions::SetActorMagickaCurrent(double value) noexcept
{
    tempActor.creatureStats.mDynamic[1].mCurrent = value;
}

void ActorFunctions::SetActorFatigueBase(double value) noexcept
{
    tempActor.creatureStats.mDynamic[2].mBase = value;
}

void ActorFunctions::SetActorFatigueCurrent(double value) noexcept
{
    tempActor.creatureStats.mDynamic[2].mCurrent = value;
}

void ActorFunctions::AddActor() noexcept
{
    writeActorList.baseActors.push_back(tempActor);

    tempActor = emptyActor;
}

void ActorFunctions::SendActorList() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_LIST)->setActorList(&writeActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_LIST)->Send(writeActorList.guid);
}

void ActorFunctions::SendActorAuthority() noexcept
{
    Cell *serverCell = CellController::get()->getCell(&writeActorList.cell);

    if (serverCell != nullptr)
    {
        serverCell->setAuthority(writeActorList.guid);

        mwmp::ActorPacket *authorityPacket = mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_AUTHORITY);
        authorityPacket->setActorList(&writeActorList);
        authorityPacket->Send(writeActorList.guid);

        // Also send this to everyone else who has the cell loaded
        serverCell->sendToLoaded(authorityPacket, &writeActorList);
    }
}

void ActorFunctions::SendActorPosition() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_POSITION)->setActorList(&writeActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_POSITION)->Send(writeActorList.guid);
}

void ActorFunctions::SendActorStatsDynamic() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_STATS_DYNAMIC)->setActorList(&writeActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_STATS_DYNAMIC)->Send(writeActorList.guid);
}

void ActorFunctions::SendActorCellChange() noexcept
{
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_CELL_CHANGE)->setActorList(&writeActorList);
    mwmp::Networking::get().getActorPacketController()->GetPacket(ID_ACTOR_CELL_CHANGE)->Send(writeActorList.guid);
}

