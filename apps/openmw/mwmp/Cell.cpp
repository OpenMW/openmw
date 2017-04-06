#include "../mwworld/worldimp.hpp"
#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>

#include "Cell.hpp"
#include "Main.hpp"
#include "LocalPlayer.hpp"
using namespace mwmp;

mwmp::Cell::Cell(MWWorld::CellStore* cellStore)
{
    store = cellStore;

    std::map<std::string, LocalActor *> localActors;
    std::map<std::string, DedicatedActor *> dedicatedActors;
}

mwmp::Cell::~Cell()
{

}

void Cell::update()
{
    for (std::map<std::string, LocalActor *>::iterator it = localActors.begin(); it != localActors.end();)
    {
        LocalActor *actor = it->second;

        // TODO:: Make sure this condition actually works
        if (actor->getPtr().getCell() != nullptr && actor->getPtr().getCell() != store)
        {
            LOG_APPEND(Log::LOG_INFO, "- Removing LocalActor %s which is no longer in this cell", it->first.c_str());
            localActors.erase(it++);
        }
        else
        {
            LOG_APPEND(Log::LOG_VERBOSE, "- Updating LocalActor %s", it->first.c_str());
            actor->update();
            ++it;
        }
    }
}

void Cell::initializeLocalActors()
{
    ESM::Cell esmCell = *store->getCell();
    MWWorld::CellRefList<ESM::NPC> *npcList = store->getNpcs();

    for (typename MWWorld::CellRefList<ESM::NPC>::List::iterator listIter(npcList->mList.begin());
        listIter != npcList->mList.end(); ++listIter)
    {
        MWWorld::Ptr ptr(&*listIter, 0);

        std::string mapIndex = generateMapIndex(ptr);
        localActors[mapIndex] = new LocalActor();
        localActors[mapIndex]->cell = esmCell;
        localActors[mapIndex]->setPtr(ptr);
        LOG_APPEND(Log::LOG_INFO, "- Initialized LocalActor %s", mapIndex.c_str());
    }
}

std::string Cell::generateMapIndex(MWWorld::Ptr ptr)
{
    std::string mapIndex = "";
    mapIndex += ptr.getCellRef().getRefId();
    mapIndex += "-" + std::to_string(ptr.getCellRef().getRefNum().mIndex);
    mapIndex += "-" + std::to_string(ptr.getCellRef().getMpNum());
    return mapIndex;
}

MWWorld::CellStore *mwmp::Cell::getCellStore()
{
    return store;
}

std::string mwmp::Cell::getDescription()
{
    return store->getCell()->getDescription();
}
