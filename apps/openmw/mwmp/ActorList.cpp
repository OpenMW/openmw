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

// TODO: Finish this
void ActorList::editActors(MWWorld::CellStore* cellStore)
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

void ActorList::sendActors(MWWorld::CellStore* cellStore)
{
    reset();
    cell = *cellStore->getCell();
    action = BaseActorList::SET;

    MWWorld::CellRefList<ESM::NPC> *npcList = cellStore->getNpcs();

    for (typename MWWorld::CellRefList<ESM::NPC>::List::iterator listIter(npcList->mList.begin());
        listIter != npcList->mList.end(); ++listIter)
    {
        MWWorld::Ptr ptr(&*listIter, 0);

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

        BaseActor actor;
        actor.refId = ptr.getCellRef().getRefId();
        actor.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
        actor.mpNum = ptr.getCellRef().getMpNum();

        addActor(actor);
    }

    mwmp::Main::get().getNetworking()->getActorPacket(ID_ACTOR_LIST)->setActorList(this);
    mwmp::Main::get().getNetworking()->getActorPacket(ID_ACTOR_LIST)->Send();
}
