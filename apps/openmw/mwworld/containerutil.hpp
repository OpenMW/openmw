#ifndef GAME_MWWORLD_CONTAINERUTIL_H
#define GAME_MWWORLD_CONTAINERUTIL_H

#include <string>
#include <vector>

#include <components/esm_store/store.hpp>

#include "containerstore.hpp"
#include "ptr.hpp"
#include "refdata.hpp"

namespace MWWorld
{
    void listItemsInContainer (const std::string& id, ContainerStore<MWWorld::RefData>& containerStore,
        const ESMS::ESMStore& store, std::vector<Ptr>& list);
    ///< append all references with the given id to list.
}

#endif
