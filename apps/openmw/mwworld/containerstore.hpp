#ifndef GAME_MWWORLD_CONTAINERSTORE_H
#define GAME_MWWORLD_CONTAINERSTORE_H

#include <components/esm_store/cell_store.hpp>

#include "refdata.hpp"

namespace MWWorld
{
    struct ContainerStore
    {
            ESMS::CellRefList<ESM::Potion, RefData>            potions;
            ESMS::CellRefList<ESM::Apparatus, RefData>         appas;
            ESMS::CellRefList<ESM::Armor, RefData>             armors;
            ESMS::CellRefList<ESM::Book, RefData>              books;
            ESMS::CellRefList<ESM::Clothing, RefData>          clothes;
            ESMS::CellRefList<ESM::Ingredient, RefData>        ingreds;
            ESMS::CellRefList<ESM::Light, RefData>             lights;
            ESMS::CellRefList<ESM::Tool, RefData>              lockpicks;
            ESMS::CellRefList<ESM::Miscellaneous, RefData>     miscItems;
            ESMS::CellRefList<ESM::Probe, RefData>             probes;
            ESMS::CellRefList<ESM::Repair, RefData>            repairs;
            ESMS::CellRefList<ESM::Weapon, RefData>            weapons;
    };
}

#endif
