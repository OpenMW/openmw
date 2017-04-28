#include "ActorList.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "LocalPlayer.hpp"

#include <components/openmw-mp/Log.hpp>

using namespace mwmp;
using namespace std;

ActorList::ActorList()
{

}

ActorList::~ActorList()
{

}

Networking *ActorList::getNetworking()
{
    return mwmp::Main::get().getNetworking();
}

void ActorList::reset()
{
    cell.blank();
    baseActors.clear();
    positionActors.clear();
    animFlagsActors.clear();
    animPlayActors.clear();
    speechActors.clear();
    statsDynamicActors.clear();
    attackActors.clear();
    cellChangeActors.clear();
    guid = mwmp::Main::get().getNetworking()->getLocalPlayer()->guid;
}

void ActorList::addActor(BaseActor baseActor)
{
    baseActors.push_back(baseActor);
}

void ActorList::addActor(LocalActor localActor)
{
    baseActors.push_back(localActor);
}

void ActorList::addPositionActor(LocalActor localActor)
{
    positionActors.push_back(localActor);
}

void ActorList::addAnimFlagsActor(LocalActor localActor)
{
    animFlagsActors.push_back(localActor);
}

void ActorList::addAnimPlayActor(LocalActor localActor)
{
    animPlayActors.push_back(localActor);
}

void ActorList::addSpeechActor(LocalActor localActor)
{
    speechActors.push_back(localActor);
}

void ActorList::addStatsDynamicActor(LocalActor localActor)
{
    statsDynamicActors.push_back(localActor);
}

void ActorList::addAttackActor(LocalActor localActor)
{
    attackActors.push_back(localActor);
}

void ActorList::addCellChangeActor(LocalActor localActor)
{
    cellChangeActors.push_back(localActor);
}

void ActorList::sendPositionActors()
{
    if (positionActors.size() > 0)
    {
        baseActors = positionActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_POSITION)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_POSITION)->Send();
    }
}

void ActorList::sendAnimFlagsActors()
{
    if (animFlagsActors.size() > 0)
    {
        baseActors = animFlagsActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ANIM_FLAGS)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ANIM_FLAGS)->Send();
    }
}

void ActorList::sendAnimPlayActors()
{
    if (animPlayActors.size() > 0)
    {
        baseActors = animPlayActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ANIM_PLAY)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ANIM_PLAY)->Send();
    }
}

void ActorList::sendSpeechActors()
{
    if (speechActors.size() > 0)
    {
        baseActors = speechActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_SPEECH)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_SPEECH)->Send();
    }
}

void ActorList::sendStatsDynamicActors()
{
    if (statsDynamicActors.size() > 0)
    {
        baseActors = statsDynamicActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_STATS_DYNAMIC)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_STATS_DYNAMIC)->Send();
    }
}

void ActorList::sendAttackActors()
{
    if (attackActors.size() > 0)
    {
        baseActors = attackActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ATTACK)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_ATTACK)->Send();
    }
}

void ActorList::sendCellChangeActors()
{
    if (cellChangeActors.size() > 0)
    {
        baseActors = cellChangeActors;
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_CELL_CHANGE)->setActorList(this);
        Main::get().getNetworking()->getActorPacket(ID_ACTOR_CELL_CHANGE)->Send();
    }
}

// TODO: Finish this
void ActorList::editActorsInCell(MWWorld::CellStore* cellStore)
{
    BaseActor actor;

    for (unsigned int i = 0; i < count; i++)
    {
        actor = baseActors.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", actor.refId.c_str(), actor.refNumIndex, actor.mpNum);

        return;

        MWWorld::Ptr ptrFound = cellStore->searchExact(actor.refId, actor.refNumIndex, actor.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());
        }
        else
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "-- Could not find %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());
        }
    }
}

void ActorList::sendActorsInCell(MWWorld::CellStore* cellStore)
{
    reset();
    cell = *cellStore->getCell();
    action = BaseActorList::SET;

    MWWorld::CellRefList<ESM::NPC> *npcList = cellStore->getNpcs();

    for (typename MWWorld::CellRefList<ESM::NPC>::List::iterator listIter(npcList->mList.begin());
        listIter != npcList->mList.end(); ++listIter)
    {
        MWWorld::Ptr ptr(&*listIter, 0);

        // If this Ptr is lacking a unique index, ignore it
        if (ptr.getCellRef().getRefNum().mIndex == 0 && ptr.getCellRef().getMpNum() == 0) continue;

        BaseActor actor;
        actor.refId = ptr.getCellRef().getRefId();
        actor.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
        actor.mpNum = ptr.getCellRef().getMpNum();

        addActor(actor);
    }

    MWWorld::CellRefList<ESM::Creature> *creatureList = cellStore->getCreatures();

    for (typename MWWorld::CellRefList<ESM::Creature>::List::iterator listIter(creatureList->mList.begin());
        listIter != creatureList->mList.end(); ++listIter)
    {
        MWWorld::Ptr ptr(&*listIter, 0);

        // If this Ptr is lacking a unique index, ignore it
        if (ptr.getCellRef().getRefNum().mIndex == 0 && ptr.getCellRef().getMpNum() == 0) continue;

        BaseActor actor;
        actor.refId = ptr.getCellRef().getRefId();
        actor.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
        actor.mpNum = ptr.getCellRef().getMpNum();

        addActor(actor);
    }

    mwmp::Main::get().getNetworking()->getActorPacket(ID_ACTOR_LIST)->setActorList(this);
    mwmp::Main::get().getNetworking()->getActorPacket(ID_ACTOR_LIST)->Send();
}
